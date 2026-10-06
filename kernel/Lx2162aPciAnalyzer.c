
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
#define _VERSION                "1"
#define _LX2162A_PCI_VENDOR_ID  0x1414
#define _LX2162A_PCI_DEVICE_ID  0x00b8
#define _RESOLUTION             1000
#define _ERRORS                 300


struct Lx2162aPciAnalyzerDriver
{
    struct delayed_work DelayedWork;
    struct pci_dev*     PDev;
    struct mutex        Mutex;
    u32                 Errors;
    s32                 Capability;
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
    u32                 RootErrorStatusRegister;            // Critical
    u32                 CorrectableErrorSourceIdRegister;
    u32                 ErrorSourceIdRegister;
    u32                 LaneErrorStatusRegister;            // Critical
};


static struct Lx2162aPciAnalyzerDriver lx2162a_pci_analyzer_driver;


static struct Lx2162aPciAnalyzerValues lx2162a_pci_analyzer_values[_ERRORS];


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
    u32 root_error_status_register_mask                 = 0x0000007f;

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
        &root_error_status_register_value,
        root_error_status_register_address,
        root_error_status_register_size,
        root_error_status_register_mask) < 0)
    {
        pr_err("%s:%d:%s: Failed to read root error status register\n", __FILE__, __LINE__, __func__);
        goto terminate;
    }

    if (lx2162a_pci_analyzer_periodic_reg_read_and_clean(
        &lane_error_status_register_value,
        lane_error_status_register_address,
        lane_error_status_register_size,
        lane_error_status_register_mask) < 0)
    {
        pr_err("%s:%d:%s: Failed to read lane error status register\n", __FILE__, __LINE__, __func__);
        goto terminate;
    }

    if (((uncorrectable_error_status_register_value  & uncorrectable_error_status_register_mask)    == 0u)
     && ((correctable_error_status_register_value    & correctable_error_status_register_mask)      == 0u)
     && ((root_error_status_register_value           & root_error_status_register_mask)             == 0u)
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

    lx2162a_pci_analyzer_values[lx2162a_pci_analyzer_driver.Errors % _ERRORS].Timestamp                             = time_stamp;
    lx2162a_pci_analyzer_values[lx2162a_pci_analyzer_driver.Errors % _ERRORS].UncorrectableErrorStatusRegister      = uncorrectable_error_status_register_value;
    lx2162a_pci_analyzer_values[lx2162a_pci_analyzer_driver.Errors % _ERRORS].UncorrectableErrorSeverityRegister    = uncorrectable_error_severity_register_value;
    lx2162a_pci_analyzer_values[lx2162a_pci_analyzer_driver.Errors % _ERRORS].CorrectableErrorStatusRegister        = correctable_error_status_register_value;
    lx2162a_pci_analyzer_values[lx2162a_pci_analyzer_driver.Errors % _ERRORS].HeaderLogRegisterDword1               = header_log_register_dword1_value;
    lx2162a_pci_analyzer_values[lx2162a_pci_analyzer_driver.Errors % _ERRORS].HeaderLogRegisterDword2               = header_log_register_dword2_value;
    lx2162a_pci_analyzer_values[lx2162a_pci_analyzer_driver.Errors % _ERRORS].HeaderLogRegisterDword3               = header_log_register_dword3_value;
    lx2162a_pci_analyzer_values[lx2162a_pci_analyzer_driver.Errors % _ERRORS].HeaderLogRegisterDword4               = header_log_register_dword4_value;
    lx2162a_pci_analyzer_values[lx2162a_pci_analyzer_driver.Errors % _ERRORS].RootErrorStatusRegister               = root_error_status_register_value;
    lx2162a_pci_analyzer_values[lx2162a_pci_analyzer_driver.Errors % _ERRORS].CorrectableErrorSourceIdRegister      = correctable_error_source_id_register_value;
    lx2162a_pci_analyzer_values[lx2162a_pci_analyzer_driver.Errors % _ERRORS].ErrorSourceIdRegister                 = error_source_id_register_value;
    lx2162a_pci_analyzer_values[lx2162a_pci_analyzer_driver.Errors % _ERRORS].LaneErrorStatusRegister               = lane_error_status_register_value;

    lx2162a_pci_analyzer_driver.Errors++;

terminate:
    mutex_unlock(&lx2162a_pci_analyzer_driver.Mutex);

    schedule_delayed_work(&lx2162a_pci_analyzer_driver.DelayedWork, msecs_to_jiffies(_RESOLUTION));
}


