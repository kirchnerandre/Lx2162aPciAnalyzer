
#ifndef LX2162A_DEVICE_H
#define LX2162A_DEVICE_H

#include "qemu/osdep.h"
#include "hw/pci/pci.h"
#include "hw/pci/pci_device.h"


#define VIRTUAL_PCI_DEVICE_CAPABILITIES_OFFSET                     0x0040
#define VIRTUAL_PCI_DEVICE_VENDOR_ID                               0x1414
#define VIRTUAL_PCI_DEVICE_DEVICE_ID                               0x00b9
#define VIRTUAL_PCI_DEVICE_REVISION                                0x01

#define TYPE_VIRTUAL_PCI_DEVICE                                    "lx2162a_pci_device"
#define DESC_VIRTUAL_PCI_DEVICE                                    "lx2162a_pci_device"

// Capabilities
#define _CAPABILITY_ID_ADVANCED_ERROR_REPORTING                     0x0100
#define _CAPABILITY_ID_ADVANCED_ERROR_CONTROL                       0x0200

// Configuration
#define _DELTA_ADVANCED_ERROR_REPORTING_CAPABILITY                  0x0000  // RO
#define _DELTA_UNCORRECTABLE_ERROR_MASK_REGISTER                    0x0008  // RW
#define _DELTA_CORRECTABLE_ERROR_MASK_REGISTER                      0x0014  // RW
#define _DELTA_ADVANCED_ERROR_CAPABILITIES_AND_CONTROL_REGISTER     0x0018  // RW
#define _DELTA_ROOT_ERROR_COMMAND_REGISTER                          0x002C  // RW

#define _SIZE_ADVANCED_ERROR_REPORTING_CAPABILITY                   2
#define _SIZE_UNCORRECTABLE_ERROR_MASK_REGISTER                     4
#define _SIZE_CORRECTABLE_ERROR_MASK_REGISTER                       4
#define _SIZE_ADVANCED_ERROR_CAPABILITIES_AND_CONTROL_REGISTER      4
#define _SIZE_ROOT_ERROR_COMMAND_REGISTER                           4

#define _INITIAL_ADVANCED_ERROR_REPORTING_CAPABILITY                0x0001
#define _INITIAL_UNCORRECTABLE_ERROR_MASK_REGISTER                  0x00000000
#define _INITIAL_CORRECTABLE_ERROR_MASK_REGISTER                    0x00002000
#define _INITIAL_ADVANCED_ERROR_CAPABILITIES_AND_CONTROL_REGISTER   0x000000a0
#define _INITIAL_ROOT_ERROR_COMMAND_REGISTER                        0x00000000

// Errors
#define _DELTA_UNCORRECTABLE_ERROR_STATUS_REGISTER                  0x0004  // W1C
#define _DELTA_UNCORRECTABLE_ERROR_SEVERITY_REGISTER                0x000C  // RO
#define _DELTA_CORRECTABLE_ERROR_STATUS_REGISTER                    0x0010  // W1C
#define _DELTA_HEADER_LOG_REGISTER_DWORD1                           0x001C  // RO
#define _DELTA_HEADER_LOG_REGISTER_DWORD2                           0x0020  // RO
#define _DELTA_HEADER_LOG_REGISTER_DWORD3                           0x0024  // RO
#define _DELTA_HEADER_LOG_REGISTER_DWORD4                           0x0028  // RO
#define _DELTA_ROOT_ERROR_STATUS_REGISTER                           0x0030  // W1C
#define _DELTA_CORRECTABLE_ERROR_SOURCE_ID_REGISTER                 0x0034  // RO
#define _DELTA_ERROR_SOURCE_ID_REGISTER                             0x0036  // RO
#define _DELTA_LANE_ERROR_STATUS_REGISTER                           0x0060  // W1C

#define _SIZE_UNCORRECTABLE_ERROR_STATUS_REGISTER                   4
#define _SIZE_UNCORRECTABLE_ERROR_SEVERITY_REGISTER                 4
#define _SIZE_CORRECTABLE_ERROR_STATUS_REGISTER                     4
#define _SIZE_HEADER_LOG_REGISTER_DWORD1                            4
#define _SIZE_HEADER_LOG_REGISTER_DWORD2                            4
#define _SIZE_HEADER_LOG_REGISTER_DWORD3                            4
#define _SIZE_HEADER_LOG_REGISTER_DWORD4                            4
#define _SIZE_ROOT_ERROR_STATUS_REGISTER                            4
#define _SIZE_CORRECTABLE_ERROR_SOURCE_ID_REGISTER                  2
#define _SIZE_ERROR_SOURCE_ID_REGISTER                              2
#define _SIZE_LANE_ERROR_STATUS_REGISTER                            4

#define _INITIAL_UNCORRECTABLE_ERROR_STATUS_REGISTER                0x00000000
#define _INITIAL_UNCORRECTABLE_ERROR_SEVERITY_REGISTER              0x00862030
#define _INITIAL_CORRECTABLE_ERROR_STATUS_REGISTER                  0x00000000
#define _INITIAL_HEADER_LOG_REGISTER_DWORD1                         0x00000000
#define _INITIAL_HEADER_LOG_REGISTER_DWORD2                         0x00000000
#define _INITIAL_HEADER_LOG_REGISTER_DWORD3                         0x00000000
#define _INITIAL_HEADER_LOG_REGISTER_DWORD4                         0x00000000
#define _INITIAL_ROOT_ERROR_STATUS_REGISTER                         0x00000000
#define _INITIAL_CORRECTABLE_ERROR_SOURCE_ID_REGISTER               0x0000
#define _INITIAL_ERROR_SOURCE_ID_REGISTER                           0x0000
#define _INITIAL_LANE_ERROR_STATUS_REGISTER                         0x00000000

#define _SIZE_TOTAL                                                 (_DELTA_LANE_ERROR_STATUS_REGISTER - _DELTA_ADVANCED_ERROR_REPORTING_CAPABILITY + _SIZE_LANE_ERROR_STATUS_REGISTER)


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
