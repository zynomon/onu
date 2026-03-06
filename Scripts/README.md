## <img src="https://github.com/zynomon/onu/blob/release/Notices/icons/stng.svg" height="32" width="32"> Scripts


Scripts are JavaScript files that get injected into every webpage you visit. They can modify page behavior, add features, or automate tasks.

### Structure
```
Scripts/
├── Your_Script/
│   ├── script.js        # The JavaScript file (injected into every page)
│   └── README.md        # Documentation
└── README.md            # This file
```

### How It Works
- Scripts run on every page load
- They have access to the page's DOM
- Can interact with the page just like a regular userscript
- Enable/disable through Onu's Settings → Scripts tab

### Example
```javascript
// script.js
console.log('My script is running!');
document.body.style.border = '5px solid red';
```

### Installation
1. Open Onu Settings → Scripts
2. Copy-paste your desired , script
3. Enable the script
