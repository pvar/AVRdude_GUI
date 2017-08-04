#!/bin/bash

PATH=/usr/bin:/bin

ICONLOCATION="/usr/share/icons/hicolor/256x256/apps/"
MENULOCATION="/usr/share/applications/"
LINKLOCATION="/usr/bin/"
EXECLOCATION="/opt/dudegui/"

echo -e "\nRemoving application files...\n";

# -------- uninstall "desktop" file --------
sudo xdg-desktop-menu uninstall dudegui.desktop --novendor --mode system

# -------- remove icon --------
if [ ! -d "${ICONLOCATION}" ]; then
        sudo rm ${ICONLOCATION}/dudegui.png
fi

# -------- remove link --------
sudo rm /usr/bin/dudegui

# -------- remove aplication --------
if [ ! -d "${EXECLOCATION}" ]; then
        sudo rm -fr ${EXECLOCATION}
fi

echo -e "All clear. Hope to see you again!";
