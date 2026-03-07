
<p align="center">
  <img width="1440" height="900" alt="Onu." src="https://github.com/user-attachments/assets/843ab11f-3471-4288-924d-4601a64d81b3">
  <img src="https://img.shields.io/badge/Status-Beta-orange?style=flat-square">
  <img src="https://img.shields.io/badge/License-Apache%202.0-green?style=flat-square">
  <img src="https://img.shields.io/badge/Platform-Linux-green?style=flat-square">
  <a href="https://github.com/zynomon/onu"><img src="https://img.shields.io/github/stars/zynomon/onu?style=social"></a>
  <a href="https://github.com/zynomon/onu/fork"><img src="https://img.shields.io/github/forks/zynomon/onu?style=social"></a>
</p>

**Onu.** is a small tiny browser. Its main goal is to fit in any low storage devices, be fast, and have every aspect customizable. A Qt6 Web browser that uses Qt6 WebEngine at its core, requiring less space with better resource management, while providing unique features with free customizations. It's a browser that users own, not what maintainers own. Zero telemetry logging and zero cloud usage.
Its made for powerusers customizers and beginners ( frankly ).

---

<h1> <img src="https://github.com/zynomon/onu/raw/release/Notices/icons/pkg.svg" height="48" width="48"> Download</h1>

if your desktop environment is KDE ( linux ) Most packages are under 1MB . If you're on GNOME or something else, dependencies might take up to 200MB.

# Before starting;
Onu browser version 0.5.9 was ended too soon next version is coming in few months, so we had made an issue that is not embedding the libtrigonometry inside the packages 

## Trigonometry Library

Version 0.5 onward uses a library called Trigonometry for crash handling. It's available in the error.os repository, but since most people don't know what that is:
do this;
```bash
wget https://github.com/zynomon/libtrigonometry/raw/main/Release%20files/libcrash.so
sudo mv libcrash.so /usr/share/trigonometry/
```

If you're on Debian and want the easy route ( Few commands to install it real quick):
```bash
wget https://github.com/zynomon/onu/blob/release/beta/onu-0.5.9.deb
sudo dpkg -i onu-0.5.9.deb
sudo apt install -f
wget https://github.com/zynomon/libtrigonometry/raw/main/Release%20files/libcrash.so
sudo mv libcrash.so /usr/share/trigonometry/
```


