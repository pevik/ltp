#!/bin/sh -eux

apk add \
        acl \
        keyutils \
        libaio \
        libacl \
        libcap \
        libselinux \
        libsepol \
        libtirpc \
        numactl \
        openssl \
        py3-msgpack

adduser -D -g "Unprivileged LTP user" ltp
