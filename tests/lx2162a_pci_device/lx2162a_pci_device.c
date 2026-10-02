
#include "lx2162a_pci_device.h"


static uint32_t config_read(PCIDevice* PciDevice, uint32_t Address, int Length)
{
    uint32_t value = pci_default_read_config(PciDevice, Address, Length);

    if ((_CAPABILITY_ID_ADVANCED_ERROR_REPORTING + _DELTA_ADVANCED_ERROR_REPORTING_CAPABILITY <= Address) && (Address < _CAPABILITY_ID_ADVANCED_ERROR_REPORTING + _DELTA_ADVANCED_ERROR_REPORTING_CAPABILITY + _SIZE_TOTAL))
    {
        printf("config_read  %08x %2d", Address, Length);

        if (Length >= 1)
        {
            printf(" %02x", PciDevice->config[Address + 0u]);
        }

        if (Length >= 2)
        {
            printf(" %02x", PciDevice->config[Address + 1u]);
        }

        if (Length >= 4)
        {
            printf(" %02x", PciDevice->config[Address + 2u]);
            printf(" %02x", PciDevice->config[Address + 3u]);
        }

        printf("\n");
    }

    return value;
}


static void config_write_normal(PCIDevice* PciDevice, uint32_t Address, uint32_t Value, int Length)
{
    if (Length >= 1)
    {
        PciDevice->config[Address - _CAPABILITY_ID_ADVANCED_ERROR_CONTROL + _CAPABILITY_ID_ADVANCED_ERROR_REPORTING + 0u] = (Value & 0x000000ff) >> 0u;
    }

    if (Length >= 2)
    {
        PciDevice->config[Address - _CAPABILITY_ID_ADVANCED_ERROR_CONTROL + _CAPABILITY_ID_ADVANCED_ERROR_REPORTING + 1u] = (Value & 0x0000ff00) >> 8u;
    }

    if (Length >= 4)
    {
        PciDevice->config[Address - _CAPABILITY_ID_ADVANCED_ERROR_CONTROL + _CAPABILITY_ID_ADVANCED_ERROR_REPORTING + 2u] = (Value & 0x00ff0000) >> 16u;
        PciDevice->config[Address - _CAPABILITY_ID_ADVANCED_ERROR_CONTROL + _CAPABILITY_ID_ADVANCED_ERROR_REPORTING + 3u] = (Value & 0xff000000) >> 24u;
    }
}

/*
static void config_write_w1c(PCIDevice* PciDevice, uint32_t Address, uint32_t Value, int Length)
{
    uint32_t value = config_read(PciDevice, Address, Length);

    value = (!Value) & value;

    config_write_normal(PciDevice, Address, Value, Length);
}
*/

static void config_write(PCIDevice* PciDevice, uint32_t Address, uint32_t Value, int Length)
{
    if (((_CAPABILITY_ID_ADVANCED_ERROR_REPORTING + _DELTA_ADVANCED_ERROR_REPORTING_CAPABILITY <= Address) && (Address < _CAPABILITY_ID_ADVANCED_ERROR_REPORTING + _DELTA_ADVANCED_ERROR_REPORTING_CAPABILITY + _SIZE_TOTAL))
    ||  ((_CAPABILITY_ID_ADVANCED_ERROR_CONTROL   + _DELTA_ADVANCED_ERROR_REPORTING_CAPABILITY <= Address) && (Address < _CAPABILITY_ID_ADVANCED_ERROR_CONTROL   + _DELTA_ADVANCED_ERROR_REPORTING_CAPABILITY + _SIZE_TOTAL)))
    {
        printf("config_write %08x %2d", Address, Length);

        if (Length >= 1)
        {
            printf(" %02x", (Value & 0x000000ff) >> 0u);
        }

        if (Length >= 2)
        {
            printf(" %02x", (Value & 0x0000ff00) >> 8u);
        }

        if (Length >= 4)
        {
            printf(" %02x", (Value & 0x00ff0000) >> 16u);
            printf(" %02x", (Value & 0xff000000) >> 24u);
        }

        printf("\n");
    }

    if (Address == _CAPABILITY_ID_ADVANCED_ERROR_REPORTING + _DELTA_UNCORRECTABLE_ERROR_MASK_REGISTER)
    {
        config_write_normal(PciDevice, Address, Value, Length);
    }
    else if (Address == _CAPABILITY_ID_ADVANCED_ERROR_REPORTING + _DELTA_CORRECTABLE_ERROR_MASK_REGISTER)
    {
        config_write_normal(PciDevice, Address, Value, Length);
    }
    else if (Address == _CAPABILITY_ID_ADVANCED_ERROR_REPORTING + _DELTA_ADVANCED_ERROR_CAPABILITIES_AND_CONTROL_REGISTER)
    {
        config_write_normal(PciDevice, Address, Value, Length);
    }
    else if (Address == _CAPABILITY_ID_ADVANCED_ERROR_REPORTING + _DELTA_ROOT_ERROR_COMMAND_REGISTER)
    {
        config_write_normal(PciDevice, Address, Value, Length);
    }
    else if (Address == _CAPABILITY_ID_ADVANCED_ERROR_REPORTING + _DELTA_UNCORRECTABLE_ERROR_STATUS_REGISTER)
    {
//      config_write_w1c(PciDevice, Address, Value, Length);
    }
    else if (Address == _CAPABILITY_ID_ADVANCED_ERROR_REPORTING + _DELTA_CORRECTABLE_ERROR_STATUS_REGISTER)
    {
//      config_write_w1c(PciDevice, Address, Value, Length);
    }
    else if (Address == _CAPABILITY_ID_ADVANCED_ERROR_REPORTING + _DELTA_ROOT_ERROR_STATUS_REGISTER)
    {
//      config_write_w1c(PciDevice, Address, Value, Length);
    }
    else if (Address == _CAPABILITY_ID_ADVANCED_ERROR_REPORTING + _DELTA_LANE_ERROR_STATUS_REGISTER)
    {
//      config_write_w1c(PciDevice, Address, Value, Length);
    }
    else if ((_CAPABILITY_ID_ADVANCED_ERROR_CONTROL + _DELTA_ADVANCED_ERROR_REPORTING_CAPABILITY <= Address) && (Address < _CAPABILITY_ID_ADVANCED_ERROR_CONTROL + _DELTA_ADVANCED_ERROR_REPORTING_CAPABILITY + _SIZE_TOTAL))
    {
        config_write_normal(PciDevice, Address, Value, Length);
    }
}


