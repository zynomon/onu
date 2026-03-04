#!/usr/bin/env bash
set -e

FOLDER_NAME="${PKG_NAME}-${PKG_VERSION}-${BUILD_TYPE}"
WORKSPACE="${PKG_BUILD_BASE}/tgz/${FOLDER_NAME}"

echo "[PKG-TGZ] Creating workspace: ${WORKSPACE}"
rm -rf "${WORKSPACE}"
mkdir -p "${WORKSPACE}"

cp -a "${STAGE_DIR}/usr/." "${WORKSPACE}/"

cat << EOF > "${WORKSPACE}/README.txt"
 ${PKG_NAME^^} v${PKG_VERSION} 
(${BUILD_TYPE})

INSTALLATION:
Installation Instructions (System-wide):
1. Binary    → Move 'bin/APP_NAME' to '/bin/'
2. Icon      → Move 'share/icons/ICON_NAME' to '/usr/share/icons/'
3. Desktop   → Move 'share/applications/APP.desktop' to '/usr/share/applications/'

Note: You can also use ~/.local/ instead of /usr/ for user-only installation:
   • Binary    → ~/.local/bin/
   • Icon      → ~/.local/share/icons/
   • Desktop   → ~/.local/share/applications/

DEPENDENCIES:

For Debian/Ubuntu (DEB):
    sudo apt install ${DEB_DEPENDS}

For Fedora/RHEL (RPM):
    sudo dnf install ${RPM_DEPENDS}

For Arch Linux:
    sudo pacman -S ${ARCH_DEPENDS}

For macOS (Homebrew):
    brew install ${MAC_FRAMEWORKS}

----------------------------------------------------------------------------------------- <!>
Built on: $(date)

EOF

FINAL_FILE="${PKG_OUT}/${FOLDER_NAME}.tar.zst"
echo "[PKG-TGZ] Compressing to ${FINAL_FILE} (Zstd -22)..."

cd "${PKG_BUILD_BASE}/tgz"

if command -v zstd >/dev/null 2>&1; then
    tar cf - "${FOLDER_NAME}" | zstd -22 --ultra -o "${FINAL_FILE}"
else
    echo "[WARN] zstd not found, falling back to gzip -9"
    tar -czf "${PKG_OUT}/${FOLDER_NAME}.tar.gz" "${FOLDER_NAME}"
fi

if [ -f "${FINAL_FILE}" ]; then
    echo "[SUCCESS] Tarball created: $(basename "${FINAL_FILE}")"
    echo "[SIZE] $(du -sh "${FINAL_FILE}" | awk '{print $1}')"
fi
