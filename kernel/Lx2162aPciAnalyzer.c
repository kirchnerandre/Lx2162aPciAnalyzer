
#include <linux/fs.h>
#include <linux/io.h>
#include <linux/kernel.h>
#include <linux/miscdevice.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/pci.h>
#include <linux/slab.h>
#include <linux/string.h>
#include <linux/workqueue.h>
#include <linux/uaccess.h>


#define _DRIVER_NAME            "Lx2162aPciAnalyzer"
#define _LX2162A_PCI_VENDOR_ID  0x1414
#define _LX2162A_PCI_DEVICE_ID  0x00b9
#define _RESOLUTION             1000
#define _SIZE                   300


struct Lx2162aPciAnalyzerDriver
{
    struct delayed_work DelayedWork;
    struct pci_dev*     PDev;
    struct mutex        Mutex;
    void __iomem*       Base;
    resource_size_t     Size;
    u32                 Offset;
};


struct Lx2162aPciAnalyzerValues
{
    struct timespec64   Timestamp;
    u32                 UncorrectableErrorStatusRegister;   // Critical
    u32                 UncorrectableErrorSeverityRegister;
    u32                 CorrectableErrorStatusRegister;     // Critical
    u32                 HeaderLogRegisterDword1;
    u32                 HeaderLogRegisterDword2;
    u32                 HeaderLogRegisterDword3;
    u32                 HeaderLogRegisterDword4;
    u32                 RootErrorStatusRegister;
    u32                 CorrectableErrorSourceIdRegister;
    u32                 ErrorSourceIdRegister;
    u32                 LaneErrorStatusRegister;            // Critical
};


static struct Lx2162aPciAnalyzerDriver lx2162a_pci_analyzer_driver;


static struct Lx2162aPciAnalyzerValues lx2162a_pci_analyzer_values[_SIZE];


static int lx2162a_pci_analyzer_periodic_reg_read(u32* Value, loff_t Offset, size_t Size)
{
    int retval = 0;

    if (Size == 16u)
    {
        u16 value = 0u;

        retval = pci_read_config_word(lx2162a_pci_analyzer_driver.PDev, Offset, &value);

        *Value = value;
    }
    else if (Size == 32u)
    {
        retval = pci_read_config_dword(lx2162a_pci_analyzer_driver.PDev, Offset, Value);
    }
    else
    {
        pr_err("%s:%d:%s: Invalid register size\n", __FILE__, __LINE__, __func__);
        return -EINVAL;
    }

    if (retval < 0)
    {
        pr_err("%s:%d:%s: Failed to read register\n", __FILE__, __LINE__, __func__);
        return retval;
    }

    return retval;
}


static int lx2162a_pci_analyzer_periodic_reg_write(u32 Value, loff_t Offset, size_t Size)
{
    int retval = 0;

    if (Size == 16u)
    {
        u16 value = Value;

        retval = pci_write_config_word(lx2162a_pci_analyzer_driver.PDev, Offset, value);
    }
    else if (Size == 32u)
    {
        retval = pci_write_config_dword(lx2162a_pci_analyzer_driver.PDev, Offset, Value);
    }
    else
    {
        pr_err("%s:%d:%s: Invalid register size\n", __FILE__, __LINE__, __func__);
        return -EINVAL;
    }

    if (retval < 0)
    {
        pr_err("%s:%d:%s: Failed to read register\n", __FILE__, __LINE__, __func__);
        return retval;
    }

    return retval;
}


static int lx2162a_pci_analyzer_periodic_reg_read_and_clean(u32* Value, loff_t Offset, size_t Size, u32 Mask)
{
    int retval = 0;

    retval = lx2162a_pci_analyzer_periodic_reg_read(Value, Offset, Size);

    if (retval < 0)
    {
        pr_err("%s:%d:%s: Failed to read register\n", __FILE__, __LINE__, __func__);
        goto terminate;
    }

    retval = lx2162a_pci_analyzer_periodic_reg_write(Mask, Offset, Size);

    if (retval < 0)
    {
        pr_err("%s:%d:%s: Failed to write register\n", __FILE__, __LINE__, __func__);
        goto terminate;
    }

terminate:
    return retval;
}


