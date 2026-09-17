
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


#define LX2162A_PCI_VENDOR_ID    0x1234
#define LX2162A_PCI_DEVICE_ID    0x5678
#define LX2162A_PCI_BAR          0
#define LX2162A_PCI_MAGIC        'M'


#define BAR_REGISTER_READ   _IOWR(LX2162A_PCI_MAGIC, 0, struct bar_register)
#define BAR_REGISTER_WRITE  _IOW (LX2162A_PCI_MAGIC, 1, struct bar_register)


struct bar_register
{
    __u32 offset;
    __u32 value;
};


struct lx2162a_pci_device
{
    struct pci_dev*     pdev;
    void __iomem*       bar;
    resource_size_t     bar_start;
    resource_size_t     bar_size;
    dev_t               devt;
    struct cdev         cdev;
    struct device*      device;
};


static struct class* lx2162a_pci_class;


static int lx2162a_pci_analyzer_open(struct inode* inode, struct file* file)
{
    struct lx2162a_pci_device* device;

    device = container_of(
        inode->i_cdev,
        struct lx2162a_pci_device,
        cdev);

    file->private_data = device;

    return 0;
}


static int lx2162a_pci_analyzer_release(struct inode* inode, struct file* file)
{
    return 0;
}


static long lx2162a_pci_analyzer_ioctl(struct file* file, unsigned int cmd, unsigned long arg)
{
    struct lx2162a_pci_device* device           = file->private_data;
    struct bar_register         bar_register;

    if (!device)
    {
        return -ENODEV;
    }

    switch (cmd)
    {
    case BAR_REGISTER_READ:
        if (copy_from_user(&bar_register, (void __user*)arg, sizeof(bar_register)))
        {
            return -EFAULT;
        }

        if (bar_register.offset & 0x3)
        {
            return -EINVAL;
        }

        if ((resource_size_t)bar_register.offset + sizeof(u32) > device->bar_size)
        {
            return -EINVAL;
        }

        bar_register.value = ioread32(device->bar + bar_register.offset);

        if (copy_to_user((void __user*)arg, &bar_register, sizeof(bar_register)))
        {
            return -EFAULT;
        }

        return 0;

    case BAR_REGISTER_WRITE:
        if (copy_from_user(&bar_register, (void __user*)arg, sizeof(bar_register)))
        {
            return -EFAULT;
        }

        if (bar_register.offset & 0x3)
        {
            return -EINVAL;
        }

        if ((resource_size_t)bar_register.offset + sizeof(u32) > device->bar_size)
        {
            return -EINVAL;
        }

        iowrite32(bar_register.value, device->bar + bar_register.offset);

        readl(device->bar + bar_register.offset);

        return 0;

    default:
        return -ENOTTY;
    }
}


static const struct file_operations lx2162a_pci_fops =
{
    .owner              = THIS_MODULE,
    .open               = lx2162a_pci_analyzer_open,
    .release            = lx2162a_pci_analyzer_release,
    .unlocked_ioctl     = lx2162a_pci_analyzer_ioctl,
};


