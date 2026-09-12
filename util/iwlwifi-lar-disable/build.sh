#!/bin/bash
# Build (and, under Secure Boot, sign) an iwlmvm module that understands a
# lar_disable parameter. See docs/howto/intel-5ghz-lar.md for the background.
#
#   ./build.sh                 build against the running kernel
#   KERNEL_SRC=/path ./build.sh    use an already unpacked kernel tree
set -euo pipefail

SELF_DIR=$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)
PATCH="$SELF_DIR/dkms/patches/lar_disable.patch"
KVER=$(uname -r)
STATE_DIR=/var/lib/linux-wifi-hotspot/lar
WORK_DIR=${WORK_DIR:-/var/tmp/lwh-lar-build}
BUILD_DIR=/lib/modules/$KVER/build
IWL_SUBDIR=drivers/net/wireless/intel/iwlwifi

SUDO=
[[ $EUID -ne 0 ]] && SUDO=sudo

die() { echo "ERROR: $*" >&2; exit 1; }
say() { echo "==> $*"; }

# mokutil exits non-zero when it cannot read the kernel trusted keyring even
# though the answer on stdout is correct, so match the text and ignore status.
secure_boot_on() {
    local out; out=$(mokutil --sb-state 2>/dev/null || true)
    [[ "$out" == *"SecureBoot enabled"* ]]
}
mok_enrolled() {
    local out; out=$(mokutil --test-key "$1" 2>/dev/null || true)
    [[ "$out" == *"is already enrolled"* ]]
}


[[ -f "$PATCH" ]] || die "missing $PATCH"
[[ -d "$BUILD_DIR" ]] || die "no kernel build tree at $BUILD_DIR - install the headers for $KVER
       Debian/Ubuntu: apt install linux-headers-$KVER
       Arch:          pacman -S linux-headers
       Fedora:        dnf install kernel-devel-$KVER"

# Only iwlmvm gates REGULATORY_WIPHY_SELF_MANAGED behind a LAR test, so it is
# the only operating-mode module this patch can hook into. Fail early rather
# than build a module the machine will never use. FORCE=1 to build anyway, e.g.
# when preparing a module for a different machine.
# grep -q exits on the first match, which SIGPIPEs the writer; under pipefail
# that makes a successful match look like a failed pipeline. Read once instead.
LSMOD=$(lsmod 2>/dev/null || true)
module_loaded() { grep -q "^$1 " <<<"$LSMOD"; }

if [[ "${FORCE:-0}" != "1" ]]; then
    if module_loaded iwlmld; then
        die "this machine uses the iwlmld driver (Wi-Fi 7 parts).
       iwl_mld_hw_set_regulatory() sets REGULATORY_WIPHY_SELF_MANAGED
       unconditionally, so there is no LAR test for lar_disable to switch off
       and this patch cannot help. See docs/howto/intel-5ghz-lar.md."
    elif module_loaded iwldvm; then
        die "this machine uses the iwldvm driver (older Centrino parts).
       Those never claim a self-managed regulatory domain, so the NO_IR problem
       this patch addresses does not apply. See docs/howto/intel-5ghz-lar.md."
    elif ! module_loaded iwlmvm; then
        die "iwlmvm is not loaded - this patch only applies to Intel iwlwifi
       adapters handled by the mvm operating mode. Check with:
           lsmod | grep -E '^iwl(dvm|mvm|mld)'
       Set FORCE=1 to build anyway."
    fi
fi

# ---------------------------------------------------------------- kernel source
if [[ -n "${KERNEL_SRC:-}" ]]; then
    SRC_ROOT=$KERNEL_SRC
    [[ -d "$SRC_ROOT/$IWL_SUBDIR" ]] || die "$SRC_ROOT does not contain $IWL_SUBDIR"
    say "using kernel source at $SRC_ROOT"
