
#include <linux/module.h>
#include <linux/pci.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/uaccess.h>
#include <linux/io.h>
#include <linux/slab.h>


#define DRIVER_NAME "Lx2162aPciAnalyzer"
#define DEVICE_NAME "Lx2162aPciAnalyzer"


#define MY_PCI_VENDOR_ID    0x1234
#define MY_PCI_DEVICE_ID    0x5678
#define MY_PCI_BAR          0
#define MYPCI_MAGIC         'M'


struct mypci_reg
{
    __u32 offset;
    __u32 value;
};


#define MYPCI_READ_REG  _IOWR(MYPCI_MAGIC, 0, struct mypci_reg)

#define MYPCI_WRITE_REG _IOW(MYPCI_MAGIC,  1, struct mypci_reg)


struct mypci_dev
{
    struct pci_dev*     pdev;
    void __iomem*       bar;
    resource_size_t     bar_start;
    resource_size_t     bar_size;
    dev_t               devt;
    struct cdev         cdev;
    struct device*      device;
};

static struct class* mypci_class;


static int mypci_open(struct inode* inode, struct file* file)
{
    struct mypci_dev* dev;

    dev = container_of(inode->i_cdev,
        struct mypci_dev,
        cdev);

    file->private_data = dev;

    return 0;
}


static int mypci_release(struct inode* inode, struct file* file)
{
    return 0;
}


static long mypci_ioctl(struct file* file, unsigned int cmd, unsigned long arg)
{
    struct mypci_dev* dev = file->private_data;
    struct mypci_reg reg;

    if (!dev)
    {
        return -ENODEV;
    }

    switch (cmd)
    {
    case MYPCI_READ_REG:
        if (copy_from_user(&reg, (void __user*)arg, sizeof(reg)))
        {
            return -EFAULT;
        }

        if (reg.offset & 0x3)
        {
            return -EINVAL;
        }

        if ((resource_size_t)reg.offset + sizeof(u32) > dev->bar_size)
        {
            return -EINVAL;
        }

        reg.value = ioread32(dev->bar + reg.offset);

        if (copy_to_user((void __user*)arg, &reg, sizeof(reg)))
        {
            return -EFAULT;
        }

        return 0;

    case MYPCI_WRITE_REG:
        if (copy_from_user(&reg, (void __user*)arg, sizeof(reg)))
        {
            return -EFAULT;
        }

        if (reg.offset & 0x3)
        {
            return -EINVAL;
        }

        if ((resource_size_t)reg.offset + sizeof(u32) > dev->bar_size)
        {
            return -EINVAL;
        }

        iowrite32(reg.value, dev->bar + reg.offset);

        readl(dev->bar + reg.offset);

        return 0;

    default:
        return -ENOTTY;
    }
}


static const struct file_operations mypci_fops =
{
    .owner              = THIS_MODULE,
    .open               = mypci_open,
    .release            = mypci_release,
    .unlocked_ioctl     = mypci_ioctl,
};