static void device_init(PCIDevice* PciDevice, Error** Error)
{
    if (pcie_endpoint_cap_init(PciDevice, 0x80) < 0)
    {
        printf("Failed to initialize PCIe endpoint capability\n");
        return;
    }

    pcie_add_capability(PciDevice, _CAPABILITY_ID_ADVANCED_ERROR_REPORTING, VIRTUAL_PCI_DEVICE_REVISION, _CAPABILITY_ID_ADVANCED_ERROR_REPORTING, _SIZE_TOTAL);
    pcie_add_capability(PciDevice, _CAPABILITY_ID_ADVANCED_ERROR_CONTROL,   VIRTUAL_PCI_DEVICE_REVISION, _CAPABILITY_ID_ADVANCED_ERROR_CONTROL,   _SIZE_TOTAL);

    memset(PciDevice->config + _CAPABILITY_ID_ADVANCED_ERROR_REPORTING, 0, _SIZE_TOTAL);

    config_write(PciDevice, _CAPABILITY_ID_ADVANCED_ERROR_CONTROL + _DELTA_ADVANCED_ERROR_REPORTING_CAPABILITY,              _INITIAL_ADVANCED_ERROR_REPORTING_CAPABILITY,              _SIZE_ADVANCED_ERROR_REPORTING_CAPABILITY);
    config_write(PciDevice, _CAPABILITY_ID_ADVANCED_ERROR_CONTROL + _DELTA_UNCORRECTABLE_ERROR_MASK_REGISTER,                _INITIAL_UNCORRECTABLE_ERROR_MASK_REGISTER,                _SIZE_UNCORRECTABLE_ERROR_MASK_REGISTER);
    config_write(PciDevice, _CAPABILITY_ID_ADVANCED_ERROR_CONTROL + _DELTA_CORRECTABLE_ERROR_MASK_REGISTER,                  _INITIAL_CORRECTABLE_ERROR_MASK_REGISTER,                  _SIZE_CORRECTABLE_ERROR_MASK_REGISTER);
    config_write(PciDevice, _CAPABILITY_ID_ADVANCED_ERROR_CONTROL + _DELTA_ADVANCED_ERROR_CAPABILITIES_AND_CONTROL_REGISTER, _INITIAL_ADVANCED_ERROR_CAPABILITIES_AND_CONTROL_REGISTER, _SIZE_ADVANCED_ERROR_CAPABILITIES_AND_CONTROL_REGISTER);
    config_write(PciDevice, _CAPABILITY_ID_ADVANCED_ERROR_CONTROL + _DELTA_ROOT_ERROR_COMMAND_REGISTER,                      _INITIAL_ROOT_ERROR_COMMAND_REGISTER,                      _SIZE_ROOT_ERROR_COMMAND_REGISTER);

    config_write(PciDevice, _CAPABILITY_ID_ADVANCED_ERROR_CONTROL + _DELTA_UNCORRECTABLE_ERROR_STATUS_REGISTER,              _INITIAL_UNCORRECTABLE_ERROR_STATUS_REGISTER,              _SIZE_UNCORRECTABLE_ERROR_STATUS_REGISTER);
    config_write(PciDevice, _CAPABILITY_ID_ADVANCED_ERROR_CONTROL + _DELTA_UNCORRECTABLE_ERROR_SEVERITY_REGISTER,            _INITIAL_UNCORRECTABLE_ERROR_SEVERITY_REGISTER,            _SIZE_UNCORRECTABLE_ERROR_SEVERITY_REGISTER);
    config_write(PciDevice, _CAPABILITY_ID_ADVANCED_ERROR_CONTROL + _DELTA_CORRECTABLE_ERROR_STATUS_REGISTER,                _INITIAL_CORRECTABLE_ERROR_STATUS_REGISTER,                _SIZE_CORRECTABLE_ERROR_STATUS_REGISTER);
    config_write(PciDevice, _CAPABILITY_ID_ADVANCED_ERROR_CONTROL + _DELTA_HEADER_LOG_REGISTER_DWORD1,                       _INITIAL_HEADER_LOG_REGISTER_DWORD1,                       _SIZE_HEADER_LOG_REGISTER_DWORD1);
    config_write(PciDevice, _CAPABILITY_ID_ADVANCED_ERROR_CONTROL + _DELTA_HEADER_LOG_REGISTER_DWORD2,                       _INITIAL_HEADER_LOG_REGISTER_DWORD2,                       _SIZE_HEADER_LOG_REGISTER_DWORD2);
    config_write(PciDevice, _CAPABILITY_ID_ADVANCED_ERROR_CONTROL + _DELTA_HEADER_LOG_REGISTER_DWORD3,                       _INITIAL_HEADER_LOG_REGISTER_DWORD3,                       _SIZE_HEADER_LOG_REGISTER_DWORD3);
    config_write(PciDevice, _CAPABILITY_ID_ADVANCED_ERROR_CONTROL + _DELTA_HEADER_LOG_REGISTER_DWORD4,                       _INITIAL_HEADER_LOG_REGISTER_DWORD4,                       _SIZE_HEADER_LOG_REGISTER_DWORD4);
    config_write(PciDevice, _CAPABILITY_ID_ADVANCED_ERROR_CONTROL + _DELTA_ROOT_ERROR_STATUS_REGISTER,                       _INITIAL_ROOT_ERROR_STATUS_REGISTER,                       _SIZE_ROOT_ERROR_STATUS_REGISTER);
    config_write(PciDevice, _CAPABILITY_ID_ADVANCED_ERROR_CONTROL + _DELTA_CORRECTABLE_ERROR_SOURCE_ID_REGISTER,             _INITIAL_CORRECTABLE_ERROR_SOURCE_ID_REGISTER,             _SIZE_CORRECTABLE_ERROR_SOURCE_ID_REGISTER);
    config_write(PciDevice, _CAPABILITY_ID_ADVANCED_ERROR_CONTROL + _DELTA_ERROR_SOURCE_ID_REGISTER,                         _INITIAL_ERROR_SOURCE_ID_REGISTER,                         _SIZE_ERROR_SOURCE_ID_REGISTER);
    config_write(PciDevice, _CAPABILITY_ID_ADVANCED_ERROR_CONTROL + _DELTA_LANE_ERROR_STATUS_REGISTER,                       _INITIAL_LANE_ERROR_STATUS_REGISTER,                       _SIZE_LANE_ERROR_STATUS_REGISTER);
}


