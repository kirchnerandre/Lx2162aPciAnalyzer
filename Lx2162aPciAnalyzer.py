#!/usr/bin/env python3


'''
PCIe1 base address: 340_0000h
PCIe3 base address: 360_0000h
PCIe4 base address: 370_0000h

******
Common
******

100h PCI Express Advanced Error Reporting Capability ID Register    (Advanced_Error_Reporting_Capability_ID_Register)   16 RO  0001h
104h PCI Express Uncorrectable Error Status Register                (Uncorrectable_Error_Status_Register)               32 W1C 0000_0000h
108h PCI Express Uncorrectable Error Mask Register                  (Uncorrectable_Error_Mask_Register)                 32 RW  0000_0000h
10Ch PCI Express Uncorrectable Error Severity Register              (Uncorrectable_Error_Severity_Register)             32 RW  0046_2030h
110h PCI Express Correctable Error Status Register                  (Correctable_Error_Status_Register)                 32 W1C 0000_0000h
114h PCI Express Correctable Error Mask Register                    (Correctable_Error_Mask_Register)                   32 RW  0000_2000h
118h PCI Express Advanced Error Capabilities and Control Register   (Advanced_Error_Capabilities_and_Control_Register)  32 RW  0000_00A0h
12Ch PCI Express Root Error Command Register                        (Root_Error_Command_Register)                       32 RW  0000_0000h
130h PCI Express Root Error Status Register                         (Root_Error_Status_Register)                        32 W1C 0000_0000h
134h PCI Express Correctable Error Source ID Register               (Correctable_Error_Source_ID_Register)              16 RO  0000h
136h PCI Express Error Source ID Register                           (Error_Source_ID_Register)                          16 RO  0000h

11Ch PCI Express Header Log Register 1                              (Header_Log_Register_DWORD1)                        32 RO  0000_0000h
120h PCI Express Header Log Register 2                              (Header_Log_Register_DWORD2)                        32 RO  0000_0000h
124h PCI Express Header Log Register 3                              (Header_Log_Register_DWORD3)                        32 RO  0000_0000h
128h PCI Express Header Log Register 4                              (Header_Log_Register_DWORD4)                        32 RO  0000_0000h

***************
PCIe1 and PCIe4
***************

150h Lane Error Status Register                                     (LANE_ERR_STATUS_REG)                               32 W1C 0000_0000h

*****
PCIe3
*****

160h Lane Error Status Register                                     (LANE_ERR_STATUS_REG)                               32 W1C 0000_0000h

'''


import subprocess


_fpga_device_address                                        = "5582:00:00.0"    # TODO: Get right address

_address_Advanced_Error_Reporting_Capability_ID_Register    = 0x100
_address_Uncorrectable_Error_Status_Register                = 0x104
_address_Uncorrectable_Error_Mask_Register                  = 0x108
_address_Uncorrectable_Error_Severity_Register              = 0x10C
_address_Correctable_Error_Status_Register                  = 0x110
_address_Correctable_Error_Mask_Register                    = 0x114
_address_Advanced_Error_Capabilities_and_Control_Register   = 0x118
_address_Root_Error_Command_Register                        = 0x12C
_address_Root_Error_Status_Register                         = 0x130
_address_Correctable_Error_Source_ID_Register               = 0x134
_address_Error_Source_ID_Register                           = 0x136


def verify_setpci_availability():
    try:
        result = subprocess.run(
            ["setpci", "--version"],
            check=True,
            text=True,
            capture_output=True)
    except:
        return False
    else:
        return True


def verify_advanced_error_reporting_capability_availability():
    result = subprocess.run(
        ["setpci", "-s 5582:00:00.0", "100.W"],
        check=True,
        text=True,
        capture_output=True)

    print(result.stderr)

    return True


def configure_masks():
    pass


def main():
    if verify_setpci_availability() == False:
        print("setpci not available")
        return -1

    if verify_advanced_error_reporting_capability_availability() == False:
        print("Advanced Error Reporting Capability not available")
        return -1

    return 0


if __name__ == "__main__":
    main()
