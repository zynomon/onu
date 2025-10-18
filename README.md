


<?xml version="1.0" encoding="UTF-8"?>
<ui version="4.0">
  <class>BrowserWindow</class>
  <widget class="QMainWindow" name="BrowserWindow">
    <property name="geometry">
      <rect>
        <x>0</x>
        <y>0</y>
        <width>1300</width>
        <height>768</height>
      </rect>
    </property>
    <property name="windowTitle">
      <string>Onu Browser</string>
    </property>
    <widget class="QWidget" name="centralWidget">
      <layout class="QVBoxLayout" name="verticalLayout">
        <item>
          <widget class="QTabWidget" name="tabWidget">
            <property name="tabsClosable">
              <bool>true</bool>
            </property>
            <property name="movable">
              <bool>true</bool>
            </property>
            <property name="documentMode">
              <bool>true</bool>
            </property>
          </widget>
        </item>
      </layout>
    </widget>
    <widget class="QToolBar" name="toolBar">
      <property name="windowTitle">
        <string>Navigation</string>
      </property>
      <attribute name="toolBarArea">
        <enum>TopToolBarArea</enum>
      </attribute>
      <addaction name="actionBack"/>
      <addaction name="actionForward"/>
      <addaction name="actionReload"/>
      <addaction name="actionHome"/>
      <addaction name="separator"/>
      <widget class="QLineEdit" name="lineEdit">
        <property name="placeholderText">
          <string>Search or enter URL</string>
        </property>
        <sizepolicy hsizetype="Expanding" vsizetype="Fixed">
          <horstretch>0</horstretch>
          <verstretch>0</verstretch>
        </sizepolicy>
      </widget>
      <addaction name="actionNewTab"/>
      <addaction name="separator"/>
      <addaction name="actionSettings"/>
    </widget>
    <widget class="QStatusBar" name="statusBar"/>
    <widget class="QMenuBar" name="menuBar">
      <widget class="QMenu" name="menuFile">
        <property name="title">
          <string>&amp;File</string>
        </property>
        <addaction name="actionNewTab"/>
        <addaction name="actionQuit"/>
      </widget>
      <widget class="QMenu" name="menuNavigation">
        <property name="title">
          <string>&amp;Navigation</string>
        </property>
        <addaction name="actionBack"/>
        <addaction name="actionForward"/>
        <addaction name="actionReload"/>
        <addaction name="actionHome"/>
      </widget>
      <widget class="QMenu" name="menuBookmarks">
        <property name="title">
          <string>&amp;Bookmarks</string>
        </property>
        <addaction name="actionAddBookmark"/>
        <addaction name="actionManageBookmarks"/>
      </widget>
      <widget class="QMenu" name="menuTools">
        <property name="title">
          <string>&amp;Tools</string>
        </property>
        <addaction name="actionSettings"/>
      </widget>
      <addaction name="menuFile"/>
      <addaction name="menuNavigation"/>
      <addaction name="menuBookmarks"/>
      <addaction name="menuTools"/>
    </widget>
    <action name="actionNewTab">
      <property name="icon">
        <iconset theme="tab-new"/>
      </property>
      <property name="text">
        <string>New Tab</string>
      </property>
      <property name="shortcut">
        <string>Ctrl+T</string>
      </property>
    </action>
    <action name="actionQuit">
      <property name="text">
        <string>Quit</string>
      </property>
      <property name="shortcut">
        <string>Ctrl+Q</string>
      </property>
    </action>
    <action name="actionBack">
      <property name="icon">
        <iconset theme="go-previous"/>
      </property>
      <property name="text">
        <string>Back</string>
      </property>
      <property name="shortcut">
        <string>Alt+Left</string>
      </property>
    </action>
    <action name="actionForward">
      <property name="icon">
        <iconset theme="go-next"/>
      </property>
      <property name="text">
        <string>Forward</string>
      </property>
      <property name="shortcut">
        <string>Alt+Right</string>
      </property>
    </action>
    <action name="actionReload">
      <property name="icon">
        <iconset theme="view-refresh"/>
      </property>
      <property name="text">
        <string>Reload</string>
      </property>
      <property name="shortcut">
        <string>F5</string>
      </property>
    </action>
    <action name="actionHome">
      <property name="icon">
        <iconset theme="go-home"/>
      </property>
      <property name="text">
        <string>Home</string>
      </property>
    </action>
    <action name="actionAddBookmark">
      <property name="text">
        <string>Add Bookmark</string>
      </property>
      <property name="shortcut">
        <string>Ctrl+D</string>
      </property>
    </action>
    <action name="actionManageBookmarks">
      <property name="text">
        <string>Manage Bookmarks</string>
      </property>
    </action>
    <action name="actionSettings">
      <property name="text">
        <string>Settings</string>
      </property>
      <property name="shortcut">
        <string>Ctrl+,</string>
      </property>
    </action>
  </widget>
</ui>


# 🌐 Onu Browser

**Onu** is a fast, minimalist browser designed to stay under **10 MB**—yet packed with enough
features to make browsing expressive, playful, and powerful to be run on lower end devices.

---
# preview 🖼️
  <img src="https://github.com/user-attachments/assets/c64652bb-1218-413a-93bc-6d3c3750076e" />
  <img src="https://github.com/user-attachments/assets/f4e9ec85-a291-4f75-90a8-aff8217d2b85"  />

---

### 📦 Downloads

- 🔗 [Download .DEB ](https://github.com/zynomon/onu/raw/release/beta/onu-0.1.deb)
the file is only 200KB,     how to run it? -  well type   ```Onu```  in your terminal. it should work

---
<img width="1145" height="632" alt="image" src="https://github.com/user-attachments/assets/8a7a8f69-8c3a-4926-83cb-fb3bb2116d07" />

> __⚠️ This is the **first release**. (0.1 beta) It can run typical websites, but some features may be unstable or incomplete. Expect quirks—and enjoy the ride.__

---

## 🎮 Features
check this buttons for the features,
<img width="393" height="95" alt="image" src="https://github.com/user-attachments/assets/deafd1e8-25dd-42b4-b09b-1f9c7632b547" />

---

- 🕹️ **Built-in Game**  
  A hidden mini-game embedded in the browser—because why not?
<img width="1165" height="592" alt="image" src="https://github.com/user-attachments/assets/f9e6aa4a-6952-4145-aa2e-8ec5969356c9" />
🤦‍♂️ it seems it can be an issue but the game is fully functional and fun , can you beat 10,000 score?

---

- 🎨 **QSS Editor (Plug & Play)**  
  Customize your browser’s look and feel with live QSS editing. No restart needed.
<img width="1022" height="446" alt="image" src="https://github.com/user-attachments/assets/402d563a-b63e-4d3f-9aee-2d87128f2d26" />

---

- 🧠 **Tampermonkey-Compatible JS Extension Support**  
  Inject custom scripts, automate websites, and unlock endless possibilities.  
  Onu supports user-defined JavaScript extensions—just like Tampermonkey.
  <img width="1144" height="597" alt="image" src="https://github.com/user-attachments/assets/6a374589-765b-4992-b9c2-9b5457f635d5" />

# means even if it can have issues, it has enough features to throw away the fraustration

---

## 🛠️ Coming Soon

- Unicode splash screen logic  
- Modular download manager with native/aria2c fallback
- advanced downloader
- advanced ui
- advanced customization
- for more suggestions open a issue on this repo, or you can [join us on discord](https://discord.gg/Jn7FBwu99F) if you feel comfort there. 😅
  
---