static void class_init(ObjectClass* ObjectClass, const void* ClassData)
{
    DeviceClass*    device_class        = DEVICE_CLASS(ObjectClass);
    PCIDeviceClass* pci_device_class    = PCI_DEVICE_CLASS(ObjectClass);

    pci_device_class->realize           = device_init;
    pci_device_class->vendor_id         = VIRTUAL_PCI_DEVICE_VENDOR_ID;
    pci_device_class->device_id         = VIRTUAL_PCI_DEVICE_DEVICE_ID;
    pci_device_class->revision          = VIRTUAL_PCI_DEVICE_REVISION;
    pci_device_class->class_id          = PCI_CLASS_OTHERS;
    pci_device_class->config_read       = config_read;
    pci_device_class->config_write      = config_write;

    set_bit(DEVICE_CATEGORY_MISC, device_class->categories);

    device_class->desc = DESC_VIRTUAL_PCI_DEVICE;
}


static const TypeInfo type_info =
{
    .name           = TYPE_VIRTUAL_PCI_DEVICE,
    .parent         = TYPE_PCI_DEVICE,
    .instance_size  = sizeof(VirtualPciDevice),
    .class_init     = class_init,
    .interfaces     =
        (InterfaceInfo[]){
            { INTERFACE_PCIE_DEVICE },
            {},
        },
};


static void register_types(void)
{
    type_register_static(&type_info);
}


type_init(register_types)
