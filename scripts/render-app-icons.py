#!/usr/bin/env python3
"""
Renders application icon PNG assets and Windows ICO from resources/icons/app-color.svg.
Uses g++ with Qt6 (or Python Pillow) to produce pixel-perfect raster icons.
"""

import os
import subprocess
import sys
from PIL import Image

def main():
    repo_root = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
    os.chdir(repo_root)

    cpp_source = "scripts/render-app-icons.cpp"
    bin_target = "/tmp/render-app-icons"

    compile_cmd = [
        "g++", "-std=c++20", cpp_source,
        "-I/usr/include/x86_64-linux-gnu/qt6",
        "-I/usr/include/x86_64-linux-gnu/qt6/QtCore",
        "-I/usr/include/x86_64-linux-gnu/qt6/QtGui",
        "-I/usr/include/x86_64-linux-gnu/qt6/QtSvg",
        "-lQt6Core", "-lQt6Gui", "-lQt6Svg",
        "-o", bin_target
    ]
    subprocess.check_call(compile_cmd)
    subprocess.check_call([bin_target])

    # Windows ICO with 16, 32, 48, 256
    os.makedirs("resources/windows", exist_ok=True)
    img256 = Image.open("resources/icons/app-256.png")
    img256.save(
        "resources/windows/jobprep.ico",
        format="ICO",
        sizes=[(16, 16), (32, 32), (48, 48), (256, 256)]
    )
    print("Generated resources/windows/jobprep.ico with sizes: 16, 32, 48, 256")

if __name__ == "__main__":
    main()
