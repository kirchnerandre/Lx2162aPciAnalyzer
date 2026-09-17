
#include <linux/module.h>
#include <linux/pci.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/uaccess.h>
#include <linux/io.h>
#include <linux/slab.h>


#define DRIVER_NAME             "Lx2162aPciAnalyzer"
#define DEVICE_NAME             "Lx2162aPciAnalyzer"


#define LX2162A_PCI_VENDOR_ID   0x1234
#define LX2162A_PCI_DEVICE_ID   0x5678
#define LX2162A_PCI_BAR         0
#define LX2162A_PCI_MAGIC       'M'


#define BAR_REGISTER_READ       _IOWR(LX2162A_PCI_MAGIC, 0, struct bar_register)
#define BAR_REGISTER_WRITE      _IOW (LX2162A_PCI_MAGIC, 1, struct bar_register)


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


static int lx2162a_pci_analyzer_open(struct inode* INode, struct file* File)
{
    struct lx2162a_pci_device* device;

    device = container_of(
        INode->i_cdev,
        struct lx2162a_pci_device,
        cdev);

    File->private_data = device;

    return 0;
}


static int lx2162a_pci_analyzer_release(struct inode* INode, struct file* File)
{
    return 0;
}


static long lx2162a_pci_analyzer_ioctl(struct file* File, unsigned int Command, unsigned long Arg)
{
    struct lx2162a_pci_device*  device          = File->private_data;
    struct bar_register         bar_register;

    if (!device)
    {
        return -ENODEV;
    }

    switch (Command)
    {
    case BAR_REGISTER_READ:
        if (copy_from_user(&bar_register, (void __user*)Arg, sizeof(bar_register)))
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

        if (copy_to_user((void __user*)Arg, &bar_register, sizeof(bar_register)))
        {
            return -EFAULT;
        }

        return 0;

    case BAR_REGISTER_WRITE:
        if (copy_from_user(&bar_register, (void __user*)Arg, sizeof(bar_register)))
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


static int lx2162a_pci_analyzer_probe(struct pci_dev* PDev, const struct pci_device_id* Id)
{
    struct lx2162a_pci_device*  device;
    int                         ret_val;

    dev_info(&PDev->dev, "Lx2162aPciAnalyzer: device found: %04x:%04x\n", PDev->vendor, PDev->device);

    ret_val = pci_enable_device(PDev);

    if (ret_val)
    {
        dev_err(&PDev->dev, "pci_enable_device() failed: %d\n", ret_val);
        return ret_val;
    }

    ret_val = pci_request_region(PDev, LX2162A_PCI_BAR, DRIVER_NAME);

    if (ret_val)
    {
        dev_err(&PDev->dev, "pci_request_region() failed: %d\n", ret_val);

        pci_disable_device(PDev);
        return ret_val;
    }

    device = kzalloc(sizeof(*device), GFP_KERNEL);

    if (!device)
    {
        ret_val = -ENOMEM;
        goto err_release_region;
    }

    device->pdev = PDev;

    device->bar_start   = pci_resource_start(PDev, LX2162A_PCI_BAR);
    device->bar_size    = pci_resource_len  (PDev, LX2162A_PCI_BAR);

    dev_info(&PDev->dev, "BAR%d start = 0x%llx\n", LX2162A_PCI_BAR, (unsigned long long)device->bar_start);

    dev_info(&PDev->dev, "BAR%d size  = 0x%llx (%llu bytes)\n", LX2162A_PCI_BAR, (unsigned long long)device->bar_size, (unsigned long long)device->bar_size);

    if (!(pci_resource_flags(PDev, LX2162A_PCI_BAR) & IORESOURCE_MEM))
    {
        dev_err(&PDev->dev, "BAR%d is not a memory BAR\n", LX2162A_PCI_BAR);

        ret_val = -ENODEV;
        goto err_free;
    }

    device->bar = pci_iomap(PDev, LX2162A_PCI_BAR, 0);

    if (!device->bar)
    {
        dev_err(&PDev->dev, "pci_iomap() failed\n");

        ret_val = -ENOMEM;
        goto err_free;
    }

    dev_info(&PDev->dev, "BAR%d mapped at %p\n", LX2162A_PCI_BAR, device->bar);

    ret_val = alloc_chrdev_region(&device->devt, 0, 1, DEVICE_NAME);

    if (ret_val)
    {
        dev_err(&PDev->dev, "alloc_chrdev_region() failed: %d\n", ret_val);
        goto err_unmap;
    }

    cdev_init(&device->cdev, &lx2162a_pci_fops);

    device->cdev.owner = THIS_MODULE;

    ret_val = cdev_add(&device->cdev, device->devt, 1);

    if (ret_val)
    {
        dev_err(&PDev->dev, "cdev_add() failed: %d\n", ret_val);
        goto err_unregister;
    }

    device->device = device_create(lx2162a_pci_class, &PDev->dev, device->devt, device, DEVICE_NAME);

    if (IS_ERR(device->device))
    {
        ret_val = PTR_ERR(device->device);

        dev_err(&PDev->dev, "device_create() failed: %d\n", ret_val);

        goto err_cdev;
    }

    pci_set_drvdata(PDev, device);

    dev_info(&PDev->dev, "Lx2162aPciAnalyzer driver loaded successfully\n");

    return 0;


err_cdev:
    cdev_del(&device->cdev);

err_unregister:
    unregister_chrdev_region(device->devt, 1);

err_unmap:
    pci_iounmap(PDev, device->bar);

err_free:
    kfree(device);

err_release_region:
    pci_release_region(PDev, LX2162A_PCI_BAR);
    pci_disable_device(PDev);

    return ret_val;
}


static void lx2162a_pci_analyzer_remove(struct pci_dev* PDev)
{
    struct lx2162a_pci_device* device;

    device = pci_get_drvdata(PDev);

    if (!device)
    {
        return;
    }

    dev_info(&PDev->dev, "removing Lx2162aPciAnalyzer driver\n");

    device_destroy(lx2162a_pci_class, device->devt);

    cdev_del(&device->cdev);

    unregister_chrdev_region(device->devt, 1);

    if (device->bar)
    {
        pci_iounmap(PDev, device->bar);
    }

    pci_release_region(PDev, LX2162A_PCI_BAR);

    pci_disable_device(PDev);

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
    int ret_val;

    lx2162a_pci_class = class_create(DRIVER_NAME);

    if (IS_ERR(lx2162a_pci_class))
    {
        return PTR_ERR(lx2162a_pci_class);
    }

    ret_val = pci_register_driver(&lx2162a_pci_driver);

    if (ret_val)
    {
        class_destroy(lx2162a_pci_class);
        return ret_val;
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
