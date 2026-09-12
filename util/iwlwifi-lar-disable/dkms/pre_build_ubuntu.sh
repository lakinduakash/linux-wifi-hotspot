#!/bin/bash

set -euo pipefail

# This is a pre-build script for Ubuntu which gets the module source
# code and applies the patch.

KMOD="linux-modules-$kernelver"
SRCPKG=$(dpkg-query -W -f='${source:Package}' "$KMOD" 2>/dev/null || true)
VER=$(dpkg-query -W -f='${source:Version}' "$KMOD" 2>/dev/null || true)

echo "==> fetching iwlwifi source for kernel $kernelver"
if [[ -z $SRCPKG ]] || [[ -z $VER ]] || ! apt-get source "$SRCPKG=$VER" >/dev/null 2>&1; then
    echo "ERROR: no apt source for $SRCPKG" >&2
    echo "       enable deb-src in /etc/apt/sources.list.d/ and run: sudo apt-get update" >&2
    exit 1
fi
SRC="$SRCPKG-$(dpkg-query -W -f='${source:Upstream-Version}' "$KMOD" 2>/dev/null || true)"

mkdir -p drivers/net/wireless/intel
cp -a "$SRC/drivers/net/wireless/intel/iwlwifi" drivers/net/wireless/intel/
patch -p1 < patches/lar_disable.patch
