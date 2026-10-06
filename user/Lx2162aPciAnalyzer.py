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
    try:
        data = open(FilePath, "r").read()

        print(data)

        return True
    except:
        return False


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
