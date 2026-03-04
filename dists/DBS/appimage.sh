#!/usr/bin/env bash
set -e

: "${PROJECT_ROOT:?PROJECT_ROOT not set}"
: "${PKG_BUILD_BASE:?PKG_BUILD_BASE not set}"
: "${STAGE_DIR:?STAGE_DIR not set}"
: "${PKG_VERSION:?PKG_VERSION not set}"
: "${PKG_NAME:?PKG_NAME not set}"
: "${PKG_OUT:?PKG_OUT not set}"

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
TOOLS_DIR="${SCRIPT_DIR}/../tools"
APPIMAGETOOL="${TOOLS_DIR}/appimagetool"

APPDIR="${PKG_BUILD_BASE}/AppDir"
DOWNLOAD_DIR="${PROJECT_ROOT}/dists/linux/appimage"

mkdir -p "${TOOLS_DIR}"

if [ ! -f "${APPIMAGETOOL}" ]; then
    echo "appimagetool not found in ${TOOLS_DIR}. Downloading..."
    APPIMAGETOOL_URL="https://github.com/AppImage/AppImageKit/releases/download/continuous/appimagetool-x86_64.AppImage"
    wget -q --show-progress -O "${APPIMAGETOOL}" "${APPIMAGETOOL_URL}"
    chmod +x "${APPIMAGETOOL}"
else
    echo "Found appimagetool at ${APPIMAGETOOL}"
    chmod +x "${APPIMAGETOOL}"
fi

rm -rf "${APPDIR}"
mkdir -p "${APPDIR}"

# Detect system package manager to determine dependency type
detect_package_manager() {
    if command -v apt-get &> /dev/null; then
        echo "deb"
    elif command -v dnf &> /dev/null || command -v yum &> /dev/null || command -v rpm &> /dev/null; then
        echo "rpm"
    elif command -v pacman &> /dev/null; then
        echo "arch"
    else
        echo "unknown"
    fi
}

PKG_MANAGER=$(detect_package_manager)
echo "Detected package manager type: ${PKG_MANAGER}"

