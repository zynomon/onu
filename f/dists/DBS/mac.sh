#!/usr/bin/env bash
set -euo pipefail

check_dep() {
    local cmd="$1" pkg="$2"
    command -v "$cmd" >/dev/null 2>&1 && return 0
    echo "[DEP] $cmd not found: $pkg"
    return 1
}
check_dep "hdiutil" "macOS required" || exit 1

if [[ -z "${STAGE_DIR:-}" ]]; then
    SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
    source "${SCRIPT_DIR}/../CONF"
fi

WORK_DIR="${PKG_BUILD_BASE}/mac"
APP_BUNDLE="${WORK_DIR}/${PKG_NAME}.app"
CONTENTS="${APP_BUNDLE}/Contents"
mkdir -p "${CONTENTS}"/{MacOS,Resources}

cp "${STAGE_DIR}/usr/bin/${PKG_NAME}" "${CONTENTS}/MacOS/${PKG_NAME}"
chmod +x "${CONTENTS}/MacOS/${PKG_NAME}"

if [[ -f "${DBS_DIR}/../mac/Info.plist" ]]; then
    sed -e "s/@PKG_VERSION@/${PKG_VERSION}/g" -e "s/@PKG_NAME@/${PKG_NAME}/g" \
        "${DBS_DIR}/../mac/Info.plist" > "${CONTENTS}/Info.plist"
fi

DMG_FILE="${PKG_OUT}/${PKG_NAME}-${PKG_VERSION}.dmg"
hdiutil create -volname "${PKG_NAME}" -srcfolder "${APP_BUNDLE}" -ov -format UDZO "${DMG_FILE}"
echo "[MAC] DMG created: ${DMG_FILE}"