static int lx2162a_pci_analyzer_configure(void)
{
    int retval                                                      = 0;

    u32 advanced_error_reporting_capability_id_register_address     = 0x0100;
    u32 advanced_error_reporting_capability_id_register_value       = 0u;
    u32 advanced_error_reporting_capability_id_register_size        = 16u;
    u32 advanced_error_reporting_capability_id_register_expected    = 0x00000001;

    u32 uncorrectable_error_mask_register_value                     = 0x001ff010;
    u32 uncorrectable_error_mask_register_address                   = 0x0108;
    u32 uncorrectable_error_mask_register_size                      = 32u;

    u32 correctable_error_mask_register_value                       = 0x000031c1;
    u32 correctable_error_mask_register_address                     = 0x0114;
    u32 correctable_error_mask_register_size                        = 32u;

    u32 advanced_error_capabilities_and_control_register_value      = 0x000001e0;
    u32 advanced_error_capabilities_and_control_register_address    = 0x0118;
    u32 advanced_error_capabilities_and_control_register_size       = 32u;

    u32 root_error_command_register_value                           = 00000007;
    u32 root_error_command_register_address                         = 0x012C;
    u32 root_error_command_register_size                            = 32u;

    retval = lx2162a_pci_analyzer_periodic_reg_read(
        &advanced_error_reporting_capability_id_register_value,
        advanced_error_reporting_capability_id_register_address,
        advanced_error_reporting_capability_id_register_size);

    if (retval < 0)
    {
        pr_err("%s:%d:%s: Failed to read advanced error reporting capability register\n", __FILE__, __LINE__, __func__);
        return retval;
    }

    if (advanced_error_reporting_capability_id_register_value != advanced_error_reporting_capability_id_register_expected)
    {
        pr_err("%s:%d:%s: Advanced error reporting capability not supported\n", __FILE__, __LINE__, __func__);
        return -EINVAL;
    }

    retval = lx2162a_pci_analyzer_periodic_reg_write(
        uncorrectable_error_mask_register_value,
        uncorrectable_error_mask_register_address,
        uncorrectable_error_mask_register_size);

    if (retval < 0)
    {
        pr_err("%s:%d:%s: Failed to set uncorrectable error mask register\n", __FILE__, __LINE__, __func__);
        return retval;
    }

    retval = lx2162a_pci_analyzer_periodic_reg_write(
        correctable_error_mask_register_value,
        correctable_error_mask_register_address,
        correctable_error_mask_register_size);

    if (retval < 0)
    {
        pr_err("%s:%d:%s: Failed to set correctable error mask register\n", __FILE__, __LINE__, __func__);
        return retval;
    }

    retval = lx2162a_pci_analyzer_periodic_reg_write(
        advanced_error_capabilities_and_control_register_address,
        advanced_error_capabilities_and_control_register_value,
        advanced_error_capabilities_and_control_register_size);

    if (retval < 0)
    {
        pr_err("%s:%d:%s: Failed to set advanced error capabilities and control register\n", __FILE__, __LINE__, __func__);
        return retval;
    }

    retval = lx2162a_pci_analyzer_periodic_reg_write(
        root_error_command_register_address,
        root_error_command_register_value,
        root_error_command_register_size);

    if (retval < 0)
    {
        pr_err("%s:%d:%s: Failed to set advanced error capabilities and control register\n", __FILE__, __LINE__, __func__);
        return retval;
    }

    return 0;
}


