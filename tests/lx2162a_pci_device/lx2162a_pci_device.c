
#include "lx2162a_pci_device.h"


static uint32_t config_read(PCIDevice* PciDevice, uint32_t Address, int Length)
{
    uint32_t value = pci_default_read_config(PciDevice, Address, Length);

    if (Address >= 0x100)
    {
        printf("config_read     %p %08x %08x %2d %s *\n", (void*)PciDevice, Address, value, Length, PciDevice->name);
    }
    else
    {
        printf("config_read     %p %08x %08x %2d %s\n", (void*)PciDevice, Address, value, Length, PciDevice->name);
    }

    return value;
}


static void config_write(PCIDevice* PciDevice, uint32_t Address, uint32_t Value, int Length)
{
    if (((_CAPABILITY_ID_ADVANCED_ERROR_REPORTING + _DELTA_ADVANCED_ERROR_REPORTING_CAPABILITY <= Address) && (Address < _CAPABILITY_ID_ADVANCED_ERROR_REPORTING + _DELTA_ADVANCED_ERROR_REPORTING_CAPABILITY + _SIZE_TOTAL))
    ||  ((_CAPABILITY_ID_ADVANCED_ERROR_CONTROL   + _DELTA_ADVANCED_ERROR_REPORTING_CAPABILITY <= Address) && (Address < _CAPABILITY_ID_ADVANCED_ERROR_CONTROL   + _DELTA_ADVANCED_ERROR_REPORTING_CAPABILITY + _SIZE_TOTAL)))
    {
        printf("config_write %08x %2d", Address, Length);

        uint8_t value = 0;

        if (Length >= 1)
        {
            value = (Value & 0x000000ff) >> 0;
            PciDevice->config[Address + 0u] = value;
            printf(" %02x", value);
        }

        if (Length >= 2)
        {
            value = (Value & 0x0000ff00) >> 8;
            PciDevice->config[Address + 1u] = value;
            printf(" %02x", value);
        }

        if (Length >= 4)
        {
            value = (Value & 0x00ff0000) >> 16;
            PciDevice->config[Address + 2u] = value;
            printf(" %02x", value);

            value = (Value & 0xff000000) >> 24;
            PciDevice->config[Address + 3u] = value;
            printf(" %02x", value);
        }

        printf("\n");
    }
}


