#!/usr/bin/env python3

import pathlib

import datetime
import sys
import time


def read_Lx2162aPciAnalyzer_data(FilePath):
    try:
        data = open(FilePath, "r")

        if data != "":
            version = int(data.readline())
            errors  = int(data.readline())

            print("f{version} f{errors}")

#           for i in range(errors):
#               values = data.readline().split(".")

#                timestamp_up                            = int(value[0])

#                timestamp_down                          = int(line.split(".")[1])
#                uncorrectable_error_status_register     = int(line.split(".")[2])
#                uncorrectable_error_severity_register   = int(line.split(".")[3])
#                correctable_error_status_register       = int(line.split(".")[4])
#                header_log_register_dword_1             = int(line.split(".")[5])
#                header_log_register_dword_2             = int(line.split(".")[6])
#                header_log_register_dword_3             = int(line.split(".")[7])
#                header_log_register_dword_4             = int(line.split(".")[8])
#                root_error_status_register              = int(line.split(".")[9])
#                correctable_error_source_id_register    = int(line.split(".")[10])
#                error_source_id_register                = int(line.split(".")[11])
#                lane_error_status_register              = int(line.split(".")[12])

#                print(
#                    f"{timestamp_up}.")

#                    f"{timestamp_down}."
#                    f"{uncorrectable_error_status_register:08X}."
#                    f"{uncorrectable_error_severity_register:08X}."
#                    f"{correctable_error_status_register:08X}."
#                    f"{header_log_register_dword_1:08X}."
#                    f"{header_log_register_dword_2:08X}."
#                    f"{header_log_register_dword_3:08X}."
#                    f"{header_log_register_dword_4:08X}."
#                    f"{root_error_status_register:08X}."
#                    f"{correctable_error_source_id_register:08X}."
#                    f"{error_source_id_register:08X}."
#                    f"{lane_error_status_register:08X}.")

        return True
    except Exception as e:
        print(e)
        return False


def main():
    file_path = pathlib.Path("/dev/Lx2162aPciAnalyzer")

    if read_Lx2162aPciAnalyzer_data(file_path) == False:
        print("Failed to read /dev/Lx2162aPciAnalyzer data")
        return -1

    return 0


if __name__ == "__main__":
    main()
