# Build environment for util/k580_build.sh: the official SDCC binary (MCS51 files only) and the QMK CLI.
# SDCC 4.5.0 (the Debian trixie package) stops with an internal error on tmk_core/protocol/ch555/usb_device.c
# (SDCC bug #3894, fixed in 4.6.0). 4.4.0 also works.
# The image has only what util/k580_build.sh uses. compile.sh (the original port's link script, not used for the
# K580) does not run in it: it needs ripgrep, binutils and packihx, which are left out. git is not installed:
# qmk compile warns that the tree is not a git repo and sets QMK_VERSION to "NA", which the K580 image does not
# contain.
# util/k580_build.sh builds this image with the tree's requirements.txt as the only other file in the context,
# and tags it vial-qmk-8051-k580:sdcc-<SDCC_VER>.
ARG SDCC_VER=4.6.0

FROM debian:trixie-slim AS sdcc
ARG SDCC_VER
RUN apt-get update && apt-get install -y --no-install-recommends ca-certificates curl bzip2
RUN curl -fsSL -o /tmp/sdcc.tar.bz2 \
        "https://sourceforge.net/projects/sdcc/files/sdcc-linux-amd64/${SDCC_VER}/sdcc-${SDCC_VER}-amd64-unknown-linux2.5.tar.bz2/download" \
    && sha256sum /tmp/sdcc.tar.bz2 \
    && mkdir /tmp/sdcc && tar -xjf /tmp/sdcc.tar.bz2 -C /tmp/sdcc --strip-components=1
# Keep only the files an MCS51 build with --model-large uses.
RUN mkdir -p /opt/sdcc/bin /opt/sdcc/share/sdcc/lib && cd /tmp/sdcc \
    && cp bin/sdcc bin/sdcpp bin/sdas8051 bin/sdld /opt/sdcc/bin/ \
    && cp -r libexec /opt/sdcc/ \
    && cp -r share/sdcc/include /opt/sdcc/share/sdcc/ \
    && cp -r share/sdcc/lib/large /opt/sdcc/share/sdcc/lib/

FROM debian:trixie-slim
RUN apt-get update && apt-get install -y --no-install-recommends libstdc++6 make python3 python3-venv \
    && rm -rf /var/lib/apt/lists/*
COPY --from=sdcc /opt/sdcc /opt/sdcc
COPY requirements.txt /tmp/req.txt
RUN python3 -m venv /opt/qmk && /opt/qmk/bin/pip install --no-cache-dir qmk -r /tmp/req.txt && rm /tmp/req.txt
ENV PATH=/opt/sdcc/bin:/opt/qmk/bin:$PATH QMK_HOME=/qmk
