#!/usr/bin/env python3


import datetime
import subprocess
import sys
import time


uncorrectable_error_status_register                 = 0x0104        # Error Data
correctable_error_status_register                   = 0x0110        # Error Data
header_log_register_dword1                          = 0x011c        # Error Data
header_log_register_dword2                          = 0x0120        # Error Data
header_log_register_dword3                          = 0x0124        # Error Data
header_log_register_dword4                          = 0x0128        # Error Data
root_error_status_register                          = 0x0130        # Error Data

uncorrectable_error_mask_register                   = 0x0108
uncorrectable_error_severity_register               = 0x010c
correctable_error_mask_register                     = 0x0114
advanced_error_capabilities_and_control_register    = 0x0118
root_error_command_register                         = 0x012c

uncorrectable_error_mask_value                      = 0x001ff010
uncorrectable_error_severity_value                  = 0x001ff010
correctable_error_mask_value                        = 0x000031c1
advanced_error_capabilities_and_control_value       = 0x000001e0    # Error Data bits 5-0
root_error_command_value                            = 0x00000007


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


def verify_lspci_availability():
    try:
        result = subprocess.run(
            ["lspci", "--version"],
            check=True,
            text=True,
            capture_output=True)
    except:
        return False
    else:
        return True


def find_fpga_nic():
    device = None

    try:
        result = subprocess.run(
            ["lspci", "-D"],
            check=True,
            text=True,
            capture_output=True)

        for line in result.stdout.splitlines():
            if "Ethernet controller: Microsoft Corporation Device 00b8" in line:
                device = line.split()[0]
    except:
        return False, None

    return True, device


def read_register(Device, Register, Size):
    value = 0

    try:
        result = subprocess.run(
            ["setpci", "-s " + Device, hex(Register) + "." + Size],
            check=True,
            text=True,
            capture_output=True)

        value = int(result.stdout, 16)
    except:
        return False, None

    return True, value


def set_register(Device, Register, Size, Value):
    try:
        result = subprocess.run(
            ["setpci", "-s " + Device, hex(Register) + "." + Size + "=" + hex(Value)],
            check=True,
            text=True,
            capture_output=True)
    except Except as e:
        print(e)
        return False

    return True


def verify_advanced_error_reporting_capability_availability(Device):
    advanced_error_reporting_capability_id_register     = 0x0100
    advanced_error_reporting_capability_value_expected  = 0x0001

    retval, value = read_register(Device, advanced_error_reporting_capability_id_register, "W")

    if retval == False:
        print("Failed to read register")
        return False

    if value != advanced_error_reporting_capability_value_expected:
        print("Advanced error reporting capability not supported")
        return False

    return True


def configure(Device):
    if set_register(Device, uncorrectable_error_mask_register, "L", uncorrectable_error_mask_value) == False:
        print("Failed to set uncorrectable error mask register")
        return False

    if set_register(Device, uncorrectable_error_severity_register, "L", uncorrectable_error_severity_value) == False:
        print("Failed to set uncorrectable error severity register")
        return False

    if set_register(Device, correctable_error_mask_register, "L", correctable_error_mask_value) == False:
        print("Failed to set uncorrectable error mask register")
        return False

    if set_register(Device, advanced_error_capabilities_and_control_register, "L", advanced_error_capabilities_and_control_value) == False:
        print("Failed to set uncorrectable error severity register")
        return False

    if set_register(Device, root_error_command_register, "L", root_error_command_value) == False:
        print("Failed to set root error command register")
        return False

    return True


def print_data(Device):
    retval, value_uncorrectable_error_status_register = read_register(Device, uncorrectable_error_status_register, "L")

    if retval == False:
        print("Failed to read uncorrectable error status register")
        return false;

    if set_register(Device, uncorrectable_error_status_register, "L", 0) == False:
        print("Failed to clear uncorrectable error status register")
        return false;

    retval, value_correctable_error_status_register = read_register(Device, correctable_error_status_register, "L")

    if retval == False:
        print("Failed to read correctable error status register")
        return false;

    if set_register(Device, correctable_error_status_register, "L", 0) == False:
        print("Failed to clear correctable error status register")
        return false;

    retval, value_header_log_register_dword1 = read_register(Device, header_log_register_dword1, "L")

    if retval == False:
        print("Failed to read header log register dword1")
        return false;

    retval, value_header_log_register_dword2 = read_register(Device, header_log_register_dword2, "L")

    if retval == False:
        print("Failed to read header log register dword2")
        return false;

    retval, value_header_log_register_dword3 = read_register(Device, header_log_register_dword3, "L")

    if retval == False:
        print("Failed to read header log register dword3")
        return false;

    retval, value_header_log_register_dword4 = read_register(Device, header_log_register_dword4, "L")

    if retval == False:
        print("Failed to read header log register dword4")
        return false;

    retval, value_advanced_error_capabilities_and_control_value = read_register(Device, advanced_error_capabilities_and_control_value,  "L")

    if retval == False:
        print("Failed to read advanced error capabilities and control value")
        return false;
""" """
    retval, value_root_error_status_register = read_register(Device, root_error_status_register, "L")

    if retval == False:
        print("Failed to read uncorrectable error status register")
        return false;

    if set_register(Device, root_error_status_register, "L", 0) == False:
        print("Failed to clear uncorrectable error status register")
        return false;
""" """
    print(f" {datetime.datetime.now().time()}"
          f" {value_uncorrectable_error_status_register             :08X}"
          f" {value_correctable_error_status_register               :08X}"
          f" {value_header_log_register_dword1                      :08X}"
          f" {value_header_log_register_dword2                      :08X}"
          f" {value_header_log_register_dword3                      :08X}"
          f" {value_header_log_register_dword4                      :08X}"
          f" {value_advanced_error_capabilities_and_control_value   :08X}"
          f" {value_root_error_status_register                      :08X}")


def main():
    if len(sys.argv) != 2:
        print("Missing parameters")
        return -1

    interval = 0

    try:
        interval = int(sys.argv[1])
    except:
        print("Invalid interval")
        return -1
    finally:
        if interval <= 0:
            print("Invalid interval")
            return -1

    if verify_setpci_availability() == False:
        print("setpci not available")
        return -1

    if verify_lspci_availability() == False:
        print("lspci not available")
        return -1

    retval, device = find_fpga_nic()

    if retval == False:
        print("Failed to find FPGA nic")
        return -1

    if verify_advanced_error_reporting_capability_availability(device) == False:
        print("Capability not available")
        return -1

    if configure(device) == False:
        print("Failed to configure")
        return -1

    try:
        next_run = time.monotonic()

        while True:
            next_run += interval
            time.sleep(max(0, next_run - time.monotonic()))
            print_data(device)
    except:
        pass

    return 0


if __name__ == "__main__":
    main()
