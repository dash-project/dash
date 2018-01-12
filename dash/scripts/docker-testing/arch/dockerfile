# Minimal working example on arch-linux
FROM greyltc/archlinux:latest
MAINTAINER    The DASH Team <team@dash-project.org>

RUN pacman -S --noconfirm \
      git \
      cmake \
      make \
      gcc \
      cblas \
      openmpi

WORKDIR /opt/dash
CMD ["/bin/bash"]

