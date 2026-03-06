<img src="https://github.com/zynomon/onu/blob/release/Notices/icons/theme.svg" height="256" width="256">

# Onu Browser QSS Themes

Create and share custom QSS (Qt Style Sheets) themes for the Onu browser. Themes control the appearance of the browser UI, including colors, widgets, tabs, and toolbars.

## Repository Structure

```
Themes/
├── Your_Theme/              # Your theme directory
│   ├── Your_Theme.qss       # Required: QSS theme file
│   ├── preview.png          # Optional: Theme screenshot
│   └── README.md            # Required: Theme documentation
└── README.md                # This file
```

## Contributing a Theme

### Step 1: Clone the Assets Branch

```bash
git clone https://github.com/zynomon/onu --branch assets
cd onu/Themes
```

### Step 2: Create Your Theme Directory

```bash
mkdir Your_Theme_Name
cd Your_Theme_Name
```

### Step 3: Create Your QSS File

Create `Your_Theme_Name.qss`:

```css
/* Onu Theme - Your Theme Name */

/* Main Window */
QMainWindow {
    background-color: #1e1e1e;
}

/* Add your custom styles here */
```

### Step 4: Widget Reference Guide

Common Onu widgets you can style:

| Widget | Description | Common Properties |
|--------|-------------|-------------------|
| `QMainWindow` | Main browser window | `background-color` |
| `QMenuBar` | Top menu bar | `background-color`, `color` |
| `QToolBar` | Navigation toolbars | `background-color`, `border`, `spacing` |
| `QToolButton` | Toolbar buttons | `background-color`, `border-radius`, `padding` |
| `QTabWidget` | Tab container | `border`, `background` |
| `QTabBar::tab` | Individual tabs | `background`, `color`, `padding`, `border` |
| `QLineEdit` | Address/search bar | `background`, `border`, `color` |
| `QStatusBar` | Bottom status bar | `background`, `color` |
| `QScrollBar` | Scrollbars | `width`, `background`, `border-radius` |
| `QDialog` | Settings dialogs | `background`, `color` |
| `QPushButton` | Buttons | `background`, `border`, `border-radius` |
| `QCheckBox` | Checkboxes | `color` |
| `QProgressBar` | Loading progress | `border`, `background`, `chunk` |

### Step 5: Add a Preview Image (Optional)

Take a screenshot of Onu with your theme applied and save it as `preview.png` in your theme directory.

### Step 6: Create README.md

```markdown
# Your Theme Name

## Description
Briefly describe your theme - colors, style, inspiration.

## Author
Your Name/Handle

## Preview
![Theme Preview](preview.png)

## Color Palette
- **Background:** #1e1e1e
- **Surface:** #2d2d2d  
- **Accent:** #0d7377
- **Text:** #ffffff
- **Hover:** #3d3d3d

## Styled Elements
- Main window and menus
- Toolbars and buttons
- Tab bar
- Address bar
- Status bar
- Scrollbars
- Dialogs
- Progress bars

## Installation

### Option 1: Direct Import
1. Download `Your_Theme_Name.qss`
2. Open Onu Settings (Ctrl+,)
3. Navigate to Appearance
4. Click "+" and paste the QSS content

### Option 2: Manual Installation

**Linux:**
```bash
cp Your_Theme_Name.qss ~/.local/share/Onu/Conf/themes/
```

**Windows:**
```pwsh
copy Your_Theme_Name.qss %APPDATA%\Onu\Conf\themes\
```

**macOS:**
```bash
cp Your_Theme_Name.qss ~/Library/Application\ Support/Onu/Conf/themes/
```

After copying:
1. Restart Onu
2. Go to Settings → Appearance
3. Select "Your Theme Name" from the theme dropdown
4. Click Save

## Compatibility
- Tested with Onu version: 0.5
- Works alongside built-in light/dark themes

## License
[Choose a license: MIT, GPL, CC0, etc.]

## Notes
Any additional information about your theme.
```

## Theme Design Tips

- Use consistent color schemes throughout
- Test with both light and dark web content
- Ensure sufficient contrast for readability (WCAG guidelines)
- Consider accessibility for color-blind users
- Choose descriptive theme names
- Test all UI elements before submitting

## Submitting Your Theme

```bash
# Add your theme directory
git add Your_Theme_Name/

# Commit with a descriptive message
git commit -m "Add Your_Theme_Name QSS theme"

# Push to assets branch
git push origin assets
```

Then create a pull request on GitHub for the `assets` branch including:
- Theme name and description
- Preview image (if available)
- Color scheme details

---

For QSS reference check https://github.com/Ktiseos-Nyx/qss_themes
it contains various qss theme;