static int mypci_probe(struct pci_dev* pdev, const struct pci_device_id* id)
{
    struct mypci_dev* dev;
    int ret;

    dev_info(&pdev->dev, "my_pci: device found: %04x:%04x\n", pdev->vendor, pdev->device);

    ret = pci_enable_device(pdev);

    if (ret)
    {
        dev_err(&pdev->dev, "pci_enable_device() failed: %d\n", ret);
        return ret;
    }

    ret = pci_request_region(pdev, MY_PCI_BAR, DRIVER_NAME);

    if (ret)
    {
        dev_err(&pdev->dev, "pci_request_region() failed: %d\n", ret);

        pci_disable_device(pdev);
        return ret;
    }

    dev = kzalloc(sizeof(*dev), GFP_KERNEL);

    if (!dev)
    {
        ret = -ENOMEM;
        goto err_release_region;
    }

    dev->pdev = pdev;

    dev->bar_start  = pci_resource_start(pdev, MY_PCI_BAR);
    dev->bar_size   = pci_resource_len  (pdev, MY_PCI_BAR);

    dev_info(&pdev->dev, "BAR%d start = 0x%llx\n", MY_PCI_BAR, (unsigned long long)dev->bar_start);

    dev_info(&pdev->dev, "BAR%d size  = 0x%llx (%llu bytes)\n", MY_PCI_BAR, (unsigned long long)dev->bar_size, (unsigned long long)dev->bar_size);

    if (!(pci_resource_flags(pdev, MY_PCI_BAR) & IORESOURCE_MEM))
    {
        dev_err(&pdev->dev, "BAR%d is not a memory BAR\n", MY_PCI_BAR);

        ret = -ENODEV;
        goto err_free;
    }

    dev->bar = pci_iomap(pdev, MY_PCI_BAR, 0);

    if (!dev->bar)
    {
        dev_err(&pdev->dev, "pci_iomap() failed\n");

        ret = -ENOMEM;
        goto err_free;
    }

    dev_info(&pdev->dev, "BAR%d mapped at %p\n", MY_PCI_BAR, dev->bar);

    ret = alloc_chrdev_region(&dev->devt, 0, 1, DEVICE_NAME);

    if (ret)
    {
        dev_err(&pdev->dev, "alloc_chrdev_region() failed: %d\n", ret);
        goto err_unmap;
    }

    cdev_init(&dev->cdev, &mypci_fops);

    dev->cdev.owner = THIS_MODULE;

    ret = cdev_add(&dev->cdev, dev->devt, 1);

    if (ret)
    {
        dev_err(&pdev->dev, "cdev_add() failed: %d\n", ret);
        goto err_unregister;
    }

    dev->device = device_create(mypci_class, &pdev->dev, dev->devt, dev, DEVICE_NAME);

    if (IS_ERR(dev->device))
    {
        ret = PTR_ERR(dev->device);

        dev_err(&pdev->dev, "device_create() failed: %d\n", ret);

        goto err_cdev;
    }

    pci_set_drvdata(pdev, dev);

    dev_info(&pdev->dev, "my_pci driver loaded successfully\n");

    return 0;


err_cdev:
    cdev_del(&dev->cdev);

err_unregister:
    unregister_chrdev_region(dev->devt, 1);

err_unmap:
    pci_iounmap(pdev, dev->bar);

err_free:
    kfree(dev);

err_release_region:
    pci_release_region(pdev, MY_PCI_BAR);
    pci_disable_device(pdev);

    return ret;
}


static void mypci_remove(struct pci_dev* pdev)
{
    struct mypci_dev* dev;

    dev = pci_get_drvdata(pdev);

    if (!dev)
    {
        return;
    }

    dev_info(&pdev->dev, "removing my_pci driver\n");

    device_destroy(mypci_class, dev->devt);

    cdev_del(&dev->cdev);

    unregister_chrdev_region(dev->devt, 1);

    if (dev->bar)
    {
        pci_iounmap(pdev, dev->bar);
    }

    pci_release_region(pdev, MY_PCI_BAR);

    pci_disable_device(pdev);

    kfree(dev);
}


static const struct pci_device_id mypci_ids[] = {
    {
        PCI_DEVICE(MY_PCI_VENDOR_ID, MY_PCI_DEVICE_ID)
    },

    { 0, }
};


MODULE_DEVICE_TABLE(pci, mypci_ids);


static struct pci_driver mypci_driver =
{
    .name       = DRIVER_NAME,
    .id_table   = mypci_ids,

    .probe      = mypci_probe,
    .remove     = mypci_remove,
};


static int __init mypci_init(void)
{
    int ret;

    mypci_class = class_create(DRIVER_NAME);

    if (IS_ERR(mypci_class))
    {
        return PTR_ERR(mypci_class);
    }

    ret = pci_register_driver(&mypci_driver);

    if (ret)
    {
        class_destroy(mypci_class);
        return ret;
    }

    pr_info("my_pci: driver loaded\n");

    return 0;
}


static void __exit mypci_exit(void)
{
    pci_unregister_driver(&mypci_driver);

    class_destroy(mypci_class);

    pr_info("my_pci: driver unloaded\n");
}


module_init(mypci_init);
module_exit(mypci_exit);


MODULE_LICENSE("GPL");
MODULE_AUTHOR("Example");
MODULE_DESCRIPTION("Simple PCIe BAR register access driver");
MODULE_VERSION("1.0");
