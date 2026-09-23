
#include "lx2162a_device.h"


static uint32_t config_read(PCIDevice* PciDevice, uint32_t Address, int Length)
{
    uint32_t value = pci_default_read_config(PciDevice, Address, Length);

    if ((VIRTUAL_PCI_DEVICE_CAPABILITIES_OFFSET <= Address) && (Address <= VIRTUAL_PCI_DEVICE_CAPABILITIES_OFFSET + PCI_PM_SIZEOF))
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
    if ((VIRTUAL_PCI_DEVICE_CAPABILITIES_OFFSET <= Address) && (Address <= VIRTUAL_PCI_DEVICE_CAPABILITIES_OFFSET + PCI_PM_SIZEOF))
    {
        printf("config_write    %p %08x %08x %2d %s *\n", (void*)PciDevice, Address, Value, Length, PciDevice->name);
    }
    else
    {
        printf("config_write    %p %08x %08x %2d %s\n", (void*)PciDevice, Address, Value, Length, PciDevice->name);
    }

    if (Length >= 1)
    {
        PciDevice->config[Address + 0u] = (Value & 0x000000ff) >> 0;
    }

    if (Length >= 2)
    {
        PciDevice->config[Address + 1u] = (Value & 0x0000ff00) >> 8;
    }

    if (Length >= 4)
    {
        PciDevice->config[Address + 2u] = (Value & 0x00ff0000) >> 16;
        PciDevice->config[Address + 3u] = (Value & 0xff000000) >> 24;
    }
}


static void device_init(PCIDevice* PciDevice, Error** Error)
{
    PciDevice->config[PCI_STATUS]                                                       = PCI_STATUS_CAP_LIST;
    PciDevice->config[PCI_CAPABILITY_LIST]                                              = VIRTUAL_PCI_DEVICE_CAPABILITIES_OFFSET;
    PciDevice->config[VIRTUAL_PCI_DEVICE_CAPABILITIES_OFFSET]                           = PCI_CAP_ID_PM;
    PciDevice->config[VIRTUAL_PCI_DEVICE_CAPABILITIES_OFFSET + PCI_CAP_LIST_NEXT]       = 0x00;

    config_write(PciDevice, OFFSET_ADVANCED_ERROR_REPORTING_REPORTING_CAPABILITY,       0x00000001, 2); 
    config_write(PciDevice, OFFSET_UNCORRECTABLE_ERROR_MASK_REGISTER,                   0x00000000, 4);
    config_write(PciDevice, OFFSET_CORRECTABLE_ERROR_MASK_REGISTER,                     0x00002000, 4);
    config_write(PciDevice, OFFSET_ADVANCED_ERROR_CAPABILITIES_AND_CONTROL_REGISTER,    0x000000a0, 4);
    config_write(PciDevice, OFFSET_ROOT_ERROR_COMMAND_REGISTER,                         0x00000000, 4);

    config_write(PciDevice, OFFSET_UNCORRECTABLE_ERROR_STATUS_REGISTER,                 0X00000000, 4);
    config_write(PciDevice, OFFSET_UNCORRECTABLE_ERROR_SEVERITY_REGISTER,               0X00465030, 4);
    config_write(PciDevice, OFFSET_CORRECTABLE_ERROR_STATUS_REGISTER,                   0X00000000, 4);
    config_write(PciDevice, OFFSET_HEADER_LOG_REGISTER_DWORD1,                          0X00000000, 4);
    config_write(PciDevice, OFFSET_HEADER_LOG_REGISTER_DWORD2,                          0X00000000, 4);
    config_write(PciDevice, OFFSET_HEADER_LOG_REGISTER_DWORD3,                          0X00000000, 4);
    config_write(PciDevice, OFFSET_HEADER_LOG_REGISTER_DWORD4,                          0X00000000, 4);
    config_write(PciDevice, OFFSET_ROOT_ERROR_STATUS_REGISTER,                          0X00000000, 4);
    config_write(PciDevice, OFFSET_CORRECTABLE_ERROR_SOURCE_ID_REGISTER,                0X00000000, 2);
    config_write(PciDevice, OFFSET_ERROR_SOURCE_ID_REGISTER,                            0X00000000, 2);
    config_write(PciDevice, OFFSET_LANE_ERROR_STATUS_REGISTER,                          0X00000000, 4);
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

    device_class->desc                  = DESC_VIRTUAL_PCI_DEVICE;
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
