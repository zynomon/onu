
# ![onu](https://github.com/user-attachments/assets/c0d6ada6-b9d5-41c3-8ecc-9c402ed73fce) Onu.
<p align="center">
  <img src="https://img.shields.io/badge/Status-Beta-orange?style=flat-square">
  <img src="https://img.shields.io/badge/License-Apache%202.0-green?style=flat-square">
  <img src="https://img.shields.io/badge/Platform-Linux-green?style=flat-square">
  <a href="https://github.com/zynomon/onu"><img src="https://img.shields.io/github/stars/zynomon/error?style=social"></a>
  <a href="https://github.com/zynomon/onu/fork"><img src="https://img.shields.io/github/forks/zynomon/error?style=social"></a>
</p>

onu is a small tiny browser. it's main goal is to fit in any low storage devices , fast and every aspect of it is customizable,
---
## 📦 Download
most of them are under 300KB if the desktop environment is KDE if its gnome or something else it has some dependency that will take upto 200MB.

- 🔗 **[Download Onu Beta ~ version 0.3 (.deb)](https://github.com/zynomon/onu/raw/release/beta/onu-0.3.deb)**
- 🔗 **[Download Onu Beta ~ version 0.2 (.deb)](https://github.com/zynomon/onu/raw/release/beta/onu-0.2.deb)**
- 🔗 **[Download Onu Beta ~ version 0.1 (.deb)](https://github.com/zynomon/onu/raw/release/beta/onu-0.1.deb)**
- 🔗 **[Download Onu Beta ~ previous version sources (.7zip)](https://github.com/zynomon/onu/blob/release/beta/)**

current version's (0.3) source is in this main branch repo so you could clone it without issues.

--- 

Changelogs
---
0.3 includes,
- Rebuilt UI into a single clean navigation toolbar (removed multi-toolbar system from 0.2)
- Removed userscript support entirely (including engine and settings tab)
- Simplified tab bar: replaced custom "+" tab button with standard new-tab button in toolbar
- Improved tab handling: proper title/icon updates, secure/insecure tooltip indicators, and right-click context menu (Close, Duplicate, View Source)
- Enhanced Settings dialog with new tabs: Privacy (JavaScript/Images toggles), Downloads (duplicate prevention), Developer (Inspect Element toggle), and Engine Flags (GPU, WebRTC, Force Dark Mode, etc.)
- Added command-line arguments: --nogpu and --force-dark
- Improved adblock engine with host caching and runtime reload from settings
- More reliable session restore using timestamped QSettings entries
- Rewrote download manager with speed tracking, cancel support, and duplicate prevention
- Unified address/search bar with history-based autocompletion (QCompleter)
- Added built-in onu:// pages: home, game, history, and clear-history
- Implemented notification presenter and permission request dialog (camera, mic, geolocation, etc.)
- Added full keyboard shortcuts (F5, Ctrl+F5, Ctrl+K, Ctrl+G, Ctrl+H, Ctrl+J, etc.)
- Refined theme system: System/Light/Dark/Custom with live QSS preview and tab bar styling
- Restored toolbar positions across sessions
- Added window dragging via empty menu bar area
- Improved drag-and-drop support on tab bar for URLs
- Removed bookmark UI in favor of history-based navigation
<details>
<summary><b>previous versions changelogs</b> <span style="font-size:14px;">(click to expand)</span></summary>
0.2 includes,
- Added new-tab button directly on the tab bar
- Implemented drag-and-drop URL opening on tab bar
- Introduced experimental multi-toolbar system (Tabs, Navigation, Search toolbars)
- Added orientation-aware toolbar behavior (icon/text layout changes when vertical)
- Custom context menu with “Inspect Element” option
- Integrated DevTools window support for debugging
- Enhanced popup handling: window.open() now opens in new tabs
- Improved userscript injection reliability
- Upgraded adblock host matching with case-insensitive support
- Refined bookmarks UI with clear-all option
- Enhanced history tracking with more consistent saving
- Improved session restore logic
- Added event filter for tab bar drop events
- Implemented dynamic tab icon and title updates
- First version to include developer tools and advanced UX features
  
0.1 included,

- Basic tabbed browsing with QTabWidget
- Unified navigation toolbar with back, forward, reload, stop, new tab, and game button
- Built-in adblock using host-based filtering from blocked_hosts.txt
- Simple userscript support (Tampermonkey-like) with .js injection on page load
- Custom settings dialog with tabs for General, Appearance, Privacy, Downloads, and Userscripts
- Theme support: System, Light, and Dark with palette-based styling
- Custom QSS stylesheet support applied without restart
- Download manager using QNetworkAccessManager with progress tracking
- Session restore: saves and reloads open tabs on launch
- Bookmark system with add/manage UI and persistent storage via QSettings
- History tracking using QSettings with timestamped entries
- Built-in offline 2048 game accessible via qrc:/2048.html
- Homepage customization (defaults to qrc:/startpage.html)
- Status bar with link hover previews and load status
- Basic keyboard shortcuts (Ctrl+T, Ctrl+G, Ctrl+J, Ctrl+D, etc.)
- First QSettings-based configuration system

</details>

<details>
<summary><b>Load</b> <span style="font-size:14px;">(click to expand)</span></summary>
## 🖼️ Preview

<div align="center">
<img width="1380" height="467" alt="image" src="https://github.com/user-attachments/assets/b35840f2-f05e-4533-8c89-8b032e7ebcfe" />
<img width="1440" height="623" alt="image" src="https://github.com/user-attachments/assets/1226e536-8dc3-4a0e-97c8-d12b2841c595" />
<img width="1440" height="726" alt="image" src="https://github.com/user-attachments/assets/d4cc68af-8265-4691-86f8-5b7c76cfcaa5" />
<img width="1438" height="334" alt="image" src="https://github.com/user-attachments/assets/a531fbde-5692-415e-a71f-5353a23ea168" />
<img width="313" height="219" alt="image" src="https://github.com/user-attachments/assets/3bb93ee7-0c8b-4157-b65a-5f1bd9d752bd" />
<img width="785" height="616" alt="image" src="https://github.com/user-attachments/assets/875f5c8b-5ef9-45b9-abce-c0e720fa29f3" />
<img width="778" height="446" alt="image" src="https://github.com/user-attachments/assets/9e7eebf9-f7bc-43f9-baac-d0c7ab9d2810" />
Dark mode ( the home menu has some issues soon it will be fixed )
<img width="1440" height="900" alt="VirtualBox_QTDEVPRO_12_12_2025_11_34_16" src="https://github.com/user-attachments/assets/efc07d22-63b3-4457-9b6e-89290f6324d0" />
<img width="1440" height="900" alt="VirtualBox_QTDEVPRO_12_12_2025_11_32_57" src="https://github.com/user-attachments/assets/742bf0e9-fe93-4cac-bace-1fc823d74364" />
<img width="1440" height="900" alt="VirtualBox_QTDEVPRO_12_12_2025_11_33_59" src="https://github.com/user-attachments/assets/ff3c5b35-e177-4f8b-b5cb-69b648d9dc3a" />
<img width="1440" height="900" alt="VirtualBox_QTDEVPRO_12_12_2025_11_34_11" src="https://github.com/user-attachments/assets/cadc3c9d-8ffe-4009-adfd-8cb53917f2f1" />
<img width="1440" height="900" alt="VirtualBox_QTDEVPRO_12_12_2025_11_34_16" src="https://github.com/user-attachments/assets/aa65d22a-18be-4701-90b5-b2b7f8446ff3" />

</details>

<details>
<summary><b>Screenshots from 0.2 version</b></summary>
0.2 includes,
 - Added new-tab button on the tab bar
- Added drag-and-drop URL opening on tab bar
- Added multiple toolbars (tabs, navigation, search)
- Added orientation-aware toolbar behavior
- Added custom context menu with “Inspect Element”
- Added DevTools window support
- Added popup/tab creation handling (window.open → new tab)
- Added improved userscript injection
- Improved adblock host matching
- Improved bookmarks UI
- Improved history tracking
- Improved session restore
- Added event filter for tabbar drop events
- Added tab icon/title updates
- First version with developer tools and advanced UX features
  
## 🖼️ This was a beta prototype

<div align="center">

![preview](https://github.com/user-attachments/assets/6fefa72e-38dc-4544-ad8d-9ed2f2edf685)
![prev](https://github.com/user-attachments/assets/6eeda817-45f7-460a-9ec8-393bf5f26d9c)
![VirtualBoxVM_LivCy2ZifW](https://github.com/user-attachments/assets/9900ea53-1da0-48db-a591-6d5cda5d343d)

<img width="1440" alt="stuff" src="https://github.com/user-attachments/assets/ffa3fd30-e5fe-4e90-b344-f89a69439294" />
<img width="1440" alt="set" src="https://github.com/user-attachments/assets/d4338294-596f-4aaf-b9c1-fefce032aa5d" />
<img width="1440" alt="set 2" src="https://github.com/user-attachments/assets/774a84dc-5cf9-4c4f-90ff-6ebc5fa6b831" />

![installing](https://github.com/user-attachments/assets/fccfeb20-741d-4bc1-a44f-b37880fc9c39)

<img width="1440" alt="home" src="https://github.com/user-attachments/assets/ac924a96-73e9-4cfd-b5ae-185cff931423" />
<img width="714" alt="game" src="https://github.com/user-attachments/assets/70b0bdce-719c-4b18-a21e-38c17caf144c" />
![gameplay](https://github.com/user-attachments/assets/cf97d488-927d-4461-8e68-a0d7913e9aeb)

</div>


</details>
<details>
<summary><b>Screenshots from 0.1 version</b></summary>
  
0.1 included,
- Basic tabbed browsing
- Basic navigation toolbar
- Basic adblock (host-based)
- Basic userscript loader
- Basic session restore (URL-only)
- Basic bookmarks & history
- Basic download manager
- Minimal settings dialog
- Built-in offline 2048 game
- First QSettings-based configuration system
  
## 🖼️ This was a beta prototype 

<div align="center">


<img width="300" alt="Onu browser" src="https://github.com/user-attachments/assets/ca1d798c-fc82-46f0-82d8-86ccbd44bf90"/>

  <img src="https://github.com/user-attachments/assets/c64652bb-1218-413a-93bc-6d3c3750076e" />
  <img src="https://github.com/user-attachments/assets/f4e9ec85-a291-4f75-90a8-aff8217d2b85"  />
  <img width="1145" height="632" alt="image" src="https://github.com/user-attachments/assets/8a7a8f69-8c3a-4926-83cb-fb3bb2116d07" />
<img width="393" height="95" alt="image" src="https://github.com/user-attachments/assets/deafd1e8-25dd-42b4-b09b-1f9c7632b547" />
<img width="1165" height="592" alt="image" src="https://github.com/user-attachments/assets/f9e6aa4a-6952-4145-aa2e-8ec5969356c9" />
<img width="1022" height="446" alt="image" src="https://github.com/user-attachments/assets/402d563a-b63e-4d3f-9aee-2d87128f2d26" />
  <img width="1144" height="597" alt="image" src="https://github.com/user-attachments/assets/6a374589-765b-4992-b9c2-9b5457f635d5" />

</div>


</details>
</div>


</details>

---

- for more suggestions open a "issue" on this repo, or you can [join us on discord](https://discord.gg/Jn7FBwu99F) if you feel comfort there. 😅
  
