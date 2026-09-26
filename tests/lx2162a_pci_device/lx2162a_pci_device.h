
#ifndef LX2162A_DEVICE_H
#define LX2162A_DEVICE_H

#include "qemu/osdep.h"
#include "hw/pci/pci.h"
#include "hw/pci/pci_device.h"

#define VIRTUAL_PCI_DEVICE_CAPABILITIES_OFFSET                  0x0040
#define VIRTUAL_PCI_DEVICE_VENDOR_ID                            0x1414
#define VIRTUAL_PCI_DEVICE_DEVICE_ID                            0x00b9
#define VIRTUAL_PCI_DEVICE_REVISION                             0x01

#define TYPE_VIRTUAL_PCI_DEVICE                                 "lx2162a_pci_device"
#define DESC_VIRTUAL_PCI_DEVICE                                 "lx2162a_pci_device"

// Configuration
#define OFFSET_ADVANCED_ERROR_REPORTING_REPORTING_CAPABILITY    0x0100
#define OFFSET_UNCORRECTABLE_ERROR_MASK_REGISTER                0x0108
#define OFFSET_CORRECTABLE_ERROR_MASK_REGISTER                  0x0114
#define OFFSET_ADVANCED_ERROR_CAPABILITIES_AND_CONTROL_REGISTER 0x0118
#define OFFSET_ROOT_ERROR_COMMAND_REGISTER                      0x012C

// Errors
#define OFFSET_UNCORRECTABLE_ERROR_STATUS_REGISTER              0x0104  // W1C
#define OFFSET_UNCORRECTABLE_ERROR_SEVERITY_REGISTER            0x010C  // RO
#define OFFSET_CORRECTABLE_ERROR_STATUS_REGISTER                0x0110  // W1C
#define OFFSET_HEADER_LOG_REGISTER_DWORD1                       0x011C  // RO
#define OFFSET_HEADER_LOG_REGISTER_DWORD2                       0x0120  // RO
#define OFFSET_HEADER_LOG_REGISTER_DWORD3                       0x0124  // RO
#define OFFSET_HEADER_LOG_REGISTER_DWORD4                       0x0128  // RO
#define OFFSET_ROOT_ERROR_STATUS_REGISTER                       0x0130  // W1C
#define OFFSET_CORRECTABLE_ERROR_SOURCE_ID_REGISTER             0x0134  // RO
#define OFFSET_ERROR_SOURCE_ID_REGISTER                         0x0136  // RO
#define OFFSET_LANE_ERROR_STATUS_REGISTER                       0x0160  // W1C

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