static void lx2162a_pci_analyzer_periodic_work(struct work_struct* DelayedWork)
{
    // W1C
    u32 uncorrectable_error_status_register_value       = 0u;
    u32 uncorrectable_error_status_register_address     = 0x0104;
    u32 uncorrectable_error_status_register_size        = 32u;
    u32 uncorrectable_error_status_register_mask        = 0x001ff010;

    // RO
    u32 uncorrectable_error_severity_register_value     = 0u;
    u32 uncorrectable_error_severity_register_address   = 0x010C;
    u32 uncorrectable_error_severity_register_size      = 32u;

    // W1C
    u32 correctable_error_status_register_value         = 0u;
    u32 correctable_error_status_register_address       = 0x0110;
    u32 correctable_error_status_register_size          = 32u;
    u32 correctable_error_status_register_mask          = 0x000031c1;

    // RO
    u32 header_log_register_dword1_value                = 0u;
    u32 header_log_register_dword1_address              = 0x011C;
    u32 header_log_register_dword1_size                 = 32u;

    // RO
    u32 header_log_register_dword2_value                = 0u;
    u32 header_log_register_dword2_address              = 0x0120;
    u32 header_log_register_dword2_size                 = 32u;

    // RO
    u32 header_log_register_dword3_value                = 0u;
    u32 header_log_register_dword3_address              = 0x0124;
    u32 header_log_register_dword3_size                 = 32u;

    // RO
    u32 header_log_register_dword4_value                = 0u;
    u32 header_log_register_dword4_address              = 0x0128;
    u32 header_log_register_dword4_size                 = 32u;

    // W1C
    u32 root_error_status_register_value                = 0u;
    u32 root_error_status_register_address              = 0x0130;
    u32 root_error_status_register_size                 = 32u;

    // RO
    u32 correctable_error_source_id_register_value      = 0u;
    u32 correctable_error_source_id_register_address    = 0x0134;
    u32 correctable_error_source_id_register_size       = 16u;

    // RO
    u32 error_source_id_register_value                  = 0u;
    u32 error_source_id_register_address                = 0x0136;
    u32 error_source_id_register_size                   = 16u;

    // W1C
    u32 lane_error_status_register_value                = 0u;
    u32 lane_error_status_register_address              = 0x0160;
    u32 lane_error_status_register_size                 = 32u;
    u32 lane_error_status_register_mask                 = 0x000000ff;

    struct timespec64 time_stamp;

    mutex_lock(&lx2162a_pci_analyzer_driver.Mutex);

    ktime_get_real_ts64(&time_stamp);

    if (lx2162a_pci_analyzer_periodic_reg_read_and_clean(
            &uncorrectable_error_status_register_value,
            uncorrectable_error_status_register_address,
            uncorrectable_error_status_register_size,
            uncorrectable_error_status_register_mask) < 0)
    {
        pr_err("%s:%d:%s: Failed to read uncorrectable error status register\n", __FILE__, __LINE__, __func__);
        goto terminate;
    }

    if (lx2162a_pci_analyzer_periodic_reg_read_and_clean(
        &correctable_error_status_register_value,
        correctable_error_status_register_address,
        correctable_error_status_register_size,
        correctable_error_status_register_mask) < 0)
    {
        pr_err("%s:%d:%s: Failed to read correctable error status register\n", __FILE__, __LINE__, __func__);
        goto terminate;
    }

    if (lx2162a_pci_analyzer_periodic_reg_read_and_clean(
        &lane_error_status_register_value,
        lane_error_status_register_address,
        lane_error_status_register_size,
        lane_error_status_register_mask) < 0)
    {
        pr_err("%s:%d:%s: Failed to read lane_error status register\n", __FILE__, __LINE__, __func__);
        goto terminate;
    }

    if (((uncorrectable_error_status_register_value  & uncorrectable_error_status_register_mask)    == 0u)
     && ((correctable_error_status_register_value    & correctable_error_status_register_mask)      == 0u)
     && ((lane_error_status_register_value           & lane_error_status_register_mask)             == 0u))
    {
        pr_info(_DRIVER_NAME " %lld.%09ld No errors\n", (long long)time_stamp.tv_sec, time_stamp.tv_nsec);
        goto terminate;
    }
    else
    {
        pr_info(_DRIVER_NAME " %lld.%09ld Has errors\n", (long long)time_stamp.tv_sec, time_stamp.tv_nsec);
    }

    if (lx2162a_pci_analyzer_periodic_reg_read(
        &uncorrectable_error_severity_register_value,
        uncorrectable_error_severity_register_address,
        uncorrectable_error_severity_register_size) < 0)
    {
        pr_err("%s:%d:%s: Failed to read uncorrectable error severity register\n", __FILE__, __LINE__, __func__);
        goto terminate;
    }

    if (lx2162a_pci_analyzer_periodic_reg_read(
        &header_log_register_dword1_value,
        header_log_register_dword1_address,
        header_log_register_dword1_size) < 0)
    {
        pr_err("%s:%d:%s: Failed to read header log register dword1\n", __FILE__, __LINE__, __func__);
        goto terminate;
    }

    if (lx2162a_pci_analyzer_periodic_reg_read(
        &header_log_register_dword2_value,
        header_log_register_dword2_address,
        header_log_register_dword2_size) < 0)
    {
        pr_err("%s:%d:%s: Failed to read header log register dword2\n", __FILE__, __LINE__, __func__);
        goto terminate;
    }

    if (lx2162a_pci_analyzer_periodic_reg_read(
        &header_log_register_dword3_value,
        header_log_register_dword3_address,
        header_log_register_dword3_size) < 0)
    {
        pr_err("%s:%d:%s: Failed to read header log register dword3\n", __FILE__, __LINE__, __func__);
        goto terminate;
    }

    if (lx2162a_pci_analyzer_periodic_reg_read(
        &header_log_register_dword4_value,
        header_log_register_dword4_address,
        header_log_register_dword4_size) < 0)
    {
        pr_err("%s:%d:%s: Failed to read header log register dword4\n", __FILE__, __LINE__, __func__);
        goto terminate;
    }

    if (lx2162a_pci_analyzer_periodic_reg_read(
        &root_error_status_register_value,
        root_error_status_register_address,
        root_error_status_register_size) < 0)
    {
        pr_err("%s:%d:%s: Failed to read root error status register\n", __FILE__, __LINE__, __func__);
        goto terminate;
    }

    if (lx2162a_pci_analyzer_periodic_reg_read(
        &correctable_error_source_id_register_value,
        correctable_error_source_id_register_address,
        correctable_error_source_id_register_size) < 0)
    {
        pr_err("%s:%d:%s: Failed to read correctable error source id register\n", __FILE__, __LINE__, __func__);
        goto terminate;
    }

    if (lx2162a_pci_analyzer_periodic_reg_read(
        &error_source_id_register_value,
        error_source_id_register_address,
        error_source_id_register_size) < 0)
    {
        pr_err("%s:%d:%s: Failed to read error source id register\n", __FILE__, __LINE__, __func__);
        goto terminate;
    }

    lx2162a_pci_analyzer_values[lx2162a_pci_analyzer_driver.Offset++ % _SIZE].Timestamp                             = time_stamp;
    lx2162a_pci_analyzer_values[lx2162a_pci_analyzer_driver.Offset++ % _SIZE].UncorrectableErrorStatusRegister      = uncorrectable_error_status_register_value;
    lx2162a_pci_analyzer_values[lx2162a_pci_analyzer_driver.Offset++ % _SIZE].UncorrectableErrorSeverityRegister    = uncorrectable_error_severity_register_value;
    lx2162a_pci_analyzer_values[lx2162a_pci_analyzer_driver.Offset++ % _SIZE].CorrectableErrorStatusRegister        = correctable_error_status_register_value;
    lx2162a_pci_analyzer_values[lx2162a_pci_analyzer_driver.Offset++ % _SIZE].HeaderLogRegisterDword1               = header_log_register_dword1_value;
    lx2162a_pci_analyzer_values[lx2162a_pci_analyzer_driver.Offset++ % _SIZE].HeaderLogRegisterDword2               = header_log_register_dword2_value;
    lx2162a_pci_analyzer_values[lx2162a_pci_analyzer_driver.Offset++ % _SIZE].HeaderLogRegisterDword3               = header_log_register_dword3_value;
    lx2162a_pci_analyzer_values[lx2162a_pci_analyzer_driver.Offset++ % _SIZE].HeaderLogRegisterDword4               = header_log_register_dword4_value;
    lx2162a_pci_analyzer_values[lx2162a_pci_analyzer_driver.Offset++ % _SIZE].RootErrorStatusRegister               = root_error_status_register_value;
    lx2162a_pci_analyzer_values[lx2162a_pci_analyzer_driver.Offset++ % _SIZE].CorrectableErrorSourceIdRegister      = correctable_error_source_id_register_value;
    lx2162a_pci_analyzer_values[lx2162a_pci_analyzer_driver.Offset++ % _SIZE].ErrorSourceIdRegister                 = error_source_id_register_value;
    lx2162a_pci_analyzer_values[lx2162a_pci_analyzer_driver.Offset++ % _SIZE].LaneErrorStatusRegister               = lane_error_status_register_value;

    lx2162a_pci_analyzer_driver.Offset++;

terminate:
    mutex_unlock(&lx2162a_pci_analyzer_driver.Mutex);

    schedule_delayed_work(&lx2162a_pci_analyzer_driver.DelayedWork, msecs_to_jiffies(_RESOLUTION));
}