if [ ! -d "${DOWNLOAD_DIR}/usr/lib/x86_64-linux-gnu" ] || [ -z "$(ls -A "${DOWNLOAD_DIR}/usr/lib/x86_64-linux-gnu"/*.so* 2>/dev/null)" ]; then
    echo "Downloading Qt6 libraries to ${DOWNLOAD_DIR}..."
    
    # Check if DEB_DEPENDS or RPM_DEPENDS are set
    if [ "${PKG_MANAGER}" = "deb" ] && [ -n "${DEB_DEPENDS}" ]; then
        echo "Using DEB dependencies: ${DEB_DEPENDS}"
        
        # Parse DEB_DEPENDS (comma-separated list)
        IFS=',' read -ra DEB_PACKAGES <<< "${DEB_DEPENDS}"
        # Trim whitespace from package names
        for i in "${!DEB_PACKAGES[@]}"; do
            DEB_PACKAGES[$i]=$(echo "${DEB_PACKAGES[$i]}" | xargs)
        done
        
        ARCH="amd64"
        TEMP_DEB_DIR="/tmp/qt6-debs-$$"
        mkdir -p "${TEMP_DEB_DIR}"
        mkdir -p "${DOWNLOAD_DIR}/usr/lib/x86_64-linux-gnu"
        mkdir -p "${DOWNLOAD_DIR}/usr/share/qt6"
        
        if ! command -v apt-get &> /dev/null; then
            echo "[ERROR] apt-get not found. Cannot download DEB packages."
            exit 1
        fi
        
        if ! command -v dpkg-deb &> /dev/null; then
            echo "[ERROR] dpkg-deb not found. Cannot extract DEB packages."
            exit 1
        fi
        
        cd "${TEMP_DEB_DIR}"
        
        for pkg in "${DEB_PACKAGES[@]}"; do
            echo "Processing DEB package: ${pkg}..."
            
            if ! apt-get download -q "${pkg}"; then
                echo "  Warning: Failed to download ${pkg}"
                continue
            fi
            
            deb_file=$(find "${TEMP_DEB_DIR}" -name "${pkg}_*_${ARCH}.deb" | head -n1)
            
            if [ -z "$deb_file" ] || [ ! -f "$deb_file" ]; then
                echo "  Warning: Failed to find downloaded ${pkg}"
                continue
            fi
            
            echo "  Downloaded: $(basename "$deb_file")"
            
            extract_dir="${TEMP_DEB_DIR}/extract/${pkg}"
            mkdir -p "${extract_dir}"
            
            if ! dpkg-deb -x "$deb_file" "${extract_dir}"; then
                echo "  Warning: Failed to extract ${pkg}"
                continue
            fi
            
            if [ -d "${extract_dir}/usr/lib/x86_64-linux-gnu" ]; then
                cp -a "${extract_dir}/usr/lib/x86_64-linux-gnu/"*.so* "${DOWNLOAD_DIR}/usr/lib/x86_64-linux-gnu/"
                echo "    Copied libraries from /usr/lib/x86_64-linux-gnu"
            fi
            
            if [ -d "${extract_dir}/usr/share/qt6" ]; then
                cp -a "${extract_dir}/usr/share/qt6/." "${DOWNLOAD_DIR}/usr/share/qt6/"
                echo "    Copied Qt6 data files"
            fi
            
            echo "  Done with ${pkg}"
        done
        
        if [ -d "${DOWNLOAD_DIR}/usr/lib/x86_64-linux-gnu" ]; then
            cd "${DOWNLOAD_DIR}/usr/lib/x86_64-linux-gnu"
            echo "Creating library symlinks..."
            
            for lib in *.so.*; do
                if [[ -f "$lib" ]] && [[ "$lib" =~ \.so\.[0-9]+$ ]]; then
                    base_name="${lib%%.so.*}.so"
                    if [ ! -f "$base_name" ] && [ ! -L "$base_name" ]; then
                        ln -sf "$lib" "$base_name"
                        echo "  Created symlink: $base_name -> $lib"
                    fi
                fi
            done
        fi
        
    elif [ "${PKG_MANAGER}" = "rpm" ] && [ -n "${RPM_DEPENDS}" ]; then
        echo "Using RPM dependencies: ${RPM_DEPENDS}"
        
        # Parse RPM_DEPENDS (comma-separated list)
        IFS=',' read -ra RPM_PACKAGES <<< "${RPM_DEPENDS}"
        # Trim whitespace from package names
        for i in "${!RPM_PACKAGES[@]}"; do
            RPM_PACKAGES[$i]=$(echo "${RPM_PACKAGES[$i]}" | xargs)
        done
        
        TEMP_RPM_DIR="/tmp/qt6-rpms-$$"
        mkdir -p "${TEMP_RPM_DIR}"
        mkdir -p "${DOWNLOAD_DIR}/usr/lib64"
        mkdir -p "${DOWNLOAD_DIR}/usr/lib"
        mkdir -p "${DOWNLOAD_DIR}/usr/share/qt6"
        
        # Check for required tools
        if command -v dnf &> /dev/null; then
            DOWNLOAD_CMD="dnf download"
        elif command -v yum &> /dev/null; then
            DOWNLOAD_CMD="yumdownloader"
            if ! command -v yumdownloader &> /dev/null; then
                echo "Installing yum-utils for yumdownloader..."
                sudo yum install -y yum-utils
            fi
        else
            echo "[ERROR] Neither dnf nor yum found. Cannot download RPM packages."
            exit 1
        fi
        
        if ! command -v rpm2cpio &> /dev/null || ! command -v cpio &> /dev/null; then
            echo "[ERROR] rpm2cpio or cpio not found. Please install rpm-build or cpio."
            exit 1
        fi
        
        cd "${TEMP_RPM_DIR}"
        
        for pkg in "${RPM_PACKAGES[@]}"; do
            echo "Processing RPM package: ${pkg}..."
            
            # Download the RPM package
            if ! ${DOWNLOAD_CMD} "${pkg}"; then
                echo "  Warning: Failed to download ${pkg}"
                continue
            fi
            
            rpm_file=$(find "${TEMP_RPM_DIR}" -name "${pkg}-*.rpm" | head -n1)
            
            if [ -z "$rpm_file" ] || [ ! -f "$rpm_file" ]; then
                # Try with arch suffix
                rpm_file=$(find "${TEMP_RPM_DIR}" -name "${pkg}*.x86_64.rpm" | head -n1)
            fi
            
            if [ -z "$rpm_file" ] || [ ! -f "$rpm_file" ]; then
                echo "  Warning: Failed to find downloaded RPM for ${pkg}"
                continue
            fi
            
            echo "  Downloaded: $(basename "$rpm_file")"
            
            # Extract the RPM package
            extract_dir="${TEMP_RPM_DIR}/extract/${pkg}"
            mkdir -p "${extract_dir}"
            
            if ! (cd "${extract_dir}" && rpm2cpio "${rpm_file}" | cpio -idm); then
                echo "  Warning: Failed to extract ${pkg}"
                continue
            fi
            
            # Copy libraries (RPMs typically use lib64 on x86_64)
            if [ -d "${extract_dir}/usr/lib64" ]; then
                cp -a "${extract_dir}/usr/lib64/"*.so* "${DOWNLOAD_DIR}/usr/lib64/"
                echo "    Copied libraries from /usr/lib64"
            fi
            
            if [ -d "${extract_dir}/usr/lib" ]; then
                cp -a "${extract_dir}/usr/lib/"*.so* "${DOWNLOAD_DIR}/usr/lib/"
                echo "    Copied libraries from /usr/lib"
            fi
            
            # Copy Qt6 data files
            if [ -d "${extract_dir}/usr/share/qt6" ]; then
                cp -a "${extract_dir}/usr/share/qt6/." "${DOWNLOAD_DIR}/usr/share/qt6/"
                echo "    Copied Qt6 data files"
            fi
            
            echo "  Done with ${pkg}"
        done
        
        # Create a unified structure (link lib64 to lib for compatibility)
        if [ -d "${DOWNLOAD_DIR}/usr/lib64" ]; then
            mkdir -p "${DOWNLOAD_DIR}/usr/lib"
            cd "${DOWNLOAD_DIR}/usr/lib64"
            for lib in *.so*; do
                if [ -f "$lib" ] || [ -L "$lib" ]; then
                    ln -sf "../lib64/$lib" "${DOWNLOAD_DIR}/usr/lib/$lib"
                fi
            done
        fi
        
    elif [ "${PKG_MANAGER}" = "arch" ] && [ -n "${ARCH_DEPENDS}" ]; then
        echo "Arch Linux detected. Please manually install required packages: ${ARCH_DEPENDS}"
        echo "Skipping automatic library download."
        exit 1
    else
        echo "[ERROR] Unsupported package manager or missing dependency variables."
        echo "DEB_DEPENDS: ${DEB_DEPENDS:-not set}"
        echo "RPM_DEPENDS: ${RPM_DEPENDS:-not set}"
        exit 1
    fi
    
    rm -rf "${TEMP_DEB_DIR:-${TEMP_RPM_DIR}}"
    echo "Qt6 libraries downloaded and extracted to ${DOWNLOAD_DIR}"
