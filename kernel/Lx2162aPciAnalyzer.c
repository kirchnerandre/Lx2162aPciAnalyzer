
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
    struct pci_dev*     PDev;
    void __iomem*       Bar;
    resource_size_t     BarStart;
    resource_size_t     BarSize;
    dev_t               DevT;
    struct cdev         CDev;
    struct device*      Device;
};


static struct class* lx2162a_pci_class;


static int lx2162a_pci_analyzer_open(struct inode* INode, struct file* File)
{
    struct lx2162a_pci_device* device;

    device = container_of(
        INode->i_cdev,
        struct lx2162a_pci_device,
        CDev);

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

        if ((resource_size_t)bar_register.offset + sizeof(u32) > device->BarSize)
        {
            return -EINVAL;
        }

        bar_register.value = ioread32(device->Bar + bar_register.offset);

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

        if ((resource_size_t)bar_register.offset + sizeof(u32) > device->BarSize)
        {
            return -EINVAL;
        }

        iowrite32(bar_register.value, device->Bar + bar_register.offset);

        readl(device->Bar + bar_register.offset);

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
    int                         retval;

    dev_info(&PDev->dev, "Lx2162aPciAnalyzer: device found: %04x:%04x\n", PDev->vendor, PDev->device);

    retval = pci_enable_device(PDev);

    if (retval)
    {
        dev_err(&PDev->dev, "pci_enable_device() failed: %d\n", retval);
        return retval;
    }

    retval = pci_request_region(PDev, LX2162A_PCI_BAR, DRIVER_NAME);

    if (retval)
    {
        dev_err(&PDev->dev, "pci_request_region() failed: %d\n", retval);

        pci_disable_device(PDev);
        return retval;
    }

    device = kzalloc(sizeof(*device), GFP_KERNEL);

    if (!device)
    {
        retval = -ENOMEM;
        goto err_release_region;
    }

    device->PDev = PDev;

    device->BarStart    = pci_resource_start(PDev, LX2162A_PCI_BAR);
    device->BarSize     = pci_resource_len  (PDev, LX2162A_PCI_BAR);

    dev_info(&PDev->dev, "BAR%d start = 0x%llx\n", LX2162A_PCI_BAR, (unsigned long long)device->BarStart);

    dev_info(&PDev->dev, "BAR%d size  = 0x%llx (%llu bytes)\n", LX2162A_PCI_BAR, (unsigned long long)device->BarSize, (unsigned long long)device->BarSize);

    if (!(pci_resource_flags(PDev, LX2162A_PCI_BAR) & IORESOURCE_MEM))
    {
        dev_err(&PDev->dev, "BAR%d is not a memory BAR\n", LX2162A_PCI_BAR);

        retval = -ENODEV;
        goto err_free;
    }

    device->Bar = pci_iomap(PDev, LX2162A_PCI_BAR, 0);

    if (!device->Bar)
    {
        dev_err(&PDev->dev, "pci_iomap() failed\n");

        retval = -ENOMEM;
        goto err_free;
    }

    dev_info(&PDev->dev, "BAR%d mapped at %p\n", LX2162A_PCI_BAR, device->Bar);

    retval = alloc_chrdev_region(&device->DevT, 0, 1, DEVICE_NAME);

    if (retval)
    {
        dev_err(&PDev->dev, "alloc_chrdev_region() failed: %d\n", retval);
        goto err_unmap;
    }

    cdev_init(&device->CDev, &lx2162a_pci_fops);

    device->CDev.owner = THIS_MODULE;

    retval = cdev_add(&device->CDev, device->DevT, 1);

    if (retval)
    {
        dev_err(&PDev->dev, "cdev_add() failed: %d\n", retval);
        goto err_unregister;
    }

    device->Device = device_create(lx2162a_pci_class, &PDev->dev, device->DevT, device, DEVICE_NAME);

    if (IS_ERR(device->Device))
    {
        retval = PTR_ERR(device->Device);

        dev_err(&PDev->dev, "device_create() failed: %d\n", retval);

        goto err_cdev;
    }

    pci_set_drvdata(PDev, device);

    dev_info(&PDev->dev, "Lx2162aPciAnalyzer driver loaded successfully\n");

    return 0;

err_cdev:
    cdev_del(&device->CDev);

err_unregister:
    unregister_chrdev_region(device->DevT, 1);

err_unmap:
    pci_iounmap(PDev, device->Bar);

err_free:
    kfree(device);

err_release_region:
    pci_release_region(PDev, LX2162A_PCI_BAR);
    pci_disable_device(PDev);

    return retval;
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

    device_destroy(lx2162a_pci_class, device->DevT);

    cdev_del(&device->CDev);

    unregister_chrdev_region(device->DevT, 1);

    if (device->Bar)
    {
        pci_iounmap(PDev, device->Bar);
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
    int retval;

    lx2162a_pci_class = class_create(DRIVER_NAME);

    if (IS_ERR(lx2162a_pci_class))
    {
        return PTR_ERR(lx2162a_pci_class);
    }

    retval = pci_register_driver(&lx2162a_pci_driver);

    if (retval)
    {
        class_destroy(lx2162a_pci_class);
        return retval;
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