else
    mkdir -p "$WORK_DIR"
    TARBALL=$(ls /usr/src/linux-source-*.tar.* 2>/dev/null | head -1 || true)
    if [[ -z "$TARBALL" ]]; then
        if command -v apt-get >/dev/null; then
            PKG="linux-source-$(echo "$KVER" | cut -d- -f1)"
            say "installing $PKG"
            $SUDO apt-get install -y "$PKG" || die "could not install $PKG"
            TARBALL=$(ls /usr/src/linux-source-*.tar.* 2>/dev/null | head -1 || true)
        fi
    fi
    [[ -n "$TARBALL" ]] || die "no kernel source found.
       Install your distribution's kernel source package, then re-run, or point
       this script at an unpacked tree:  KERNEL_SRC=/path/to/linux ./build.sh"

    say "unpacking $IWL_SUBDIR from $TARBALL"
    ( cd "$WORK_DIR" && tar -xf "$TARBALL" --wildcards "*/$IWL_SUBDIR/*" )
    SRC_ROOT=$(find "$WORK_DIR" -maxdepth 1 -mindepth 1 -type d | head -1 || true)
    [[ -d "$SRC_ROOT/$IWL_SUBDIR" ]] || die "unexpected layout in $TARBALL"
fi

# ---------------------------------------------------------------------- patch
say "applying lar_disable.patch"
if patch -p1 -d "$SRC_ROOT" -N --dry-run < "$PATCH" >/dev/null 2>&1; then
    patch -p1 -d "$SRC_ROOT" -N < "$PATCH"
elif patch -p1 -d "$SRC_ROOT" -R --dry-run < "$PATCH" >/dev/null 2>&1; then
    say "already applied, continuing"
else
    die "lar_disable.patch does not apply to this kernel source.
       The upstream function may have changed; apply it by hand and re-run with
       KERNEL_SRC=$SRC_ROOT"
fi

# ---------------------------------------------------------------------- build
say "building iwlmvm for $KVER (this takes a few minutes)"
make -C "$BUILD_DIR" M="$SRC_ROOT/$IWL_SUBDIR" modules -j"$(nproc)"

MODULE="$SRC_ROOT/$IWL_SUBDIR/mvm/iwlmvm.ko"
[[ -f "$MODULE" ]] || die "build produced no $MODULE"
MODINFO_OUT=$(modinfo "$MODULE" 2>/dev/null || true)
[[ "$MODINFO_OUT" == *lar_disable* ]] || die "built module has no lar_disable parameter"

$SUDO mkdir -p "$STATE_DIR"
$SUDO cp "$MODULE" "$STATE_DIR/iwlmvm.ko"

# ----------------------------------------------------------------------- sign
if secure_boot_on; then
    say "Secure Boot is enabled - signing the module"
    SIGN_FILE="$BUILD_DIR/scripts/sign-file"
    [[ -x "$SIGN_FILE" ]] || die "no sign-file helper at $SIGN_FILE"

    if ! $SUDO test -f "$STATE_DIR/MOK.priv"; then
        say "generating a machine owner key in $STATE_DIR"
        $SUDO openssl req -new -x509 -newkey rsa:2048 -nodes -days 36500 \
            -keyout "$STATE_DIR/MOK.priv" -outform DER -out "$STATE_DIR/MOK.der" \
            -subj "/CN=linux-wifi-hotspot iwlwifi lar_disable/"
        $SUDO chmod 600 "$STATE_DIR/MOK.priv"
    fi

    $SUDO "$SIGN_FILE" sha256 "$STATE_DIR/MOK.priv" "$STATE_DIR/MOK.der" "$STATE_DIR/iwlmvm.ko"
    say "signed"

    if ! mok_enrolled "$STATE_DIR/MOK.der"; then
        cat <<EOF

The signing key is not enrolled yet. Enroll it before installing:

    sudo mokutil --import $STATE_DIR/MOK.der

Choose a one-time password, reboot, and complete the blue "MOK Manager"
screen: press a key at the countdown, then Enroll MOK -> Continue -> Yes ->
your password -> Reboot. Letting the countdown expire discards the request.

Then run:  sudo $SELF_DIR/install.sh
EOF
        exit 0
    fi
else
    say "Secure Boot is disabled - no signature needed"
fi

echo
say "Done. Install with:  sudo $SELF_DIR/install.sh"
