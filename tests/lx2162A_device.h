
#ifndef LX2162A_DEVICE_H
#define LX2162A_DEVICE_H

#include "qemu/osdep.h"
#include "hw/pci/pci.h"
#include "hw/pci/pci_device.h"

#define VIRTUAL_PCI_DEVICE_CAPABILITIES_OFFSET  0x040
#define VIRTUAL_PCI_DEVICE_VENDOR_ID            0x1414
#define VIRTUAL_PCI_DEVICE_DEVICE_ID            0x00b9
#define VIRTUAL_PCI_DEVICE_REVISION             0x01

#define TYPE_VIRTUAL_PCI_DEVICE                 "lx2162A_device"
#define DESC_VIRTUAL_PCI_DEVICE                 "lx2162A_device"

OBJECT_DECLARE_TYPE(VirtualPciDevice, VirtualPciDeviceClass, VIRTUAL_PCI_DEVICE);

typedef struct VirtualPciDevice
{
    PCIDevice pci_dev;
}
VirtualPciDevice;

typedef struct VirtualPciDeviceClass
{
    PCIDeviceClass parent_class;
}
VirtualPciDeviceClass;

#endif /* LX2162A_DEVICE_H */
