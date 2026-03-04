#!/usr/bin/env bash
set -euo pipefail

check_dep() {
    local cmd="$1" pkg="$2"
    command -v "$cmd" >/dev/null 2>&1 && return 0
    echo "[DEP] $cmd not found, attempting to install $pkg..."
    if command -v pacman >/dev/null 2>&1; then
        sudo pacman -S --noconfirm "$pkg"
    elif command -v apt-get >/dev/null 2>&1; then
        echo "[WARN] On Debian/Ubuntu - arch packages cannot be built here"
        return 1
    else
        echo "[ERROR] Cannot install $pkg: no supported package manager"
        exit 1
    fi
}
check_dep "makepkg" "pacman"

if [[ -z "${STAGE_DIR:-}" ]]; then
    SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
    source "${SCRIPT_DIR}/../CONF"
fi

WORK_DIR="${PKG_BUILD_BASE}/arch"
mkdir -p "${WORK_DIR}"

cat > "${WORK_DIR}/PKGBUILD" <<EOF
pkgname=${PKG_NAME}
pkgver=${PKG_VERSION}
pkgrel=${PKG_RELEASE}
pkgdesc="${PKG_SUMMARY}"
arch=('x86_64')
url="${PKG_URL}"
license=('${PKG_LICENSE}')
depends=(${ARCH_DEPENDS})
maintainer='${PKG_MAINTAINER}'

package() {
    cp -a "${STAGE_DIR}/." "\${pkgdir}/"
}
EOF

[[ -f "${DBS_DIR}/../linux/arch/onu.install" ]] && cp "${DBS_DIR}/../linux/arch/onu.install" "${WORK_DIR}/"

cd "${WORK_DIR}"
makepkg --cleanbuild --skipinteg --noconfirm --pkgdest "${PKG_OUT}"
echo "[ARCH] Package created in: ${PKG_OUT}"