fi

if [ -d "${STAGE_DIR}/usr" ]; then
    cp -a "${STAGE_DIR}/usr/." "${APPDIR}/usr/"
else
    echo "[ERROR] STAGE_DIR/usr not found: ${STAGE_DIR}/usr"
    exit 1
fi

if [ -d "${DOWNLOAD_DIR}/usr" ]; then
    cp -a "${DOWNLOAD_DIR}/usr/." "${APPDIR}/usr/"
    echo "Copied Qt6 libraries from ${DOWNLOAD_DIR}"
fi

if [ -f "${DOWNLOAD_DIR}/usr/lib/trigonometry/libcrash.so" ]; then
    mkdir -p "${APPDIR}/usr/lib/trigonometry"
    cp -a "${DOWNLOAD_DIR}/usr/lib/trigonometry/." "${APPDIR}/usr/lib/trigonometry/"
    echo "Copied Trig::crash library"
fi

DESKTOP_SRC="${APPDIR}/usr/share/applications/${PKG_NAME}.desktop"
if [ ! -f "${DESKTOP_SRC}" ] && [ -f "${PROJECT_ROOT}/dists/onu.desktop.in" ]; then
    mkdir -p "${APPDIR}/usr/share/applications"
    cp "${PROJECT_ROOT}/dists/onu.desktop.in" "${DESKTOP_SRC}"
    echo "Copied desktop file from project root"