static void device_init(PCIDevice* PciDevice, Error** Error)
{
    if (pcie_endpoint_cap_init(PciDevice, 0x80) < 0)
    {
        printf("Failed to initialize PCIe endpoint capability\n");
        return;
    }

    pcie_add_capability(PciDevice, _CAPABILITY_ID_ADVANCED_ERROR_REPORTING, VIRTUAL_PCI_DEVICE_REVISION, _CAPABILITY_ID_ADVANCED_ERROR_REPORTING + _DELTA_ADVANCED_ERROR_REPORTING_CAPABILITY, _SIZE_TOTAL);
    pcie_add_capability(PciDevice, _CAPABILITY_ID_ADVANCED_ERROR_CONTROL,   VIRTUAL_PCI_DEVICE_REVISION, _CAPABILITY_ID_ADVANCED_ERROR_CONTROL   + _DELTA_ADVANCED_ERROR_REPORTING_CAPABILITY, _SIZE_TOTAL);

    config_write(PciDevice, _CAPABILITY_ID_ADVANCED_ERROR_REPORTING + _DELTA_ADVANCED_ERROR_REPORTING_CAPABILITY,              _INITIAL_ADVANCED_ERROR_REPORTING_CAPABILITY,              _SIZE_ADVANCED_ERROR_REPORTING_CAPABILITY);
    config_write(PciDevice, _CAPABILITY_ID_ADVANCED_ERROR_REPORTING + _DELTA_UNCORRECTABLE_ERROR_MASK_REGISTER,                _INITIAL_UNCORRECTABLE_ERROR_MASK_REGISTER,                _SIZE_UNCORRECTABLE_ERROR_MASK_REGISTER);
    config_write(PciDevice, _CAPABILITY_ID_ADVANCED_ERROR_REPORTING + _DELTA_CORRECTABLE_ERROR_MASK_REGISTER,                  _INITIAL_CORRECTABLE_ERROR_MASK_REGISTER,                  _SIZE_CORRECTABLE_ERROR_MASK_REGISTER);
    config_write(PciDevice, _CAPABILITY_ID_ADVANCED_ERROR_REPORTING + _DELTA_ADVANCED_ERROR_CAPABILITIES_AND_CONTROL_REGISTER, _INITIAL_ADVANCED_ERROR_CAPABILITIES_AND_CONTROL_REGISTER, _SIZE_ADVANCED_ERROR_CAPABILITIES_AND_CONTROL_REGISTER);
    config_write(PciDevice, _CAPABILITY_ID_ADVANCED_ERROR_REPORTING + _DELTA_ROOT_ERROR_COMMAND_REGISTER,                      _INITIAL_ROOT_ERROR_COMMAND_REGISTER,                      _SIZE_ROOT_ERROR_COMMAND_REGISTER);

    config_write(PciDevice, _CAPABILITY_ID_ADVANCED_ERROR_REPORTING + _DELTA_UNCORRECTABLE_ERROR_STATUS_REGISTER,              _INITIAL_UNCORRECTABLE_ERROR_STATUS_REGISTER,              _SIZE_UNCORRECTABLE_ERROR_STATUS_REGISTER);
    config_write(PciDevice, _CAPABILITY_ID_ADVANCED_ERROR_REPORTING + _DELTA_UNCORRECTABLE_ERROR_SEVERITY_REGISTER,            _INITIAL_UNCORRECTABLE_ERROR_SEVERITY_REGISTER,            _SIZE_UNCORRECTABLE_ERROR_SEVERITY_REGISTER);
    config_write(PciDevice, _CAPABILITY_ID_ADVANCED_ERROR_REPORTING + _DELTA_CORRECTABLE_ERROR_STATUS_REGISTER,                _INITIAL_CORRECTABLE_ERROR_STATUS_REGISTER,                _SIZE_CORRECTABLE_ERROR_STATUS_REGISTER);
    config_write(PciDevice, _CAPABILITY_ID_ADVANCED_ERROR_REPORTING + _DELTA_HEADER_LOG_REGISTER_DWORD1,                       _INITIAL_HEADER_LOG_REGISTER_DWORD1,                       _SIZE_HEADER_LOG_REGISTER_DWORD1);
    config_write(PciDevice, _CAPABILITY_ID_ADVANCED_ERROR_REPORTING + _DELTA_HEADER_LOG_REGISTER_DWORD2,                       _INITIAL_HEADER_LOG_REGISTER_DWORD2,                       _SIZE_HEADER_LOG_REGISTER_DWORD2);
    config_write(PciDevice, _CAPABILITY_ID_ADVANCED_ERROR_REPORTING + _DELTA_HEADER_LOG_REGISTER_DWORD3,                       _INITIAL_HEADER_LOG_REGISTER_DWORD3,                       _SIZE_HEADER_LOG_REGISTER_DWORD3);
    config_write(PciDevice, _CAPABILITY_ID_ADVANCED_ERROR_REPORTING + _DELTA_HEADER_LOG_REGISTER_DWORD4,                       _INITIAL_HEADER_LOG_REGISTER_DWORD4,                       _SIZE_HEADER_LOG_REGISTER_DWORD4);
    config_write(PciDevice, _CAPABILITY_ID_ADVANCED_ERROR_REPORTING + _DELTA_ROOT_ERROR_STATUS_REGISTER,                       _INITIAL_ROOT_ERROR_STATUS_REGISTER,                       _SIZE_ROOT_ERROR_STATUS_REGISTER);
    config_write(PciDevice, _CAPABILITY_ID_ADVANCED_ERROR_REPORTING + _DELTA_CORRECTABLE_ERROR_SOURCE_ID_REGISTER,             _INITIAL_CORRECTABLE_ERROR_SOURCE_ID_REGISTER,             _SIZE_CORRECTABLE_ERROR_SOURCE_ID_REGISTER);
    config_write(PciDevice, _CAPABILITY_ID_ADVANCED_ERROR_REPORTING + _DELTA_ERROR_SOURCE_ID_REGISTER,                         _INITIAL_ERROR_SOURCE_ID_REGISTER,                         _SIZE_ERROR_SOURCE_ID_REGISTER);
    config_write(PciDevice, _CAPABILITY_ID_ADVANCED_ERROR_REPORTING + _DELTA_LANE_ERROR_STATUS_REGISTER,                       _INITIAL_LANE_ERROR_STATUS_REGISTER,                       _SIZE_LANE_ERROR_STATUS_REGISTER);
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


static const TypeInfo type_info = {
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
