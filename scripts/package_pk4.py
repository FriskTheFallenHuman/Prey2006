#!/usr/bin/env python3
import zipfile
import os
import sys

def main():
    dll_name = os.environ.get("DLL_NAME")
    if not dll_name:
        sys.exit("DLL_NAME environment variable not set")

    with zipfile.ZipFile("game01.pk4", "w", compression=zipfile.ZIP_STORED) as z:
        z.write("binary.conf")
        z.write(dll_name)

if __name__ == "__main__":
    main()