fi

if [ ! -f "${DESKTOP_SRC}" ]; then
    echo "[ERROR] Desktop file not found: ${DESKTOP_SRC}"
    exit 1
fi

ICON_SRC="${APPDIR}/usr/share/icons/hicolor/scalable/apps/${PKG_NAME}.svg"
ICON_DIR="$(dirname "${ICON_SRC}")"
mkdir -p "${ICON_DIR}"

if [ -f "${DOWNLOAD_DIR}/usr/share/icons/hicolor/scalable/apps/${PKG_NAME}.svg" ]; then
    cp "${DOWNLOAD_DIR}/usr/share/icons/hicolor/scalable/apps/${PKG_NAME}.svg" "${ICON_SRC}"
    echo "Copied SVG icon from download dir"
elif [ -f "${PROJECT_ROOT}/dists/icons/${PKG_NAME}.svg" ]; then
    cp "${PROJECT_ROOT}/dists/icons/${PKG_NAME}.svg" "${ICON_SRC}"
    echo "Copied SVG icon from project dists/icons"
elif [ -f "${STAGE_DIR}/usr/share/icons/hicolor/scalable/apps/${PKG_NAME}.svg" ]; then
    cp "${STAGE_DIR}/usr/share/icons/hicolor/scalable/apps/${PKG_NAME}.svg" "${ICON_SRC}"
    echo "Copied SVG icon from stage dir"
fi

if [ ! -f "${ICON_SRC}" ]; then
    echo "[ERROR] SVG icon not found at ${ICON_SRC}"
    exit 1
fi

DESKTOP_FILE=$(basename "${DESKTOP_SRC}")
ICON_FILE=$(basename "${ICON_SRC}")
cp "${DESKTOP_SRC}" "${APPDIR}/${DESKTOP_FILE}"
cp "${ICON_SRC}" "${APPDIR}/${ICON_FILE}"

sed -i 's/^Version=.*/Version=1.0/' "${APPDIR}/${DESKTOP_FILE}"
echo "X-AppImage-Version=${PKG_VERSION}" >> "${APPDIR}/${DESKTOP_FILE}"

if [ ! -f "${APPDIR}/AppRun" ] && [ -f "${APPDIR}/usr/bin/${PKG_NAME}" ]; then
    ln -sf "usr/bin/${PKG_NAME}" "${APPDIR}/AppRun"
    chmod +x "${APPDIR}/usr/bin/${PKG_NAME}"
fi

if [ ! -f "${APPDIR}/usr/bin/${PKG_NAME}" ] && [ ! -f "${APPDIR}/AppRun" ]; then
    echo "[ERROR] Binary ${PKG_NAME} missing!"
    exit 1
fi

mkdir -p "${PKG_OUT}"
FINAL_FILE="${PKG_OUT}/${PKG_NAME}-${PKG_VERSION}-x86_64.AppImage"

echo "Creating AppImage..."
"${APPIMAGETOOL}" "${APPDIR}" "${FINAL_FILE}"

if [ -f "${FINAL_FILE}" ]; then
    FILE_SIZE=$(stat -c%s "${FINAL_FILE}" 2>/dev/null || stat -f%z "${FINAL_FILE}" 2>/dev/null)
    if [ "${FILE_SIZE}" -gt 100000 ]; then
        echo "[SUCCESS] ${FINAL_FILE} ($(du -sh "${FINAL_FILE}" | awk '{print $1}'))"
    else
        echo "[ERROR] AppImage too small (${FILE_SIZE} bytes)."
        exit 1
    fi
else
    echo "[ERROR] AppImage not created: ${FINAL_FILE}"
    exit 1
fi