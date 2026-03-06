# QR CODE Extension by Onu Team
<img width="1372" height="740" alt="image" src="https://github.com/user-attachments/assets/144bd926-4cde-434c-b394-ee9ef0f5d7a1" />

## Overview
QR code extension that adds QR code generation capabilities to Onu browser. Available in two versions: Basic and Advanced.

Supported OS : GNU/Linux
Variants : Basic, Advanced
Sizes :
 Basic : 74.6 KB  (QrCode-Basic.so)
 Advanced : 1.85 MB (QrCode.so)
Supported Onu versions : 0.5 , 0.5.9

---

## Dependencies
```bash
# Install dependency first
sudo apt install libqrencode   # Debian/Ubuntu
```
```bash
sudo pacman -S qrencode        # Arch
```
```bash
sudo dnf install qrencode-devel # Fedora
```
## Keyboard Shortcut
- **Ctrl+Shift+Q**: Generate QR from current page (Advanced version)


# Choose either Basic or Advanced version
But remember basic uses less space and advanced uses more
eg  64kb vs 124 kb difference.


---
<img width="288" height="342" alt="image" src="https://github.com/user-attachments/assets/88380a49-7ccc-4a06-b722-36056e9767fd" />
<img width="411" height="120" alt="image" src="https://github.com/user-attachments/assets/8dd9837b-5d72-4c59-a3de-f3a970d23ca2" />

both version offers same (left click) Context menu, and also an icon in navbar
## QR-CODE Basic
*Simple QR code generator for everyday use*

### Features:
- **Core Functionality**
  - Generate QR codes from current page URL
  - Generate QR codes from selected text/link (context menu)
  - Copy URL to clipboard

- **Export Options**
  - Save as PNG
  - Save as SVG (vector format)
  - Real-time preview

- **Basic Customizations**
  - Foreground color picker
  - Background color picker
  - QR code size adjustment (150-800px)
  - Margin control (0-50px)

- **Error Correction Levels**
  - L (Low) - 7% recovery
  - M (Medium) - 15% recovery  
  - Q (Quartile) - 25% recovery
  - H (High) - 30% recovery

- **Simple UI**
  - Clean, minimal interface
  - Live preview
  - One-click save

---

## QR-CODE (Advanced)
*Feature-rich QR generator with extensive customization*

### Everything in Basic, PLUS:

- **Advanced Visual Styles**
  - Dot Styles: Square, Rounded, Circle, Diamond
  - Eye Styles: Square, Rounded, Circle (finder patterns)
  - Corner Radius Control: 0-50% for rounded dots
  - Gradient Support: Two-color gradients with angle control (0-360°)

- **Logo Embedding**
  - Center logo placement
  - Frame Styles: None, Circle, Square, Rounded Square
  - Adjustable logo size (20-150px)
  - Padding control (0-20px)
  - Optional white background behind logo
  - Auto-crop logos to square
  - Live logo preview

- **Text Frame**
  - Add custom text below QR code
  - Customizable frame color
  - Adjustable frame thickness (1-10px)
  - Auto-sizing frame

- **Enhanced UI**
  - Scrollable settings panel
  - Live preview updates
  - Color pickers with contrast-aware text
  - Settings grouped by category

---

## Quick Comparison

| Feature | Basic | Advanced |
|---------|-------|----------|
| URL/Text to QR | ✓ | ✓ |
| PNG Export | ✓ | ✓ |
| SVG Export | ✓ | ✓ |
| Basic Colors | ✓ | ✓ |
| Custom Dot Shapes | - | ✓ |
| Custom Eye Shapes | - | ✓ |
| Gradients | - | ✓ |
| Logo Embedding | - | ✓ |
| Text Frames | - | ✓ |
| Corner Radius | - | ✓ |
| Finder Pattern Detection | - | ✓ |
| Live Statistics | - | ✓ |
| Settings Preview | ✓ | ✓ |

---

## Source code;
We have also provided the source code for this qr code extension, just clone the mainbranch and create a directory in that folder , and add this to your main cmake ``add_subdirectory(`folder path where you have the cmakelists with main.cxx of qr code`) ``

For building from source you will need;
Debian
```bash
sudo apt install libqrencode-dev pkg-config
```

RPM (Fedora/rhel)
```bash
sudo dnf install qrencode-devel pkgconfig
```

FreeBSD
```bash
pkg install graphics/libqrencode pkgconf
```

Manual build
```bash
sudo apt install pkg-config autoconf automake libtool libpng-dev
git clone https://github.com/fukuchi/libqrencode.git
cd libqrencode
./autogen.sh
./configure
make
sudo make install
```

- Add this to the cmakelists, when you are building.
```make
find_package(PkgConfig REQUIRED) && pkg_check_modules(QRENCODE REQUIRED libqrencode)
```
