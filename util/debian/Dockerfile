FROM ubuntu:20.04

ARG GIT_TAG=master
ARG GIT_URL=https://github.com/lakinduakash/linux-wifi-hotspot/archive/${GIT_TAG}.tar.gz

# Retrieve dependencies
RUN apt-get update && DEBIAN_FRONTEND=noninteractive apt-get install -y debhelper devscripts build-essential pkg-config libgtk-3-dev

# Clone source code
WORKDIR /root
RUN curl -L --output linux-wifi-hotspot.tar.gz --silent ${GIT_URL}
RUN tar -xzf linux-wifi-hotspot.tar.gz

WORKDIR /root/linux-wifi-hotspot-${GIT_TAG}
RUN debuild -uc -us

ENTRYPOINT ["/bin/bash"]