**Latest releases:**
- 🔗 **[Download Onu 0.5.9 AppImage](https://github.com/zynomon/onu/raw/release/beta/onu-0.5.9.AppImage)** (96MB due to embedded libraries, also you dont need libtrigonometry in this version along with Qt libraries)
- 🔗 **[Download Onu 0.5.9 (.deb)](https://github.com/zynomon/onu/blob/release/beta/onu-0.5.9.deb)**
- 🔗 **[Download Onu 0.5.9 (.rpm)](https://github.com/zynomon/onu/blob/release/beta/onu-0.5.9.rpm)**
- 🔗 **[Download Onu 0.5 (.deb)](https://github.com/zynomon/onu/blob/release/beta/onu-0.5.deb)**
- 🔗 **[Download Onu 0.5 (.rpm)](https://github.com/zynomon/onu/blob/release/beta/onu-0.5.rpm)**

<details>
<summary><b>Older versions</b></summary>

- 🔗 **[Onu 0.4 (.deb)](https://github.com/zynomon/onu/blob/release/beta/onu-0.4.deb)**
- 🔗 **[Onu 0.3 (.deb)](https://github.com/zynomon/onu/blob/release/beta/onu-0.3.deb)**
- 🔗 **[Onu 0.2 (.deb)](https://github.com/zynomon/onu/blob/release/beta/onu-0.2.deb)**
- 🔗 **[Onu 0.1 (.deb)](https://github.com/zynomon/onu/blob/release/beta/onu-0.1.deb)**
- 🔗 **[Source code archives](https://github.com/zynomon/onu/blob/release/)**

</details>


---

## 🔧 Building from Source

```bash
# Install Qt6 and build tools
sudo apt install qt6-base-dev qt6-webengine-dev qt6-multimedia-dev cmake build-essential

# Install trigonometric library (needed for extensions and crash handling)
wget https://github.com/zynomon/libtrigonometry/raw/main/Release%20files/libtrigonometry-1.0.0-Linux.deb
sudo dpkg -i libtrigonometry-1.0.0-Linux.deb
sudo apt install -f
# ONLY FOR DEBIAN LIKE AND DEBIAN LINUX
```

If you use other distros, check https://github.com/zynomon/libtrigonometry and compile it for your OS instead of installing that .deb.

```bash
# Clone the repository
git clone https://github.com/zynomon/onu
cd onu

# Create build directory
mkdir -p build && cd build

# Configure with CMake (install to ~/.local)
cmake .. -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=~/.local

# Build and install
make -j$(nproc) && make install
```

If you face any issues while compiling, feel free to open an issue.
And as for forking check this [#Forking](#creating-a-fork-very-easily)

---
<h1> <img width="727" height="243" alt="image" src="https://github.com/user-attachments/assets/5c30347d-cf96-4a54-ab4f-51ce0ae482e9" /> Toolbars </h1>

Toolbars are Qt widgets for desktop OS(es) that can be dragged, docked, or left floating freely without recompiling the browser. If you don't like toolbars dragging or floating, left click on the drag handle to customize how you want.
![e](https://github.com/user-attachments/assets/b6b9df3e-83db-47f8-b06e-85696e8f83f6)

If you hid all toolbars, don't worry - just press `Ctrl+Shift+,` and customize them from here:

<img width="895" height="413" alt="image" src="https://github.com/user-attachments/assets/aa5f5088-48d4-441b-beb3-06448d5f4b50" />

Here's an introduction to each bar:

## URL bar
URL bar (also called "Search bar") contains the search function. You can change the search engine or add your own - it's easy once you try it.

<img width="549" height="96" alt="image" src="https://github.com/user-attachments/assets/ca75428e-3e23-437d-b107-109f9c1fb3ed" />

It supports basic autocomplete from history and favorites.

<img width="848" height="641" alt="image" src="https://github.com/user-attachments/assets/d9ed38b6-ca09-46ed-b810-60da837b1238" />

(API parser issues are being worked on.)

## Fav bar
Favorites bar shows your favorite sites as shortcuts. Dragging links into it will save them as new favorites. Customize your favorites in the Favorites Manager.

## Nav bar
<img width="492" height="311" alt="image" src="https://github.com/user-attachments/assets/f4c9d03c-f749-4263-8ec4-89b0e3727bd1" />

Navigation bar has back/forward buttons and other options you'll use regularly.

<sub>The QR icon is from an extension: https://github.com/zynomon/onu/blob/assets/extensions/QrCode</sub>

## Tab bar
<img width="403" height="50" alt="image" src="https://github.com/user-attachments/assets/7f5ef9c7-b339-410f-ac79-4622009da136" />

Tab bar is just a toolbar with tabs. Drag and dock it horizontally and it looks like this.

## Rec bar
Recents bar shows recently opened websites - great for quick recaps. As always, you can hide it, lock dragging, or do whatever you want with it.

---
<h1> <img src="https://github.com/zynomon/onu/raw/release/Notices/icons/home.svg" height="48" width="48"> Home</h1>

<img src="https://github.com/user-attachments/assets/224cdd3f-835a-4155-8680-e15d31ec247e" width="600">

`onu://home` is a customizable dashboard:

- **Widgets** - Add clocks, rotating quotes, bookmarks, or custom HTML
- **Edit mode** - Press `Ctrl+Alt+S` to toggle. Drag widgets anywhere, resize from corners
- **Auto-save** - Widget positions and settings persist
- **Search integration** - Engine tray with GitHub, Wikipedia, and default search
- **Live theming** - Colors and fonts update immediately

Widget types:
- **Chronos** - Digital clock with seconds, date, 12/24h options
- **MOTD** - Rotating quotes (you can add your own)
- **Bookmarks** - Displays favorites from browser (filter which ones)
- **Custom** - Write your own HTML/JavaScript

> **⚠️ DISCLAIMER:** There are still some inconsistencies to be fixed in the next version.

---

<h1> <img src="https://github.com/zynomon/onu/raw/release/Notices/icons/pkg.svg" height="48" width="48"> Custom Multi-threaded Download Manager</h1>

<img src="https://github.com/user-attachments/assets/abcdd8ce-8f38-4a0e-93e5-07741d08665e" width="600">

- **Multi-segment downloads** - Files split into parallel chunks (1-32 segments) for faster speeds
- **Pause/Resume** - Stop and continue downloads across browser sessions
- **Smart folders** - Files auto-sorted into Documents, Images, Videos, Music, Archives, Others
- **Download manager** - `Ctrl+J` opens manager with progress, speed, file size
- **History grouping** - Downloads grouped by Today, Yesterday, This Week, Older
- **Right-click options** - Open, Show in Folder, Pause, Resume, Cancel, Remove

---

<h1><img src="https://github.com/zynomon/onu/raw/release/Notices/icons/theme.svg" height="48" width="48"> Style and Customizations</h1>

- **Built-in themes** - Dark, Light, System (follows desktop)
- **Custom themes** - Create your own in `~/.local/share/Onu/Conf/themes/`
- **Live QSS editing** - Edit and see changes immediately
- **Theme controls** - Colors (accent, background, text, surface), fonts, border radius, padding, header size
- **Icon themes** - Support for system icon packs
- **Qt styles** - Change widget style (Fusion, Windows, etc.)

<details>
<summary><b>What is QSS?</b></summary>

QSS means Qt Style Sheet. The syntax is the same as CSS, but advanced features like shadows aren't there (gradients are supported). For more reference, see the [Qt Style Sheets Reference](https://doc.qt.io/qt-6.8/stylesheet-reference.html).

Our default QSS themes can make you familiar with the syntax and give you information about what properties are available. You can also check [Making QSS themes](#creating-theme).
</details>

---

<h1><img src="https://github.com/zynomon/onu/raw/release/Notices/icons/ext.svg" height="48" width="48"> Extensions/Plugin and Scripting System</h1>

- **Location** - `~/.local/share/Onu/Conf/extensions/`
- **Format** - Qt plugins (`.so`/`.dll`/`.dylib`)
- **Capabilities** - Modify browser behavior, inject JavaScript, add context menu items
- **Safety** - New extensions require explicit approval (ambiguity resolution dialog)
- **Management** - `Ctrl+E` or Settings → Scripts → Extension Manager
- **Metadata** - Extensions provide name, version, maintainer, description, icon

For creating an extension, look at [Creating Extensions](#creating-extension).

---

<h1><img src="https://github.com/zynomon/onu/raw/release/Notices/icons/stng.svg" height="48" width="48"> Settings</h1>

Settings are organized into tabs with search that filters entire sections:

- **General** - Homepage, startup behavior, confirm close, adblock toggle, developer mode
- **Toolbars** - Visibility, lock dragging/floating, button visibility, icon sizes, search engines
- **Appearance** - Fonts, icon themes, Qt style, theme editor with live preview
- **Privacy & Downloads** - JavaScript/images toggles, cookies, Do Not Track, Chromium flags, encryption, download path
- **Scripts** - Extension manager, user script editor

**Storage:**
- Public settings: `~/.local/share/Onu/Conf/onu.conf`
- Encrypted data: `~/.local/share/Onu/Data/encrypted.conf`

Settings backend saves files into this hierarchy:
```bash
~/.local/share/Onu/
├── Conf/
│   ├── onu.conf # public config
│   ├── blocked_hosts.txt # adblock hosts
│   ├── themes/ # themes folder
│   └── extensions/ # extension folder
└── Data/
    ├── encrypted.conf
    ├── QtWebengine/  # for webengine logging 
    └── icon/ # icon cached by browser
        ├── ThemeCache/ 
        └── others/
```

---

<h1><img src="https://github.com/zynomon/onu/raw/release/Notices/icons/adblock.svg" height="48" width="48"> Adblock</h1>

Onu's adblock works like a hosts file. When you visit a website, the browser checks every domain that page tries to load against a list. If there's a match, the request gets blocked before it even leaves your computer.

**How it works:**
- The list lives at `~/.local/share/Onu/Conf/blocked_hosts.txt`
- Add one domain per line: `doubleclick.net`, `googleadservices.com`, etc.
- You can also use patterns:
  - `||example.com^` blocks example.com and all subdomains
  - `|https://tracker.com/` blocks exact URLs
  - `*/analytics.js` blocks any file named analytics.js
  - Regular expressions work too (between `/`)

**Where to get lists:**
- Write your own
- Copy from [EasyList](https://github.com/KDE/falkon/blob/master/tests/benchmarks/files/easylist.txt)
- Find community lists online

**Performance trade-off:**
Every domain gets checked against every rule. 100 rules = fast. 100,000 rules = slower page loads. The file has a warning about this. Start small and add only what you need.

**Toggle:**
Settings → General → "Enable AdBlock". It's on by default.
---

<h1><img src="https://github.com/zynomon/onu/raw/release/Notices/icons/flag.svg" height="48" width="48"> QtWebEngine Flags</h1>

- **Pass flags to QtWebEngine** - Settings → Privacy & Downloads → Chromium Flags
- **Examples**: `--disable-gpu`, `--enable-logging`, `--ignore-gpu-blocklist`
- **Default**: `--disable-gpu` for stability

<details>
<summary><b>Common flags you might need</b></summary>

| Flag | Description |
|------|-------------|
| `--disable-gpu` | Disable GPU hardware acceleration. Use if you're having graphics glitches or crashes. |
| `--enable-logging` | Enable console logging. Run from terminal to see debug output. |
| `--ignore-gpu-blocklist` | Use GPU even if blacklisted. Can improve performance but might cause crashes. |
| `--remote-debugging-port=9222` | Enable remote debugging on port 9222. |
| `--force-dark-mode` | Force dark mode on all websites. |
| `--disable-web-security` | Disable same-origin policy. **Dangerous** - only for testing. |
| `--disk-cache-size=0` | Disable disk cache. |
| `--user-agent="Custom UA"` | Fake your user agent string. |
| `--disable-javascript` | Disable JavaScript. |
| `--disable-images` | Don't load images. |
| `--disable-plugins` | Disable all plugins. |
| `--disable-notifications` | Block website notifications. |
| `--disable-geolocation` | Block geolocation requests. |


</details>


**View active flags:**
Check `onu://about` page. It shows what flags are currently being used.

**Default flags:**
Onu sets `--disable-gpu` by default for stability. You can override this by adding your own flags.
---

<div align="center">
<img src="https://github.com/zynomon/onu/raw/release/Notices/icons/build.svg" height="500" width="500">
<h1>Build systems</h1>
</div>

### DBS (Distribution Build Scripts)

Onu uses a custom build system that packages for multiple distributions:

```
dists/
├── build                 # Main interactive build script
├── CONF                  # Configuration variables
├── DBS/                  # Distribution build scripts
│   ├── deb.sh           # Debian/Ubuntu package builder
│   ├── rpm.sh           # Fedora/RHEL package builder
│   ├── arch.sh          # Arch Linux package builder (experimental)
│   ├── mac.sh           # macOS package builder (experimental)
│   ├── freebsd.sh       # FreeBSD package builder (experimental)
│   ├── tgz.sh           # Tar.gz archive builder
│   └── appimage.sh      # AppImage builder
└── Packages/            # Output directory for built packages
```

Check [dists/Readme.md](./dists/Readme.md) for more detailed information.

**Usage:**
```bash
cd dists/build
./build                    # Interactive mode (asks for profile and target)
./build --options 2 1      # Build Release .deb
./build --options 2 7      # Build Release AppImage
./build --options 2 8      # Build all package formats
./build --clean            # Remove build directories
```

---

### Troubleshooting

**Build fails: Qt6 not found**
```bash
# Debian/Ubuntu
sudo apt install qt6-base-dev qt6-webengine-dev qt6-multimedia-dev
```

**Trigonometric library missing**
```bash
# Debian/Ubuntu only
wget https://github.com/zynomon/libtrigonometry/raw/main/Release%20files/libtrigonometry-1.0.0-Linux.deb
sudo dpkg -i libtrigonometry-1.0.0-Linux.deb
sudo apt install -f
```
For other distros, build from source: https://github.com/zynomon/libtrigonometry

**"zstd not found" when building tgz**
```bash
sudo apt install zstd        # Debian/Ubuntu
```

**Encryption issues at startup**
- After 10 failed password attempts, the browser will shut down. Delete `~/.local/share/Onu/Data/encrypted.conf`
- Warning: This erases all encrypted data (history, favorites, sessions, download path config)

---

### Creating a Fork very easily

1. Fork the repository on GitHub/Codeberg
2. Clone your fork:
   ```bash
   git clone https://github.com/zynomon/onu
   cd onu
   ```
3. Update project metadata in `dists/build/CONF` and add these:
   ```bash
   PKG_NAME="your-browser-name"
   PKG_MAINTAINER="Your Name <your.email@example.com>"
   PKG_VENDOR="Your Name/Organization"
   PKG_URL="https://github.com/yourusername/onu"
   ```
4. Change branding:
   - Window title in `main.cxx`
   - About page in `qrc/about.html`
   - Icon and other assets in `dists/onu.svg`
5. Build your fork:
   ```bash
   cd dists/build
   ./build --options 2 1
   ```

---

### Understanding the codebase

**Core files:**
- `main.cxx` - Entry point, QApplication setup, window creation
- `onu.H` - Main browser classes (Icon_Man, Browser_Backend, Onu_Web, Onu_window)
- `settings.Hxx` - Settings backend with encryption
- `Qtdl.H` - Multi-segment downloader with pause/resume
- `extense.H` - Extension system with plugin loading

**Key directories:**
- `qrc/` - Resources (HTML, CSS, icons)
- `dists/` - Build system and packaging
- `dists/DBS/` - Distribution build scripts

**Data locations:**
- `~/.local/share/Onu/Conf/` - Public settings
- `~/.local/share/Onu/Data/` - Encrypted data
- `~/.local/share/Onu/Conf/themes/` - Custom themes
- `~/.local/share/Onu/Conf/extensions/` - Extensions

**Main classes:**
- `Onu_window` - Main browser window, tab management
- `Browser_Backend` - History, favorites, sessions
- `Create_Toolbars` - UI assembly, toolbar management
- `Onu_Web` - Web page with adblock, context menus
- `Qtdl`/`DL_Man` - Download engine and manager
- `Settings_Backend` - Encrypted settings storage

---

# Building Assets

## Creating Extension

Extensions are Qt plugins that implement the `IExtension` interface. Basic structure:

```cpp
#include <extense.H>

class MyExtension : public QObject, public IExtension {
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "onu.Iext/0.5")
    Q_INTERFACES(IExtension)

public:
    ExtensionMetadata metadata() const override {
        return {
            "Extension Name",
            "1.0.0",
            "Your Name",
            "Description of what it does",
            QIcon::fromTheme("extension-icon")
        };
    }

    bool apply(QMainWindow* window, Browser_Backend* backend) override {
        // Initialize your extension
        return true;
    }

    QString injectScript() const override {
        return "// JavaScript to inject into every page";
    }

    QList<QAction*> getContextMenuActions(QWebEngineView* view) const override {
        QList<QAction*> actions;
        // Add custom context menu items
        return actions;
    }
};
```

Compile as a shared library and place in `~/.local/share/Onu/Conf/extensions/`
AS for refrence check the assets branch

---

## Creating Theme

Themes are QSS files placed in `~/.local/share/Onu/Conf/themes/`
Check assets branch for refrence
Create your theme in Settings → Appearance → Create New Theme, or manually add `.qss` files to the themes folder.

---

## Contributing Assets

**For the main branch:**
- Code changes, bug fixes, feature implementations
- Follow existing code style
- Test your changes before submitting
- Open a pull request with clear description

**For the assets branch:**
- Icons, themes, documentation, promotional materials
- Place assets in appropriate directories
- Include attribution if not original
- Add readme file
- Open a pull request against the `assets` branch

**Guidelines:**
- Keep it simple and functional
- Match existing aesthetic where appropriate
- Document what your asset does
- Test themes with both light and dark variants

---

## Q&A

<summary><strong>What is Qt6?</strong></summary>
<details>
Qt6 is a Framework for C++ and many more programming languages. It's what KDE, VLC, Telegram and many more known applications use to build their software. While it's very mainstream, it's lightweight if used properly.
</details>

<summary><strong>Which OS(es) are supported?</strong></summary>
<details>
Onu supports Linux based distros, BSD and Mac, but BSD and Mac haven't been tested yet. Windows support is planned.
</details>

<summary><strong>Is it safe?</strong></summary>
<details>
Safety depends on who uses it and how. For daily driving, it's experimental in version 0.5, so incognito mode is turned on by default. After adding permission systems and safety checks, users will get control over incognito mode.
</details>

<summary><strong>How stable is this?</strong></summary>
<details>
Partially stable. Report bugs to help fix them.
</details>

<summary><strong>Where can I find older versions?</strong></summary>
<details>
https://github.com/zynomon/onu/blob/release/
</details>

<summary><strong>How do I report bugs?</strong></summary>
<details>
https://github.com/zynomon/onu/issues
</details>

<summary><strong>Can I contribute?</strong></summary>
<details>
Yes. Report bugs, suggest features, submit pull requests, create themes, write extensions, improve docs.
</details>

<summary><strong>Where is my data stored?</strong></summary>
<details>
`~/.local/share/Onu/Conf/` for settings, `~/.local/share/Onu/Data/` for encrypted history/favorites.
</details>

<summary><strong>How do I uninstall?</strong></summary>
<details>
Use your package manager (dpkg/rpm/pacman) or delete `~/.local/bin/onu` and `~/.local/share/Onu` if installed manually.
</details>

<summary><strong>How do I reset to defaults?</strong></summary>
<details>
Delete `~/.local/share/Onu/Conf` and `~/.local/share/Onu/Data`, or use the "Remove All Data" button in Settings.
</details>

<summary><strong>I forgot my encryption password. What now?</strong></summary>
<details>
After 10 failed attempts, delete `~/.local/share/Onu/Data/encrypted.conf`. Warning: This erases all encrypted data.
</details>

---

## 🖼️ Previews

<div align="center">

<details>
<summary><b>Current version (0.5)</b></summary>

<img width="1303" height="698" alt="image" src="https://github.com/user-attachments/assets/c65b6179-f8ad-4ae3-a724-a8bf42b80fd7" />
<img width="1388" height="711" alt="image" src="https://github.com/user-attachments/assets/039c9992-3e45-4485-be4a-7a3b12693496" />
<img width="1440" height="775" alt="image" src="https://github.com/user-attachments/assets/ce1e774c-a704-4f8b-af77-d52a9e36c403" />
<img width="1346" height="717" alt="image" src="https://github.com/user-attachments/assets/c6431b92-55b9-49c5-abd1-cee13606ec41" />
<img width="1320" height="708" alt="image" src="https://github.com/user-attachments/assets/0ddf1d51-f40b-4784-ab9a-490495b9a3e1" />
<img width="1295" height="689" alt="image" src="https://github.com/user-attachments/assets/a6c38d89-29cb-404f-b57d-0e22c2ebeda2" />
<img width="968" height="719" alt="image" src="https://github.com/user-attachments/assets/c379329f-6a28-4638-9c5b-d41dc4fc0110" />
<img width="136" height="63" alt="image" src="https://github.com/user-attachments/assets/78da2a04-c2ef-4882-a3d1-b65e8c432ec4" /> (fav toolbar)
<img width="854" height="638" alt="image" src="https://github.com/user-attachments/assets/df0cef6b-ef14-43a1-9dc3-53ffdb779a35" /> (fav manager)
<img width="1361" height="728" alt="image" src="https://github.com/user-attachments/assets/a4df8196-0eb8-46d9-88ac-4fdb13a46ead" /> (history)
<img width="267" height="65" alt="image" src="https://github.com/user-attachments/assets/30fabf6a-0365-4d86-b4bd-ec3b44c71dde" /> (recent toolbar)
<img width="469" height="333" alt="image" src="https://github.com/user-attachments/assets/b6b9df3e-83db-47f8-b06e-85696e8f83f6" /> (movable draggable lockable hidable)
<img width="1303" height="702" alt="image" src="https://github.com/user-attachments/assets/76cfecc6-115b-49db-9eb1-43a74baec3fa" />
docking everything aside,
<img width="1314" height="703" alt="image" src="https://github.com/user-attachments/assets/c3a04647-806b-40d7-8149-0fd9895ee2ec" />
(making the layout look like some traditional browsers)
<img width="769" height="550" alt="image" src="https://github.com/user-attachments/assets/b411a78e-7d94-4a6b-b5f7-32a42ac3050d" />
downloads system
light theme<img width="1299" height="700" alt="image" src="https://github.com/user-attachments/assets/95a765fb-ecbd-4a51-9c97-e26699641222" />
keybind editor (Ctrl+K)<img width="779" height="654" alt="image" src="https://github.com/user-attachments/assets/b7bd9756-1d38-4480-8d3d-3139d67380d2" />

</details>

<details>
<summary><b>Version 0.4</b></summary>

<img width="1440" height="734" alt="Onu 0.4" src="https://github.com/user-attachments/assets/066fb104-7711-4c3a-bb30-526b3b63ee62" />
<img width="842" height="652" alt="Page source viewer" src="https://github.com/user-attachments/assets/0f136429-c8c1-4a3c-a219-e4f08b2d1dd9" />
<img width="566" height="371" alt="Purple neon theme" src="https://github.com/user-attachments/assets/c4713935-3e71-406b-abc3-4ebd6d484ec5" />

</details>

<details>
<summary><b>Version 0.3</b></summary>

<img width="1278" height="663" alt="image" src="https://github.com/user-attachments/assets/63766c98-eab4-4101-a4da-2788453053be" />
<img width="1281" height="707" alt="image" src="https://github.com/user-attachments/assets/f1701846-3bd7-4c33-af4f-be1a5e071324" />
<img width="1440" height="727" alt="image" src="https://github.com/user-attachments/assets/43f7b73d-44cb-433e-ab8b-1e81760a0012" />

</details>

<details>
<summary><b>Version 0.2</b></summary>

<img width="1380" height="467" alt="Onu 0.2" src="https://github.com/user-attachments/assets/b35840f2-f05e-4533-8c89-8b032e7ebcfe" />
<img width="1440" height="623" alt="Onu 0.2" src="https://github.com/user-attachments/assets/1226e536-8dc3-4a0e-97c8-d12b2841c595" />
<img width="1440" height="726" alt="Onu 0.2" src="https://github.com/user-attachments/assets/d4cc68af-8265-4691-86f8-5b7c76cfcaa5" />
<img width="1438" height="334" alt="Onu 0.2" src="https://github.com/user-attachments/assets/a531fbde-5692-415e-a71f-5353a23ea168" />
<img width="313" height="219" alt="Onu 0.2" src="https://github.com/user-attachments/assets/3bb93ee7-0c8b-4157-b65a-5f1bd9d752bd" />
<img width="785" height="616" alt="Onu 0.2" src="https://github.com/user-attachments/assets/875f5c8b-5ef9-45b9-abce-c0e720fa29f3" />
<img width="778" height="446" alt="Onu 0.2" src="https://github.com/user-attachments/assets/9e7eebf9-f7bc-43f9-baac-d0c7ab9d2810" />

</details>

<details>
<summary><b>Version 0.1</b></summary>

<img width="300" alt="Onu 0.1" src="https://github.com/user-attachments/assets/ca1d798c-fc82-46f0-82d8-86ccbd44bf90"/>
<img src="https://github.com/user-attachments/assets/c64652bb-1218-413a-93bc-6d3c3750076e" />
<img src="https://github.com/user-attachments/assets/f4e9ec85-a291-4f75-90a8-aff8217d2b85" />
<img width="1145" height="632" alt="Onu 0.1" src="https://github.com/user-attachments/assets/8a7a8f69-8c3a-4926-83cb-fb3bb2116d07" />
<img width="393" height="95" alt="Onu 0.1" src="https://github.com/user-attachments/assets/deafd1e8-25dd-42b4-b09b-1f9c7632b547" />
<img width="1165" height="592" alt="Onu 0.1" src="https://github.com/user-attachments/assets/f9e6aa4a-6952-4145-aa2e-8ec5969356c9" />
<img width="1022" height="446" alt="Onu 0.1" src="https://github.com/user-attachments/assets/402d563a-b63e-4d3f-9aee-2d87128f2d26" />
<img width="1144" height="597" alt="Onu 0.1" src="https://github.com/user-attachments/assets/6a374589-765b-4992-b9c2-9b5457f635d5" />

</details>
</div>

---

## TLDR;

- For more suggestions or bug reports, open an "issue" on this repo, or you can [join us on discord](https://discord.gg/Jn7FBwu99F) if you feel comfortable there. 😅
- This project follows Apache 2.0 license. You can fork it or do whatever you want, but don't forget to credit me.

![Built with](https://img.shields.io/badge/Built%20with-070707?style=for-the-badge) ![C++](https://img.shields.io/badge/-00599C?logo=cplusplus&logoColor=white&style=for-the-badge)
![Qt6 Webengine](https://img.shields.io/badge/Qt6%20Webengine-185214?style=for-the-badge&logo=qt&logoColor=white)
![Ko-fi](https://img.shields.io/badge/Ko--fi-FF5E5B?style=for-the-badge&logo=kofi&logoColor=white)

<a href="https://www.star-history.com/?repos=zynomon%2Fonu&type=timeline&legend=bottom-right">
 <picture>
   <source media="(prefers-color-scheme: dark)" srcset="https://api.star-history.com/image?repos=zynomon/onu&type=timeline&theme=dark&legend=bottom-right" />
   <source media="(prefers-color-scheme: light)" srcset="https://api.star-history.com/image?repos=zynomon/onu&type=timeline&legend=bottom-right" />
   <img alt="Star History Chart" src="https://api.star-history.com/image?repos=zynomon/onu&type=timeline&legend=bottom-right" />
 </picture>
</a>
