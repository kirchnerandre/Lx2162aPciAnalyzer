#!/usr/bin/env python3

import subprocess


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


def main():
    if verify_setpci_availability() == False:
        print("setpci not available")
        return -1


if __name__ == "__main__":
    main()
