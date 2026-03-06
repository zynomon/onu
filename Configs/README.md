<img src="https://github.com/zynomon/onu/blob/release/Notices/icons/config.svg" height="256" width="256">

# Onu Browser Configurations

## Overview

Users can share their entire Onu browser configuration folder. This includes all public settings, custom themes, extension states, and other non-sensitive configuration data.

## File Structure

```
Configs
├── Your_Config/
│   ├── Conf/                         # Entire Conf folder
│   │   ├── onu.conf                  # Main configuration
│   │   ├── themes/                    # Custom themes
│   │   │   ├── YourTheme.qss
│   │   │   └── ...
│   │   ├── extensions/                 # Extension states
│   │   └── blocked_hosts.txt          # Adblock rules
│   └── README.md                      # Information about your config
└── README.md
```

## How to Contribute Configurations

### Step 1: Clone the Assets Branch

```bash
git clone https://github.com/zynomon/onu --branch assets
cd onu/Configs
```

### Step 2: Create Your Config Directory

```bash
mkdir Your_Config
cd Your_Config
```

### Step 3: Copy Your Conf Folder

Your configuration is located at:

**Linux:** `~/.local/share/Onu/Conf/`

**Windows:** `%APPDATA%/Onu/Conf/`

**macOS:** `~/Library/Application Support/Onu/Conf/`

Copy the entire Conf folder:

```bash
cp -r ~/.local/share/Onu/Conf ./
```

### Step 4: Clean Sensitive Data

Remove any sensitive information from your config before sharing:

- Check `onu.conf` for any private data
- Ensure no passwords or personal information are included
- Extension states are safe to share (only enable/disable status, no private data)

### Step 5: Create README.md

```md
# Your Configuration Name

## Description
What makes this configuration special - workflow, visual style, privacy setup, etc.

## Author
Your Name/Handle

## Contents

### onu.conf
Main configuration file containing:
- Search engine preferences
- Privacy settings
- Download options
- Keyboard shortcuts
- UI preferences
- Extension states

### themes/
Custom themes included:
- [Theme Name] - Description
- [Theme Name] - Description

### extensions/
Extension states (enabled/disabled only, no private data)

### blocked_hosts.txt
Adblock rules configuration

## Screenshots
[Link to screenshots]

## Installation

1. Back up your current Conf folder:
```bash
mv ~/.local/share/Onu/Conf ~/.local/share/Onu/Conf.backup
```

2. Copy this configuration:
```bash
cp -r Conf ~/.local/share/Onu/
```

3. Launch Onu browser

## Features

### Visual
- Theme description
- Icon theme
- Font settings
- Toolbar layout

### Functionality
- Search engine setup
- Keyboard shortcuts
- Privacy settings
- Download preferences

### Extensions
List of enabled extensions and what they do

## Notes
Any additional information about using or modifying this configuration.

## License
[Choose a license]
```

## Submitting Your Configuration

```bash
git add Your_Config/
git commit -m "Add Your_Config configuration"
git push origin assets
```

Create a pull request on GitHub for the assets branch with:
- Description of your configuration
- Screenshots
- List of key features

## Important Notes

- Only share **public** configurations not data folder
- Configurations can be combined manually
