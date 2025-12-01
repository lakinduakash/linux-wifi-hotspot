## Linux Wifi Hotspot

<!-- [![Build Status](https://travis-ci.com/lakinduakash/linux-wifi-hotspot.svg?branch=master)](https://travis-ci.com/lakinduakash/linux-wifi-hotspot) -->
![Build](https://github.com/lakinduakash/linux-wifi-hotspot/actions/workflows/build.yml/badge.svg)
<!--[![Gitter](https://badges.gitter.im/linux-wihotspot/community.svg)](https://gitter.im/linux-wihotspot/community?utm_source=badge&utm_medium=badge&utm_campaign=pr-badge) -->
[![FOSSA Status](https://app.fossa.com/api/projects/git%2Bgithub.com%2Flakinduakash%2Flinux-wifi-hotspot.svg?type=shield)](https://app.fossa.com/projects/git%2Bgithub.com%2Flakinduakash%2Flinux-wifi-hotspot?ref=badge_shield)


### What's new
* Use aa-complain instead of complain to fix the permission issue for dnsmasq
* Fix some 5Ghz band not working issue
* Compatible with iw 6.7

#### Project Update

Hi everyone — I've been inactive on this project for a while due to other commitments. I'm now actively looking for contributors and maintainers to help keep the project alive and growing.

If you're interested in contributing — whether it's fixing bugs, improving documentation, or developing new features — please open an issue to introduce yourself.

I'm also open to adding trusted collaborators with commit access who demonstrate consistent contributions and interest.

Thanks for being part of the community and keeping Linux-WiFi-Hotspot going!

### Features

* Share your wifi like in Windows - Use wifi and enable hotspot at the same time.
* Share a wifi access point from any network interface
* [Create a hotspot with VPN](#vpn-hotspot) - The hotspot has the traffic tunnelled through VPN. Useful for devices with no VPN app support like TV or gaming consoles.
* Share wifi via QR code
* MAC filter
* View connected devices
* Includes Both command line and GUI.
* Support both 2.4GHz and 5GHz (Need to be compatible with your wifi adapter). Ex: You have connected to the 5GHz network and share a connection with 2.4GHz.
* Customise wifi Channel, Change MAC address, etc.
* Hide SSID
* customize gateway IP address
* Enable IEEE 80211n, IEEE 80211ac and IEEE 80211ax modes
* Experimental headless web dashboard powered by `hotspotd` + Svelte UI

![screenshot](docs/sc4.png)


### Command line help and documentation

Read [Command line help and documentation here](src/scripts/README.md).

If you only need the command line without GUI run `make install-cli-only` as the root user.

### Notes

- Sometimes there are troubles with **5Ghz bands** due to some vendor restrictions. If you cannot start the hotspot while you are connected to the 5Ghz band, Unselect **Auto** and select **2.4Ghz** in frequency selection.

- If any problems with **RealTeK Wifi Adapters** see [this](docs/howto/realtek.md)

- **Unable to allocate IP: firewalld issue:** Please check for potential fixes: [#209](https://github.com/lakinduakash/linux-wifi-hotspot/issues/209) [#166](https://github.com/lakinduakash/linux-wifi-hotspot/issues/166)

## Installation

#### Debian/Ubuntu

Download the Debian package from the latest [release](https://github.com/lakinduakash/linux-wifi-hotspot/releases/latest)

**OR**
Good news! I was able to restore keys, new versions will be available via the PPA
```bash
sudo add-apt-repository ppa:lakinduakash/lwh
sudo apt update
sudo apt install linux-wifi-hotspot

```

#### Arch based distributions

Linux Wifi Hotspot is available as an [AUR package](https://aur.archlinux.org/packages/linux-wifi-hotspot/). You can install it manually or with your favorite AUR helper.
For example, if you use `yay` you can do:
`yay -S linux-wifi-hotspot`

### Fedora based distributions
copr based repo is available for Fedora 
```bash
sudo dnf copr enable zinix01/linux-wifi-hotspot
sudo dnf install linux-wifi-hotspot 
```

## Dependencies

#### General
* bash
* util-linux (for getopt)
* procps or procps-ng
* hostapd
* iproute2
* iw
* iwconfig (you only need this if 'iw' can not recognize your adapter)
* haveged (optional)

_Make sure you have those dependencies by typing them in terminal. If any of dependencies fail
install it using your distro's package manager_

#### For 'NATed' or 'None' Internet sharing method
* dnsmasq
* iptables

#### To build from source

* make
* gcc and g++
* build-essential
* pkg-config
* gtk
* libgtk-3-dev
* libqrencode-dev (for qr code generation)
* libpng-dev (for qr code generation)
* Rust toolchain (cargo) for the new `hotspotd` helper

On Ubuntu or Debian install dependencies by,

```bash
sudo apt install -y libgtk-3-dev build-essential gcc g++ pkg-config make hostapd libqrencode-dev libpng-dev
```

On Fedora/CentOS/Red Hat Enterprise Linux/Rocky Linux/Oracle Linux
```bash
sudo dnf install -y gtk3-devel gcc gcc-c++ kernel-devel pkg-config make hostapd qrencode-devel libpng-devel
```

> Note: if `cargo` is installed in your user profile (e.g., `~/.cargo/bin`), make sure it is available when running `sudo make install`, e.g., `CARGO="$(which cargo)" sudo -E make install`.

## Installation

    git clone https://github.com/lakinduakash/linux-wifi-hotspot
    cd linux-wifi-hotspot

    #build binaries
    make

    #install
    # install (requires Rust/cargo for hotspotd build)
    # Example: CARGO="$(which cargo)" sudo -E make install
    sudo make install

## Uninstallation
    sudo make uninstall

## Running
You can launch the GUI by searching for "Wifi Hotspot" in the Application Menu
or using the terminal with:

    wihotspot

<h2 id="vpn-hotspot">Create VPN Hotspot</h2>

After connecting to VPN, Open `wihotspot` GUI. Select the virtual interface created by the VPN. In this case it is `tun0`

![image](docs/vpn.png)




## Run on Startup
The `wihotspot` GUI uses `create_ap` to create and manage access points. This service and core logic were originally created by
[@oblique](http://github.com/oblique), and are now maintained in this
repository.

Start the hotspot service on startup (using your saved configuration) with:

    systemctl enable create_ap

## Headless Web Management (Experimental)

Work is in progress to add a Rust-based helper daemon (`hotspotd`) and a
lightweight web UI so that linux-wifi-hotspot can be configured without the GTK
app. The helper exposes authenticated APIs for editing `/etc/create_ap.conf`,
starting/stopping the hotspot service, and talking to NetworkManager to select
the upstream Wi-Fi network when a single dongle acts as both client and AP. It
can also stream `create_ap` logs and list connected station details so the web
dashboard can provide live status.

When the kit boots for the first time, `hotspotd` can seed `/etc/create_ap.conf`
with a default SSID/passphrase defined via environment variables (see
`docs/web-service.md`) so you have predictable credentials before opening the
web UI.

If you expose the helper over HTTP (instead of the default UNIX socket), set a
`HOTSPOTD_API_TOKEN` value so the web dashboard must present
`Authorization: Bearer <token>` for every request.
To allow the browser-based web UI to reach the daemon, also set
`HTTP_LISTEN=127.0.0.1:8085` (or any host:port) inside
`/etc/linux-wifi-hotspot/hotspotd.env` before restarting `hotspotd.service`.
The API token you configure here is what the web dashboard prompts for on load.

For development you can run the Svelte dashboard directly:

```
cd webui
npm install
npm run dev -- --host 127.0.0.1 --port 5181
```

Then browse to `http://127.0.0.1:5181/`, paste the API token, and you can edit
hotspot settings, monitor `create_ap` logs, and manage NetworkManager profiles
(including creating hidden SSID entries) entirely from the browser. For
production, `npm run build` emits static assets under `webui/dist` that you can
serve from any HTTP server or bundle into a future `hotspotd` release.
You can build the dashboard without installing it by running `make webui-build`
in the repo root (requires Node/npm).

See [docs/web-service.md](docs/web-service.md) for the architecture and planned
API surface.

`sudo make install` builds and installs both the legacy GUI binaries and the new
`hotspotd` helper (plus its systemd unit) under `/usr/lib/linux-wifi-hotspot/`.
Configure `/etc/linux-wifi-hotspot/hotspotd.env` to set default SSIDs, the API
token, and optional managed services (e.g., dragonsync) that the future web UI
can start/stop via the REST API.




## Contributing

If you found a bug or you have an idea about improving this make an issue. Even a small contribution makes the open source world more beautiful.
Please read [CONTRIBUTING.md](CONTRIBUTING.md) for more info.

## Disclaimer
<div>Icons made by <a href="https://www.freepik.com" title="Freepik">Freepik</a> from <a href="https://www.flaticon.com/" title="Flaticon">www.flaticon.com</a></div>


## Stargazers over time

[![Stargazers over time](https://starchart.cc/lakinduakash/linux-wifi-hotspot.svg)](https://starchart.cc/lakinduakash/linux-wifi-hotspot)


## License
FreeBSD

Copyright (c) 2013, oblique

Copyright (c) 2024, lakinduakash


[![FOSSA Status](https://app.fossa.com/api/projects/git%2Bgithub.com%2Flakinduakash%2Flinux-wifi-hotspot.svg?type=large)](https://app.fossa.com/projects/git%2Bgithub.com%2Flakinduakash%2Flinux-wifi-hotspot?ref=badge_large)
