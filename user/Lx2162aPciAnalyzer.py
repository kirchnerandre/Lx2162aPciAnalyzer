#!/usr/bin/env python3

import pathlib

import datetime
import sys
import time


def verify_Lx2162aPciAnalyzer_exists(FilePath):
    if FilePath.exists():
        return True
    else:
        return False


def read_Lx2162aPciAnalyzer_data(FilePath):
    with open(FilePath, "rb") as file:
        binary_data = file.read()

    print(f"Successfully read {len(binary_data)} bytes.")

    print("First 20 bytes:", binary_data[:20])

    return True


def main():
    file_path = pathlib.Path("/dev/Lx2162aPciAnalyzer")

    if verify_Lx2162aPciAnalyzer_exists(file_path) == False:
        print("File /dev/Lx2162aPciAnalyzer missing")
        return -1

    if read_Lx2162aPciAnalyzer_data(file_path) == False:
        print("Failed to read /dev/Lx2162aPciAnalyzer data")
        return -1

    return 0


if __name__ == "__main__":
    main()
