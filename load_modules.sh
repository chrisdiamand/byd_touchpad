#!/usr/bin/env bash

D=./VirtualBox-4.3.22/out/linux.amd64/release/bin/src

insmod $D/vboxdrv.ko
insmod $D/vboxpci.ko
insmod $D/vboxnetflt.ko
insmod $D/vboxnetadp.ko
