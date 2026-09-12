# iwlwifi `lar_disable`

Restores the `lar_disable` module parameter that `iwlwifi` used to ship, so an
Intel adapter stops claiming a self-managed regulatory domain and the kernel
regulatory database applies instead. Without this every 5GHz channel stays
flagged `no IR` and cannot host an access point.

**Read [docs/howto/intel-5ghz-lar.md](../../docs/howto/intel-5ghz-lar.md) first** -
it explains how to tell whether this is actually your problem, and covers the
Secure Boot key enrollment that the install step depends on.

Only applies to adapters driven by **`iwlmvm`**, which is the sole operating-mode
module that gates the self-managed regulatory domain behind a LAR test. `iwlmld`
(Wi-Fi 7) sets the flag unconditionally and `iwldvm` (older Centrino) never sets
it at all, so neither is helped by this. Check yours with:

```
lsmod | grep -E '^iwl(dvm|mvm|mld)'
```

`build.sh` refuses to run on the unsupported ones.

```
./build.sh                    # patch, build and sign against the running kernel
sudo COUNTRY=XX ./install.sh  # install and enable, XX = your country code
sudo ./uninstall.sh           # revert
```

| File | Purpose |
| --- | --- |
| `dkms/patches/lar_disable.patch` | the driver change, two files under `drivers/net/wireless/intel/iwlwifi/mvm` |
| `build.sh` | fetches kernel source, applies the patch, builds and signs `iwlmvm.ko` |
| `install.sh` | installs into `/lib/modules/<ver>/updates/` and writes the modprobe options |
| `uninstall.sh` | removes both and restores the distribution module |
| `dkms/` | the same patch as a DKMS module, rebuilt automatically on kernel upgrades |

Build artefacts and the generated signing key live in
`/var/lib/linux-wifi-hotspot/lar`. The key is never stored in this repository.

This has to be repeated after a kernel upgrade; the enrolled signing key
persists, so only `build.sh` and `install.sh` need re-running.

## Installing via DKMS (Ubuntu)

This is an alternative to the `build.sh`/`install.sh` flow which requires no
repeated manual build steps on a kernel upgrade.  `dkms/` is a DKMS module that
fetches the driver source from the kernel's apt source and patches it. Kernels
older than 6.11 are skipped (`BUILD_EXCLUSIVE_KERNEL_MIN` in `dkms.conf`).

DKMS signs the module with the distribution's MOK key. If you use Secure Boot,
the key should be enrolled automatically on the first build (and then never
again). If that didn't happen (e.g. the install was non-interactive) run `sudo
update-secureboot-policy --enroll-key` and reboot once.

To install, run:
```
sudo apt install dkms dpkg-dev
sudo apt install linux-headers-generic  # headers for your kernel flavor (generic, generic-hwe-*, lowlatency, ...)
# ensure source repositories (deb-src) are enabled, e.g. activate them in
# /etc/apt/sources.list or /etc/apt/sources.list.d/ubuntu.sources, then:
sudo apt-get update
sudo dkms install util/iwlwifi-lar-disable/dkms
```

Turn on `lar_disable` and optionally pin your country:
`echo 'options iwlmvm lar_disable=1' | sudo tee /etc/modprobe.d/iwlmvm-lar-disable.conf`
`echo 'options cfg80211 ieee80211_regdom=XX' | sudo tee /etc/modprobe.d/cfg80211-regdom.conf`

Revert with `sudo dkms remove iwlwifi-lar-disable/1.0 --all`, then delete
`/usr/src/iwlwifi-lar-disable-1.0`,
`/etc/modprobe.d/iwlmvm-lar-disable.conf` and
`/etc/modprobe.d/cfg80211-regdom.conf`.