static ssize_t lx2162a_pci_analyzer_read(struct file* File, char __user* Buffer, size_t Size, loff_t* Offset)
{
    u32 offset  = 0u;
    u32 size    = 0u;
    u32 version = 1u;

    mutex_lock(&lx2162a_pci_analyzer_driver.Mutex);

    if (copy_to_user(&Buffer[offset], &version, sizeof(version)))
    {
        mutex_unlock(&lx2162a_pci_analyzer_driver.Mutex);
        pr_err("%s:%d:%s: Failed to copy version\n", __FILE__, __LINE__, __func__);
        return -EFAULT;
    }

    offset += sizeof(version);

    size = _SIZE < lx2162a_pci_analyzer_driver.Offset ? _SIZE : lx2162a_pci_analyzer_driver.Offset;

    if (copy_to_user(&Buffer[offset], &size, sizeof(size)))
    {
        mutex_unlock(&lx2162a_pci_analyzer_driver.Mutex);
        pr_err("%s:%d:%s: Failed to copy size\n", __FILE__, __LINE__, __func__);
        return -EFAULT;
    }

    offset += sizeof(size);

    if (copy_to_user(&Buffer[offset], lx2162a_pci_analyzer_values, sizeof(lx2162a_pci_analyzer_values)))
    {
        mutex_unlock(&lx2162a_pci_analyzer_driver.Mutex);
        pr_err("%s:%d:%s: Failed to copy data\n", __FILE__, __LINE__, __func__);
        return -EFAULT;
    }

    memset(lx2162a_pci_analyzer_values, 0, sizeof(lx2162a_pci_analyzer_values));

    lx2162a_pci_analyzer_driver.Offset = 0u;

    mutex_unlock(&lx2162a_pci_analyzer_driver.Mutex);

    return 0;
}


