#!/bin/bash

PATH=/usr/bin:/bin

ICONLOCATION="/usr/share/icons/hicolor/256x256/apps/"
MENULOCATION="/usr/share/applications/"
LINKLOCATION="/usr/bin/"
EXECLOCATION="/opt/dudegui/"


sudo -v # just to get credentials

echo -n -e "\nRemoving application files... ";

# -------- uninstall "desktop" file --------
sudo xdg-desktop-menu uninstall dudegui.desktop --novendor --mode system

# -------- remove icon --------
if [ ! -d "${ICONLOCATION}" ]; then
        sudo rm ${ICONLOCATION}/dudegui.png
fi

# -------- remove links --------
sudo rm /usr/bin/dudegui
sudo rm /usr/bin/dudegui-uninstall

# -------- remove aplication --------
if [ ! -d "${EXECLOCATION}" ]; then
        sudo rm -fr ${EXECLOCATION}
fi

echo "DONE";
