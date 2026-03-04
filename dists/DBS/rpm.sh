#!/usr/bin/env bash
set -euo pipefail

check_dep() {
    local cmd="$1" pkg="$2"
    command -v "$cmd" >/dev/null 2>&1 && return 0
    echo "[DEP] $cmd not found, attempting to install $pkg..."
    if command -v apt-get >/dev/null 2>&1; then
        sudo apt-get update && sudo apt-get install -y "$pkg"
    elif command -v dnf >/dev/null 2>&1; then
        sudo dnf install -y "$pkg"
    elif command -v pacman >/dev/null 2>&1; then
        sudo pacman -S --noconfirm "$pkg"
    else
        echo "[ERROR] Cannot install $pkg: no supported package manager"
        exit 1
    fi
}
check_dep "rpmbuild" "rpm-build"

if [[ -z "${STAGE_DIR:-}" ]]; then
    SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
    source "${SCRIPT_DIR}/../CONF"
fi

WORK_DIR="${PKG_BUILD_BASE}/rpm"
RPMBUILD_ROOT="${WORK_DIR}/rpmbuild"
mkdir -p "${RPMBUILD_ROOT}"/{BUILD,RPMS,SOURCES,SPECS,SRPMS}

SRC_TAR="${RPMBUILD_ROOT}/SOURCES/${PKG_NAME}-${PKG_VERSION}.tar.gz"
tar -czf "${SRC_TAR}" -C "${PROJECT_ROOT}" --exclude=build --exclude=dists/DBS/build .

SPEC_FILE="${RPMBUILD_ROOT}/SPECS/${PKG_NAME}.spec"
cat > "${SPEC_FILE}" <<EOF
Name:           ${PKG_NAME}
Version:        ${PKG_VERSION}
Release:        ${PKG_RELEASE}%{?dist}
Summary:        ${PKG_SUMMARY}
License:        ${PKG_LICENSE}
URL:            ${PKG_URL}
Requires:       ${RPM_DEPENDS}
BuildArch:      x86_64

%description
${PKG_SUMMARY}

%install
mkdir -p %{buildroot}/usr
cp -a ${STAGE_DIR}/usr/* %{buildroot}/usr/

%files
/usr/*
EOF

rpmbuild \
    --define "_topdir ${RPMBUILD_ROOT}" \
    --define "_dbpath ${RPMBUILD_ROOT}/rpmdb" \
    -bb "${SPEC_FILE}"

find "${RPMBUILD_ROOT}/RPMS" -name "*.rpm" -exec mv {} "${PKG_OUT}/" \;
echo "[RPM] Packages created in: ${PKG_OUT}"