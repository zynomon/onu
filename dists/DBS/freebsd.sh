#!/usr/bin/env bash
set -euo pipefail

check_dep() {
    local cmd="$1" pkg="$2"
    command -v "$cmd" >/dev/null 2>&1 && return 0
    echo "[DEP] $cmd not found: $pkg"
    return 1
}
check_dep "pkg" "FreeBSD required" || exit 1

if [[ -z "${STAGE_DIR:-}" ]]; then
    SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
    source "${SCRIPT_DIR}/../CONF"
fi

WORK_DIR="${PKG_BUILD_BASE}/bsd"
PKG_ROOT="${WORK_DIR}/stage"
mkdir -p "${PKG_ROOT}/usr"
cp -a "${STAGE_DIR}/." "${PKG_ROOT}/usr/"

cat > "${PKG_ROOT}/+MANIFEST" <<EOF
name: ${PKG_NAME}
version: ${PKG_VERSION}_${PKG_RELEASE}
origin: www/${PKG_NAME}
comment: ${PKG_SUMMARY}
maintainer: ${PKG_MAINTAINER}
www: ${PKG_URL}
prefix: /usr
EOF

cd "${PKG_ROOT}"
pkg create -r . -o "${PKG_OUT}"
echo "[BSD] Package created in: ${PKG_OUT}"