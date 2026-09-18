
#include <linux/fs.h>
#include <linux/io.h>
#include <linux/kernel.h>
#include <linux/miscdevice.h>
#include <linux/module.h>
#include <linux/pci.h>
#include <linux/slab.h>
#include <linux/uaccess.h>


#define DRIVER_NAME             "Lx2162aPciAnalyzer"
#define LX2162A_PCI_VENDOR_ID   0x1414
#define LX2162A_PCI_DEVICE_ID   0x00b9


struct Lx2162aPciAnalyzerData
{
    struct pci_dev* pdev;
    void __iomem*   base;
    resource_size_t size;
};


static struct Lx2162aPciAnalyzerData* lx2162a_pci_analyzer_data;


static ssize_t lx2162a_pci_analyzer_read(struct file* File, char __user* Buffer, size_t Size, loff_t* Offset)
{
    u32             value   = 0;
    resource_size_t offset  = *Offset;

    if (Size != sizeof(value))
    {
        return -EINVAL;
    }

    if (offset & 0x3)
    {
        return -EINVAL;
    }

    if (offset + sizeof(value) > lx2162a_pci_analyzer_data->size)
    {
        return -EINVAL;
    }

    value = ioread32(lx2162a_pci_analyzer_data->base + offset);

    if (copy_to_user(Buffer, &value, sizeof(value)))
    {
        return -EFAULT;
    }

    *Offset += sizeof(value);

    return sizeof(value);
}


static ssize_t lx2162a_pci_analyzer_write(struct file* File, const char __user* Buffer, size_t Size, loff_t* Offset)
{
    u32             value   = 0;
    resource_size_t offset  = *Offset;

    if (Size != sizeof(value))
    {
        return -EINVAL;
    }

    if (offset & 0x3)
    {
        return -EINVAL;
    }

    if (offset + sizeof(value) > lx2162a_pci_analyzer_data->size)
    {
        return -EINVAL;
    }

    if (copy_from_user(&value, Buffer, sizeof(value)))
    {
        return -EFAULT;
    }

    iowrite32(value, lx2162a_pci_analyzer_data->base + offset);

    *Offset += sizeof(value);

    return sizeof(value);
}


static const struct file_operations lx2162a_pci_analyzer_fops =
{
    .owner  = THIS_MODULE,
    .read   = lx2162a_pci_analyzer_read,
    .write  = lx2162a_pci_analyzer_write,
    .llseek = default_llseek,
};


static struct miscdevice lx2162a_pci_analyzer_miscdev =
{
    .minor  = MISC_DYNAMIC_MINOR,
    .name   = "Lx2162aPciAnalyzer",
    .fops   = &lx2162a_pci_analyzer_fops,
    .mode   = 0444,
};


static int __init lx2162a_pci_analyzer_init(void)
{
    struct pci_dev* pdev    = NULL;
    unsigned long   flags   = 0;
    int             bar     = 0;
    int             retval  = 0;

    pdev = pci_get_device(LX2162A_PCI_VENDOR_ID, LX2162A_PCI_DEVICE_ID, NULL);

    if (!pdev)
    {
        pr_err("%s:%d:%s: Device not found\n", __FILE__, __LINE__, __func__);
        return -ENODEV;
    }

    lx2162a_pci_analyzer_data = kzalloc(sizeof(*lx2162a_pci_analyzer_data), GFP_KERNEL);

    if (!lx2162a_pci_analyzer_data)
    {
        pr_err("%s:%d:%s: Failed to allocate memory\n", __FILE__, __LINE__, __func__);
        pci_dev_put(pdev);
        return -ENOMEM;
    }

    lx2162a_pci_analyzer_data->pdev     = pdev;

    flags                               = pci_resource_flags(pdev, bar);
    lx2162a_pci_analyzer_data->size     = pci_resource_len  (pdev, bar);

    if (!(flags & (IORESOURCE_MEM | IORESOURCE_IO)))
    {
        pr_err("%s:%d:%s: Not IO or memory resource\n", __FILE__, __LINE__, __func__);
        retval = -EINVAL;
        goto err_put;
    }

    if (!lx2162a_pci_analyzer_data->size)
    {
        pr_err("%s:%d:%s: BAR register has length zero\n", __FILE__, __LINE__, __func__);
        retval = -EINVAL;
        goto err_put;
    }

    lx2162a_pci_analyzer_data->base = pci_iomap(pdev, bar, 0);

    if (!lx2162a_pci_analyzer_data->base)
    {
        pr_err("%s:%d:%s: Failed to map BAR register\n", __FILE__, __LINE__, __func__);
        retval = -ENOMEM;
        goto err_put;
    }

    retval = misc_register(&lx2162a_pci_analyzer_miscdev);

    if (retval)
    {
        pr_err("%s:%d:%s: Failed to register device\n", __FILE__, __LINE__, __func__);
        pci_iounmap(pdev, lx2162a_pci_analyzer_data->base);
        goto err_put;
    }

    pr_info(DRIVER_NAME ": (%04x:%04x) BAR%d start=0x%llx size=0x%llx flags=0x%lx\n", pdev->vendor, pdev->device, bar, (unsigned long long)pci_resource_start(pdev, bar), (unsigned long long)lx2162a_pci_analyzer_data->size, flags);

err_put:
    if (retval)
    {
        kfree(lx2162a_pci_analyzer_data);
        lx2162a_pci_analyzer_data = NULL;
        pci_dev_put(pdev);
    }

    return retval;
}


static void __exit lx2162a_pci_analyzer_exit(void)
{
    if (!lx2162a_pci_analyzer_data)
    {
        pr_err("%s:%d:%s: Driver was not loaded\n", __FILE__, __LINE__, __func__);
        return;
    }

    misc_deregister(&lx2162a_pci_analyzer_miscdev);

    if (lx2162a_pci_analyzer_data->base)
    {
        pci_iounmap(lx2162a_pci_analyzer_data->pdev, lx2162a_pci_analyzer_data->base);
    }

    pci_dev_put(lx2162a_pci_analyzer_data->pdev);

    kfree(lx2162a_pci_analyzer_data);

    lx2162a_pci_analyzer_data = NULL;
}


module_init(lx2162a_pci_analyzer_init);
module_exit(lx2162a_pci_analyzer_exit);


MODULE_LICENSE("GPL");
MODULE_AUTHOR("Andre Kirchner");
MODULE_DESCRIPTION("Monitor SoC PCIe state");
MODULE_VERSION("0.01");
