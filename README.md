## Linux Wifi Hotspot

### Features
 
* Share your wifi like in Windows - Share wifi on same interface while you are connected to a network.
* Includes Both command line and gui.
* Support both 2.4GHz and 5GHz (Need to compatible with your wifi adapter). Ex: You have connected to 5GHz and share connection with 2.4GHz.
* Select Channel to share.

### Dependencies

#### General
* bash (to run this script)
* util-linux (for getopt)
* procps or procps-ng
* hostapd
* iproute2
* iw
* iwconfig (you only need this if 'iw' can not recognize your adapter)
* haveged (optional)

#### For 'NATed' or 'None' Internet sharing method
* dnsmasq
* iptables

#### For building from source

* cmake
* make
* gcc and g++
* build-essential
* pkg-config
* gtk



## Installation
    git clone https://github.com/lakinduakash/linux-wifi-hotspot
    cd linux-wifi-hotspot
    make
    sudo make install
    
    
## Uninstallation
    sudo make uninstall