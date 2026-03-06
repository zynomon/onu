<img src="https://github.com/zynomon/onu/blob/release/Notices/icons/adblock.svg" height="256" width="256">

# Adblock Rules and System
Onu browser includes a built-in adblocker that uses host-based blocking. The adblocker reads from `blocked_hosts.txt` located in the configuration directory. Users can contribute additional rule sets to help improve the adblocking capabilities for everyone.

# Prerequisites
## File Structure

```
Adblock-rules
├── Your_rules/
│   ├── Adblock-rules.txt          # you can add more than one rules .txt but you have to elaborate what each does
│   └── README.md                  # Information about your rules
└── README.md
```

## How to Contribute Rules

### Step 1: Clone the Assets Branch

```bash
git clone https://github.com/zynomon/onu --branch assets
cd onu/Adblock-rules
```

### Step 2: Create Your Rules Directory

```bash
mkdir Your_rules
cd Your_rules
```

### Step 3: Create Your Rules File

Create `Adblock-rules.txt` with one host per line:

```
# Comments start with #
# Add one host per line

example.com
ads.example.org
tracker.cdn.net
malicious-site.com
```

You can create multiple .txt files if needed, but each must be documented in your README.

### Step 4: Create README.md

```md
# Your Rules Name

## Description
Brief explanation of what these rules block and their purpose.

## Files

### Adblock-rules.txt
- **Purpose**: Blocks advertising and tracking domains
- **Source**: List of domains gathered from [source]
- **Count**: 1500+ domains
- **Maintenance**: Updated monthly

### [Optional] Additional-rules.txt
- **Purpose**: Blocks malware domains
- **Source**: [source]
- **Count**: 500+ domains

## License
[Choose a license - MIT, GPL, CC0, etc.]

## Contributors
- Your Name/Handle - Initial rules

## Notes
Any additional information about updating, maintaining, or using these rules.
```

## Rule Format Guidelines

- One domain per line
- Comments start with `#`
- No wildcards or regex (host-based blocking only)
- Include only the domain (e.g., `ads.example.com` not `https://ads.example.com/path`)
- Subdomains must be listed separately if needed

## Testing Your Rules

1. Copy your rules to the adblock configuration:

**Linux:**
```bash
cp Adblock-rules.txt ~/.local/share/Onu/Conf/blocked_hosts.txt
```

**Windows:**
```cmd
copy Adblock-rules.txt %APPDATA%\Onu\Conf\blocked_hosts.txt
```

**macOS:**
```bash
cp Adblock-rules.txt ~/Library/Application\ Support/Onu/Conf/blocked_hosts.txt
```

2. Launch Onu and verify that ads are blocked on test pages.

## Submitting Your Rules

Once your rules are ready:

```bash
git add Your_rules/
git commit -m "Add Your_rules adblock rules"
git push origin assets
```

Create a pull request on GitHub for the assets branch with:
- Clear description of your rules
- Number of domains
- Update frequency
- Any sources used

## Important Notes

- The adblocker reads from `blocked_hosts.txt` in the Conf directory
- Users can combine multiple rule sets by appending to their local file
- The browser includes a default warning about performance impact with large rule sets
- Comments in the default file provide guidance for users
