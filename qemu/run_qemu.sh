#!/usr/bin/env bash

if [ "$(id -u)" != "0" ]; then
    echo "This script must be run as root"
    exit 1;
fi

export PSMOUSE_SERIO_DEV_PATH=`eval ls /dev/serio_raw*`
echo $PSMOUSE_SERIO_DEV_PATH

export PSMOUSE_SERIO_LOG_PATH=`readlink -f .`/logfile.txt
echo $PSMOUSE_SERIO_LOG_PATH

./qemu-2.2.0/x86_64-softmmu/qemu-system-x86_64 \
    -hda w7_tp.qcow2 -m 2048MiB -enable-kvm -vga std
# -cdrom ~chris/doc/iso/X17-58997.iso \
