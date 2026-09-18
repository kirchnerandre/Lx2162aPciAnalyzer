

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


static int bar = 0;

module_param(bar, int, 0444);
MODULE_PARM_DESC(bar, "PCI BAR number to map (default: 0)");


struct lx2162a_bar
{
    struct pci_dev* pdev;
    void __iomem*   base;
    resource_size_t size;
};


static struct lx2162a_bar* devdata;


static ssize_t lx2162a_pci_analyzer_read(struct file* file, char __user* buf, size_t count, loff_t* ppos)
{
    u32 value;
    resource_size_t offset = *ppos;

    if (count != sizeof(value))
    {
        return -EINVAL;
    }

    if (offset & 0x3)
    {
        return -EINVAL;
    }

    if (offset + sizeof(value) > devdata->size)
    {
        return -EINVAL;
    }

    value = ioread32(devdata->base + offset);

    if (copy_to_user(buf, &value, sizeof(value)))
    {
        return -EFAULT;
    }

    *ppos += sizeof(value);

    return sizeof(value);
}


static ssize_t lx2162a_pci_analyzer_write(struct file* file, const char __user* buf, size_t count, loff_t* ppos)
{
    u32 value;
    resource_size_t offset = *ppos;

    if (count != sizeof(value))
    {
        return -EINVAL;
    }

    if (offset & 0x3)
    {
        return -EINVAL;
    }

    if (offset + sizeof(value) > devdata->size)
    {
        return -EINVAL;
    }

    if (copy_from_user(&value, buf, sizeof(value)))
    {
        return -EFAULT;
    }

    iowrite32(value, devdata->base + offset);

    *ppos += sizeof(value);

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
    .mode   = 0600,
};


static int __init lx2162a_pci_analyzer_init(void)
{
    struct pci_dev* pdev;
    unsigned long flags;
    int ret;

    if (bar < 0 || bar >= PCI_STD_NUM_BARS)
    {
        pr_err("%s:%d:%s: Invalid BAR number\n", __FILE__, __LINE__, __func__);
        return -EINVAL;
    }

    pdev = pci_get_device(LX2162A_PCI_VENDOR_ID, LX2162A_PCI_DEVICE_ID, NULL);

    if (!pdev)
    {
        pr_err("%s:%d:%s: Device not found\n", __FILE__, __LINE__, __func__);
        return -ENODEV;
    }

    devdata = kzalloc(sizeof(*devdata), GFP_KERNEL);

    if (!devdata)
    {
        pr_err("%s:%d:%s: Failed to allocate memory\n", __FILE__, __LINE__, __func__);
        pci_dev_put(pdev);
        return -ENOMEM;
    }

    devdata->pdev = pdev;

    pr_info(DRIVER_NAME ": found %04x:%04x at %s\n", pdev->vendor, pdev->device, pci_name(pdev));

    flags           = pci_resource_flags(pdev, bar);
    devdata->size   = pci_resource_len(pdev, bar);

    pr_info(DRIVER_NAME ": BAR%d start=0x%llx size=0x%llx flags=0x%lx\n", bar, (unsigned long long)pci_resource_start(pdev, bar), (unsigned long long)devdata->size, flags);

    if (!(flags & (IORESOURCE_MEM | IORESOURCE_IO)))
    {
        pr_err("%s:%d:%s: Not IO or memory resource\n", __FILE__, __LINE__, __func__);
        ret = -EINVAL;
        goto err_put;
    }

    if (!devdata->size)
    {
        pr_err("%s:%d:%s: BAR register has length zero\n", __FILE__, __LINE__, __func__);
        ret = -EINVAL;
        goto err_put;
    }

    devdata->base = pci_iomap(pdev, bar, 0);

    if (!devdata->base)
    {
        pr_err("%s:%d:%s: Failed to map BAR register\n", __FILE__, __LINE__, __func__);
        ret = -ENOMEM;
        goto err_put;
    }

    ret = misc_register(&lx2162a_pci_analyzer_miscdev);

    if (ret)
    {
        pr_err("%s:%d:%s: Failed to register device\n", __FILE__, __LINE__, __func__);
        pci_iounmap(pdev, devdata->base);
        goto err_put;
    }

    pr_info(DRIVER_NAME ": BAR%d mapped, device available as /dev/%s\n", bar, lx2162a_pci_analyzer_miscdev.name);

    pr_info("%s:%d:%s: Initialized\n", __FILE__, __LINE__, __func__);

    return 0;

err_put:
    kfree(devdata);
    devdata = NULL;
    pci_dev_put(pdev);
    return ret;
}


static void __exit lx2162a_pci_analyzer_exit(void)
{
    if (!devdata)
    {
        return;
    }

    misc_deregister(&lx2162a_pci_analyzer_miscdev);

    if (devdata->base)
    {
        pci_iounmap(devdata->pdev, devdata->base);
    }

    pci_dev_put(devdata->pdev);

    kfree(devdata);
    devdata = NULL;

    pr_info(DRIVER_NAME ": unloaded\n");
}


module_init(lx2162a_pci_analyzer_init);
module_exit(lx2162a_pci_analyzer_exit);


MODULE_LICENSE("GPL");
MODULE_AUTHOR("Andre Kirchner");
MODULE_DESCRIPTION("Monitor SoC PCIe state");
MODULE_VERSION("0.01");
