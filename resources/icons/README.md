# Application Icon Assets

This directory contains the application icon source files and rendered assets.

- `app-color.svg`: Original full-color application icon (rounded squircle with gradient accent, briefcase glyph, and checkmark clasp).
- `app.svg`: Monochrome sidebar glyph (remains untouched).
- `app-16.png` .. `app-256.png`: Rasterized PNG assets at 16, 24, 32, 48, 64, 128, and 256 px.
- `../../resources/windows/jobprep.ico`: Windows icon containing 16, 32, 48, and 256 px sizes.

## Reproducing the Icon Assets

To regenerate all raster PNGs and the Windows `.ico` file from `resources/icons/app-color.svg`:

```bash
# 1. Render PNG assets (16, 24, 32, 48, 64, 128, 256) using Qt6 SVG renderer:
g++ -std=c++20 scripts/render-app-icons.cpp \
    -I/usr/include/x86_64-linux-gnu/qt6 \
    -I/usr/include/x86_64-linux-gnu/qt6/QtCore \
    -I/usr/include/x86_64-linux-gnu/qt6/QtGui \
    -I/usr/include/x86_64-linux-gnu/qt6/QtSvg \
    -lQt6Core -lQt6Gui -lQt6Svg \
    -o /tmp/render-icons && /tmp/render-icons

# 2. Render Windows multi-resolution ICO (16, 32, 48, 256) using Pillow:
python3 -c "
from PIL import Image
img256 = Image.open('resources/icons/app-256.png')
img256.save('resources/windows/jobprep.ico', format='ICO', sizes=[(16, 16), (32, 32), (48, 48), (256, 256)])
"

# Or execute the all-in-one helper script:
python3 scripts/render-app-icons.py
```
