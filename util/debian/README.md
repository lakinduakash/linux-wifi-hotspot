# Debian Build Docker Image

The Dockerfile in this directory can be used to build the debian package for this project.

1. `docker build . -t linux-wifi-hotspot-deb`
2. `docker run -it linux-wifi-hotspot-deb`
3. `cd /root`

All build artifacts will be located in the `/root` directory.

4. Use `docker cp` to copy the desired artifacts out of the container.
