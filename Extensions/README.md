<div align=center><img src=https://github.com/zynomon/onu/blob/release/Notices/icons/ext.svg>
</div>
# Onu Browser Extension System

## Overview
This guide explains how to build, install, and contribute your own extension for the Onu browser. Follow the steps in order for a smooth setup.

---

## 1. Prerequisites
- Qt6 development libraries  
- CMake 3.16+  
- C++17 compatible compiler  

---

## 2. Setup Development Environment

Clone and build the main branch to get necessary headers:

```bash
git clone https://github.com/zynomon/onu
cd onu
mkdir build && cd build
cmake ..
make
cd ../..
```

---

## 3. Create Your Extension

Navigate to the Extensions folder and create a new directory:

```bash
cd onu/Extensions
mkdir Your_Extension
mkdir Your_Extension/src
```

---

## 4. Configure CMake

Create `Your_Extension/src/CMakeLists.txt`:

```cmake
cmake_minimum_required(VERSION 3.16)

project(YourExtension LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 REQUIRED COMPONENTS Core Svg Widgets WebEngineCore) # required

add_library(YourExtension SHARED
    main.cxx
)

target_link_libraries(YourExtension # required
    Trig::crash  # required
    Qt6::Core  # required
    Qt6::Gui  # required
    Qt6::Widgets  # required
    Qt6::Svg  # required
    Qt6::WebEngineWidgets  # required
    Qt6::WebEngineCore  # required
    Qt6::Network # required
    Qt6::Multimedia # required
    # your extra libraries
)

target_include_directories(YourExtension PRIVATE   # required
    ${CMAKE_CURRENT_SOURCE_DIR}/../..  # required
)  # required

set_target_properties(YourExtension PROPERTIES   # required
    PREFIX ""  # optional
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/.. 
)

```

---

## 5. Write Extension Source files

Create `Your_Extension/src/main.cxx`:

```cpp
#include <QtPlugin>
#include <QString>
#include <QDir>
#include <QPluginLoader>
#include "i.H"

class YourExtension : public QObject, public IExtension {
    Q_OBJECT 
    Q_PLUGIN_METADATA(IID IExtension_iid)
    Q_INTERFACES(IExtension)

public:
    ExtensionMetadata metadata() const override {       // required
        ExtensionMetadata md; 
        md.name = "Your Extension Name";
        md.version = "1.0";
        md.maintainer = "Your Name/Handle";
        md.description = "Brief description";
        md.icon = QIcon();
        return md;  // not returning = error
    }

    bool apply(QMainWindow* window, Browser_Backend* backend) override {
        Q_UNUSED(window);
        Q_UNUSED(backend);
        return true;
    }

    QList<QAction*> getContextMenuActions(QWebEngineView* view) const override {
        return QList<QAction*>();
    }
};

#include "main.moc"
```

---

## 6. Build Your Extension

```bash
cd Your_Extension/src
mkdir build && cd build
cmake ..
make
```

The compiled extension will appear in `Your_Extension/` directory.

---

## 7. Install Extension

Copy the compiled extension to:

- **Linux:** `~/.local/share/Onu/Conf/extensions/`  
- **Windows:** `%APPDATA%/Onu/Conf/extensions/`  
- **macOS:** `~/Library/Application Support/Onu/Conf/extensions/`  

---

## 8. Enable Extension

1. Launch Onu  
2. Press `Ctrl+E` to open Extensions Manager  
3. Find your extension and enable it  

---

## 9. Prepare for Contribution

1. Copy your extension folder to a safe location (e.g., Desktop or Documents).  
2. Clone the assets branch:  

```bash
git clone https://github.com/zynomon/onu --branch assets
cd onu
```

3. Move your copied extension into the `Extensions/` directory of the assets branch.  

---

## 10. File Structure

Ensure your extension follows this structure:

```
assets branch
├── Extensions
│   ├── Your_Extension/
│   │   ├── Compiled_version.so  # could be .dll / .dylib
│   │   ├── README.md            # Documentation
│   │   ├── LICENSE              # optional
│   │   └── src                  # Source code
│   │       ├── CMakeLists.txt
│   │       └── main.cxx
│   └── README.md
└── README.md
```

---

## 11. Submit Pull Request

```bash
cd onu
git checkout assets
git add Extensions/Your_Extension/
git commit -m "Add Your_Extension extension"
git push origin assets
```

Create a pull request on GitHub for the `assets` branch.