static int lx2162a_pci_analyzer_probe(struct pci_dev* pdev, const struct pci_device_id* id)
{
    struct lx2162a_pci_device*  dev;
    int                         ret;

    dev_info(&pdev->dev, "Lx2162aPciAnalyzer: device found: %04x:%04x\n", pdev->vendor, pdev->device);

    ret = pci_enable_device(pdev);

    if (ret)
    {
        dev_err(&pdev->dev, "pci_enable_device() failed: %d\n", ret);
        return ret;
    }

    ret = pci_request_region(pdev, LX2162A_PCI_BAR, DRIVER_NAME);

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

    dev->bar_start  = pci_resource_start(pdev, LX2162A_PCI_BAR);
    dev->bar_size   = pci_resource_len  (pdev, LX2162A_PCI_BAR);

    dev_info(&pdev->dev, "BAR%d start = 0x%llx\n", LX2162A_PCI_BAR, (unsigned long long)dev->bar_start);

    dev_info(&pdev->dev, "BAR%d size  = 0x%llx (%llu bytes)\n", LX2162A_PCI_BAR, (unsigned long long)dev->bar_size, (unsigned long long)dev->bar_size);

    if (!(pci_resource_flags(pdev, LX2162A_PCI_BAR) & IORESOURCE_MEM))
    {
        dev_err(&pdev->dev, "BAR%d is not a memory BAR\n", LX2162A_PCI_BAR);

        ret = -ENODEV;
        goto err_free;
    }

    dev->bar = pci_iomap(pdev, LX2162A_PCI_BAR, 0);

    if (!dev->bar)
    {
        dev_err(&pdev->dev, "pci_iomap() failed\n");

        ret = -ENOMEM;
        goto err_free;
    }

    dev_info(&pdev->dev, "BAR%d mapped at %p\n", LX2162A_PCI_BAR, dev->bar);

    ret = alloc_chrdev_region(&dev->devt, 0, 1, DEVICE_NAME);

    if (ret)
    {
        dev_err(&pdev->dev, "alloc_chrdev_region() failed: %d\n", ret);
        goto err_unmap;
    }

    cdev_init(&dev->cdev, &lx2162a_pci_fops);

    dev->cdev.owner = THIS_MODULE;

    ret = cdev_add(&dev->cdev, dev->devt, 1);

    if (ret)
    {
        dev_err(&pdev->dev, "cdev_add() failed: %d\n", ret);
        goto err_unregister;
    }

    dev->device = device_create(lx2162a_pci_class, &pdev->dev, dev->devt, dev, DEVICE_NAME);

    if (IS_ERR(dev->device))
    {
        ret = PTR_ERR(dev->device);

        dev_err(&pdev->dev, "device_create() failed: %d\n", ret);

        goto err_cdev;
    }

    pci_set_drvdata(pdev, dev);

    dev_info(&pdev->dev, "Lx2162aPciAnalyzer driver loaded successfully\n");

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
    pci_release_region(pdev, LX2162A_PCI_BAR);
    pci_disable_device(pdev);

    return ret;
}


static void lx2162a_pci_analyzer_remove(struct pci_dev* pdev)
{
    struct lx2162a_pci_device* device;

    device = pci_get_drvdata(pdev);

    if (!device)
    {
        return;
    }

    dev_info(&pdev->dev, "removing Lx2162aPciAnalyzer driver\n");

    device_destroy(lx2162a_pci_class, device->devt);

    cdev_del(&device->cdev);

    unregister_chrdev_region(device->devt, 1);

    if (device->bar)
    {
        pci_iounmap(pdev, device->bar);
    }

    pci_release_region(pdev, LX2162A_PCI_BAR);

    pci_disable_device(pdev);

    kfree(device);
}


static const struct pci_device_id lx2162a_pci_ids[] = {
    {
        PCI_DEVICE(LX2162A_PCI_VENDOR_ID, LX2162A_PCI_DEVICE_ID)
    },

    { 0, }
};


MODULE_DEVICE_TABLE(pci, lx2162a_pci_ids);


static struct pci_driver lx2162a_pci_driver =
{
    .name       = DRIVER_NAME,
    .id_table   = lx2162a_pci_ids,

    .probe      = lx2162a_pci_analyzer_probe,
    .remove     = lx2162a_pci_analyzer_remove,
};


static int __init lx2162a_pci_analyzer_init(void)
{
    int ret;

    lx2162a_pci_class = class_create(DRIVER_NAME);

    if (IS_ERR(lx2162a_pci_class))
    {
        return PTR_ERR(lx2162a_pci_class);
    }

    ret = pci_register_driver(&lx2162a_pci_driver);

    if (ret)
    {
        class_destroy(lx2162a_pci_class);
        return ret;
    }

    pr_info("Lx2162aPciAnalyzer: driver loaded\n");

    return 0;
}


static void __exit lx2162a_pci_analyzer_exit(void)
{
    pci_unregister_driver(&lx2162a_pci_driver);

    class_destroy(lx2162a_pci_class);

    pr_info("Lx2162aPciAnalyzer: driver unloaded\n");
}


module_init(lx2162a_pci_analyzer_init);
module_exit(lx2162a_pci_analyzer_exit);


MODULE_LICENSE("GPL");
MODULE_AUTHOR("Andre Kirchner");
MODULE_DESCRIPTION("Monitor SoC PCIe state");
MODULE_VERSION("0.01");
