#!/usr/bin/env bash

if [ "$(id -u)" != "0" ]; then
    echo "This script must be run as root"
    exit 1;
fi

export PSMOUSE_SERIO_DEV_PATH=`eval ls /dev/serio_raw*`
echo $PSMOUSE_SERIO_DEV_PATH

export PSMOUSE_SERIO_LOG_PATH=`readlink -f .`/logfile.txt
echo $PSMOUSE_SERIO_LOG_PATH

./VirtualBox-4.3.22/out/linux.amd64/release/bin/VirtualBox