static const struct file_operations lx2162a_pci_analyzer_fops =
{
    .owner  = THIS_MODULE,
    .read   = lx2162a_pci_analyzer_read,
};


static struct miscdevice lx2162a_pci_analyzer_miscdev =
{
    .minor  = MISC_DYNAMIC_MINOR,
    .name   = _DRIVER_NAME,
    .fops   = &lx2162a_pci_analyzer_fops,
    .mode   = 0444,
};


static int __init lx2162a_pci_analyzer_init(void)
{
    unsigned long   flags   = 0u;
    int             retval  = 0;

    lx2162a_pci_analyzer_driver.Offset = 0u;

    mutex_init(&lx2162a_pci_analyzer_driver.Mutex);

    lx2162a_pci_analyzer_driver.PDev = pci_get_device(_LX2162A_PCI_VENDOR_ID, _LX2162A_PCI_DEVICE_ID, NULL);

    if (!lx2162a_pci_analyzer_driver.PDev)
    {
        pr_err("%s:%d:%s: Device not found\n", __FILE__, __LINE__, __func__);
        return -ENODEV;
    }

    flags                               = pci_resource_flags(lx2162a_pci_analyzer_driver.PDev, 0);
    lx2162a_pci_analyzer_driver.Size    = pci_resource_len  (lx2162a_pci_analyzer_driver.PDev, 0);

    if (!(flags & (IORESOURCE_MEM | IORESOURCE_IO)))
    {
        pr_err("%s:%d:%s: Not IO or memory resource\n", __FILE__, __LINE__, __func__);
        retval = -EINVAL;
        goto terminate;
    }

    if (!lx2162a_pci_analyzer_driver.Size)
    {
        pr_err("%s:%d:%s: BAR register has length zero\n", __FILE__, __LINE__, __func__);
        retval = -EINVAL;
        goto terminate;
    }

    lx2162a_pci_analyzer_driver.Base = pci_iomap(lx2162a_pci_analyzer_driver.PDev, 0, 0);

    if (!lx2162a_pci_analyzer_driver.Base)
    {
        pr_err("%s:%d:%s: Failed to map BAR register\n", __FILE__, __LINE__, __func__);
        retval = -ENOMEM;
        goto terminate;
    }

    INIT_DELAYED_WORK(&lx2162a_pci_analyzer_driver.DelayedWork, lx2162a_pci_analyzer_periodic_work);

    retval = misc_register(&lx2162a_pci_analyzer_miscdev);

    if (retval)
    {
        pr_err("%s:%d:%s: Failed to register device\n", __FILE__, __LINE__, __func__);
        pci_iounmap(lx2162a_pci_analyzer_driver.PDev, lx2162a_pci_analyzer_driver.Base);
        goto terminate;
    }

    if (lx2162a_pci_analyzer_configure())
    {
        pr_err("%s:%d:%s: Failed to configure device\n", __FILE__, __LINE__, __func__);
        pci_iounmap(lx2162a_pci_analyzer_driver.PDev, lx2162a_pci_analyzer_driver.Base);
        goto terminate;
    }

    schedule_delayed_work(&lx2162a_pci_analyzer_driver.DelayedWork, msecs_to_jiffies(_RESOLUTION));

    pr_info(
        _DRIVER_NAME ": (%04x:%04x) start=0x%llx Size=0x%llx flags=0x%lx\n",
        lx2162a_pci_analyzer_driver.PDev->vendor,
        lx2162a_pci_analyzer_driver.PDev->device,
        (unsigned long long)pci_resource_start(lx2162a_pci_analyzer_driver.PDev, 0),
        (unsigned long long)lx2162a_pci_analyzer_driver.Size, flags);

terminate:
    if (retval)
    {
        pci_dev_put(lx2162a_pci_analyzer_driver.PDev);
    }

    return retval;
}


static void __exit lx2162a_pci_analyzer_exit(void)
{
    cancel_delayed_work_sync(&lx2162a_pci_analyzer_driver.DelayedWork);

    misc_deregister(&lx2162a_pci_analyzer_miscdev);

    if (lx2162a_pci_analyzer_driver.Base)
    {
        pci_iounmap(lx2162a_pci_analyzer_driver.PDev, lx2162a_pci_analyzer_driver.Base);
    }

    pci_dev_put(lx2162a_pci_analyzer_driver.PDev);
}


module_init(lx2162a_pci_analyzer_init);
module_exit(lx2162a_pci_analyzer_exit);


MODULE_LICENSE("GPL");
MODULE_AUTHOR("Andre Kirchner");
MODULE_DESCRIPTION("Monitor SoC PCIe state");
MODULE_VERSION("1");