static ssize_t lx2162a_pci_analyzer_read(struct file* File, char __user* Buffer, size_t Size, loff_t* Offset)
{
    ssize_t retval  = 0;
    u32     errors  = 0u;
    char    buffer  [128u];

    mutex_lock(&lx2162a_pci_analyzer_driver.Mutex);

    errors = lx2162a_pci_analyzer_driver.Errors < _ERRORS ? lx2162a_pci_analyzer_driver.Errors : _ERRORS;

    if (!errors)
    {
        goto terminate;
    }

    scnprintf(buffer, sizeof(buffer), "%s\n%u\n", _VERSION, lx2162a_pci_analyzer_driver.Errors);

    if (copy_to_user(Buffer, buffer, strlen(buffer)))
    {
        pr_err("%s:%d:%s: Failed to copy version\n", __FILE__, __LINE__, __func__);
        retval = -EFAULT;
        goto terminate;
    }

    retval += strlen(buffer);
pr_info("01 %zd\n", retval);
    for (u32 i = 0u; i < errors; i++)
    {
        scnprintf(buffer, sizeof(buffer), "%08x.%08x.%08x.%08x.%08x.%08x.%08x.%08x.%08x.%08x.%08x\n",   lx2162a_pci_analyzer_values[i].UncorrectableErrorStatusRegister,
                                                                                                        lx2162a_pci_analyzer_values[i].UncorrectableErrorSeverityRegister,
                                                                                                        lx2162a_pci_analyzer_values[i].CorrectableErrorStatusRegister,
                                                                                                        lx2162a_pci_analyzer_values[i].HeaderLogRegisterDword1,
                                                                                                        lx2162a_pci_analyzer_values[i].HeaderLogRegisterDword2,
                                                                                                        lx2162a_pci_analyzer_values[i].HeaderLogRegisterDword3,
                                                                                                        lx2162a_pci_analyzer_values[i].HeaderLogRegisterDword4,
                                                                                                        lx2162a_pci_analyzer_values[i].RootErrorStatusRegister,
                                                                                                        lx2162a_pci_analyzer_values[i].CorrectableErrorSourceIdRegister,
                                                                                                        lx2162a_pci_analyzer_values[i].ErrorSourceIdRegister,
                                                                                                        lx2162a_pci_analyzer_values[i].LaneErrorStatusRegister);

        if (copy_to_user(&Buffer[retval], buffer, strlen(buffer)))
        {
            pr_err("%s:%d:%s: Failed to copy value\n", __FILE__, __LINE__, __func__);
            retval = -EFAULT;
            goto terminate;
        }

        retval += strlen(buffer);
pr_info("03 %zd\n", retval);
    }

    memset(lx2162a_pci_analyzer_values, 0, sizeof(lx2162a_pci_analyzer_values));

    lx2162a_pci_analyzer_driver.Errors = 0u;

terminate:
    mutex_unlock(&lx2162a_pci_analyzer_driver.Mutex);
pr_info("04 %zd\n", retval);
    return retval;
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
    int retval = 0;

    lx2162a_pci_analyzer_driver.Errors = 0u;

    mutex_init(&lx2162a_pci_analyzer_driver.Mutex);

    lx2162a_pci_analyzer_driver.PDev = pci_get_device(_LX2162A_PCI_VENDOR_ID, _LX2162A_PCI_DEVICE_ID, NULL);

    if (!lx2162a_pci_analyzer_driver.PDev)
    {
        pr_err("%s:%d:%s: Device not found\n", __FILE__, __LINE__, __func__);
        retval = - ENODEV;
        goto terminate;
    }

    lx2162a_pci_analyzer_driver.Capability = pci_find_ext_capability(lx2162a_pci_analyzer_driver.PDev, PCI_EXT_CAP_ID_ERR);

    if (!lx2162a_pci_analyzer_driver.Capability)
    {
        pr_err("%s:%d:%s: Capability not found\n", __FILE__, __LINE__, __func__);
        retval = -ENODEV;
        goto terminate;
    }

    INIT_DELAYED_WORK(&lx2162a_pci_analyzer_driver.DelayedWork, lx2162a_pci_analyzer_periodic_work);

    retval = misc_register(&lx2162a_pci_analyzer_miscdev);

    if (retval)
    {
        pr_err("%s:%d:%s: Failed to register device\n", __FILE__, __LINE__, __func__);
        goto terminate;
    }

    if (lx2162a_pci_analyzer_configure())
    {
        pr_err("%s:%d:%s: Failed to configure device\n", __FILE__, __LINE__, __func__);
        goto terminate;
    }

    schedule_delayed_work(&lx2162a_pci_analyzer_driver.DelayedWork, msecs_to_jiffies(_RESOLUTION));

terminate:
    if (retval)
    {
        pci_dev_put(lx2162a_pci_analyzer_driver.PDev);
    }
    else
    {
        pr_info(_DRIVER_NAME ": (%04x:%04x) loaded\n", lx2162a_pci_analyzer_driver.PDev->vendor, lx2162a_pci_analyzer_driver.PDev->device);
    }

    return retval;
}


static void __exit lx2162a_pci_analyzer_exit(void)
{
    cancel_delayed_work_sync(&lx2162a_pci_analyzer_driver.DelayedWork);

    misc_deregister(&lx2162a_pci_analyzer_miscdev);

    pci_dev_put(lx2162a_pci_analyzer_driver.PDev);

    pr_info(_DRIVER_NAME ": (%04x:%04x) unloaded\n", lx2162a_pci_analyzer_driver.PDev->vendor, lx2162a_pci_analyzer_driver.PDev->device);
}


module_init(lx2162a_pci_analyzer_init);
module_exit(lx2162a_pci_analyzer_exit);


MODULE_LICENSE("GPL");
MODULE_AUTHOR("Andre Kirchner");
MODULE_DESCRIPTION("Monitor SoC PCIe state");
MODULE_VERSION(_VERSION);
