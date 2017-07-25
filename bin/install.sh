#!/bin/bash

# ****************************************************************************
# FUNCTION DEFINITIONS
# ****************************************************************************

# ----------------------------------------------------------------------------
# function that creates file with udev rules
# ----------------------------------------------------------------------------

function udev_rules ()
{
        cat <<END_RULES
# USBasp
SUBSYSTEM=="usb", ATTRS{idVendor}=="16c0", ATTRS{idProduct}=="05dc", MODE="660", GROUP="${UDEV_GROUP}"
# USBtiny
SUBSYSTEM=="usb", ATTR{idVendor}=="1781", ATTR{idProduct}=="0c9f", MODE="660", GROUP="${UDEV_GROUP}"
# Atmel JTAG ICE mkII
ATTR{idVendor}==”03eb”, ATTR{idProduct}==”2103″, MODE=”660″, GROUP=”${UDEV_GROUP}”
# Atmel AVRISP mkII
ATTR{idVendor}==”03eb”, ATTR{idProduct}==”2104″, MODE=”660″, GROUP=”${UDEV_GROUP}”
# Atmel Dragon
ATTR{idVendor}==”03eb”, ATTR{idProduct}==”2107″, MODE=”660″, GROUP=”${UDEV_GROUP}”
END_RULES
}

# ----------------------------------------------------------------------------
# function that guesses necessary user groups, for accessing serial devices
# ----------------------------------------------------------------------------

function which_groups ()
{
        [[ -r  /etc/os-release ]] && . /etc/os-release
        case "$ID" in
                debian|ubuntu)
                        NEW_GROUPS="dialout tty"
                        UDEV_GROUP="dialout"
                        ;;
                gentoo)
                        NEW_GROUPS="tty uucp"
                        UDEV_GROUP="dialout"
                        ;;
                fedora)
                        NEW_GROUPS="dialout uucp lock"
                        UDEV_GROUP="dialout"
                        ;;
                opensuse)
                        NEW_GROUPS="dialout uucp lock"
                        UDEV_GROUP="dialout"
                        ;;
                archlinux|arch)
                        NEW_GROUPS="uucp lock"
                        UDEV_GROUP="uucp"
                        ;;
                *)
                        # will have to guess necessary groups..."
                        TTYS_GROUP=$([[ -e /dev/ttyS0 ]] && ls -lh /dev/ttyS0 | awk '{print $4}')
                        case "${TTYS_GROUP}" in
                                uucp)
                                        # suppose it's an ARCH-based distribution
                                        NEW_GROUPS="uucp lock"
                                        UDEV_GROUP="uucp"
                                        ;;
                                dialout)
                                        # suppose it's a DEBIAN-based distribution
                                        NEW_GROUPS="dialout tty"
                                        UDEV_GROUP="dialout"
                                        ;;
                                *)
                                        # don't have a clue... :-)
                                        NEW_GROUPS=${TTYS_GROUP}
                                        UDEV_GROUP=${TTYS_GROUP}
                                        ;;
                        esac
                        ;;
        esac
}

# ----------------------------------------------------------------------------
# function that checks if group (NEW_GROUP) exists
# ----------------------------------------------------------------------------

function is_group ()
{
        # check if group dialout exists
        EXISTS=1
        if [ `/usr/bin/grep -c ${NEW_GROUP} /etc/group` == "0"  ]; then
                echo "This is weird! It seems that your system lacks dialout group.";
                while true; do
                        echo "Would you like this group to be created now? [Y/n]"
                        read -s -n 1 USR_RESPONSE
                        case ${USR_REPLY} in
                                [Yy]|"" )
                                        echo -n "Creating dialout group... "
                                        if ! sudo groupadd ${NEW_GROUP} 2>/dev/null; then
                                                echo "FAILED"
                                                EXISTS=0
                                        else
                                                echo "DONE"
                                                EXISTS=1
                                        fi
                                        break;;
                                [Nn] )
                                        EXISTS=0
                                        break;;
                                * )
                                        echo "Please answer y or n."
                                        break;;
                        esac
                done
        fi
        return ${EXISTS}
}

# ----------------------------------------------------------------------------
# function that checks if user is member of group (NEW_GROUP)
# ----------------------------------------------------------------------------

function in_group ()
{
        IN_GROUP=0
        # get groups the current user belongs to
        USR_GROUPS="`groups ${USERNAME}`"
        # check ${NEW_GROUP} is one of them
        for USR_GROUP in ${USR_GROUPS}; do
                if [ ${USR_GROUP} == ${NEW_GROUP} ]; then
                        IN_GROUP=1;
                fi
        done
        return ${IN_GROUP}
}

# ----------------------------------------------------------------------------
# function that adds user in group (NEW_GROUP)
# ----------------------------------------------------------------------------

function add_user_in_group ()
{
        echo "Your account in not a member of ${NEW_GROUP} group.";
        while true; do
                echo "Would you like to be added in this group now? [Y/n]"
                read -s -n 1 USR_RESPONSE
                case ${USR_RESPONSE} in
                        [Yy]|"" )
                                echo -n "Adding user in ${NEW_GROUP}... "
                                if ! sudo usermod ${USERNAME} -a -G ${NEW_GROUP}; then
                                        echo "FAILED"
                                else
                                        echo "DONE"
                                fi
                                break;;
                        [Nn] )
                                break;;
                        * )
                                echo "Please answer y or n."
                                break;;
                esac
        done
}

# ****************************************************************************
# MAIN SCRIPT BODY
# ****************************************************************************

# ----------------------------------------------------------------------------
# setup working environment
# ----------------------------------------------------------------------------

PATH=/usr/bin:/bin
# new files will be readable by everyone, but only writable by the owner
umask 022
# installation directories
USERNAME=$(whoami)
ICONLOCATION="/usr/share/icons/hicolor/256x256/apps/"
MENULOCATION="/usr/share/applications/"
LINKLOCATION="/usr/bin/"
EXECLOCATION="/var/opt/dudegui/"
# working directories
RND=$(($RANDOM  + 128))
DIR=${0%`basename $0`}
WORKDIR="${DIR}/dudetmp${RND}"
BINZIP="binary.zip"
# number of lines in this script file (plus 1)
SCRIPT_LINES=X
# run /bin/sum on your binary and put the two values here
SUM1=X
SUM2=X

# ----------------------------------------------------------------------------
# what to do in case of abnormal/unexpected termination
# ----------------------------------------------------------------------------

trap 'rm -fr ${WORKDIR}; exit 1' HUP INT QUIT TERM

# ----------------------------------------------------------------------------
# prepare archive working space and archive
# ----------------------------------------------------------------------------

echo -e -n "\nUnpacking \"bin\" file in temporary directory... ";
# create working directory
mkdir -p ${WORKDIR}
# unzip archive
tail -n +${SCRIPT_LINES} "$0" > ${WORKDIR}/${BINZIP}
echo "DONE"

# ----------------------------------------------------------------------------
# validate zip file with specified checksums
# ----------------------------------------------------------------------------

echo -e -n "\nChecking archive integrity with checksums... ";
SUM=`sum ${WORKDIR}/${BINZIP}`
ASUM1=`echo "${SUM}" | awk '{print $1}'`
ASUM2=`echo "${SUM}" | awk '{print $2}'`
if [ ${ASUM1} -ne ${SUM1} ] || [ ${ASUM2} -ne ${SUM2} ]; then
        echo "FAILED"
        echo "The binary archive appears to be corrupted. Please, download"
        echo -e "the file again and re-try the installation...\n"
        rm -fr ${WORKDIR}
        exit 1
fi
echo "DONE (archive untainted!)"

# ----------------------------------------------------------------------------
# uncompress zip file
# ----------------------------------------------------------------------------

tar -xjf ${WORKDIR}/${BINZIP} -C ${WORKDIR}

# ----------------------------------------------------------------------------
# place files in proper locations
# ----------------------------------------------------------------------------

# -------- install "desktop" file --------

# exit if directory not present
if [ ! -d "${MENULOCATION}" ]; then
        echo -e "\nThere is something wrong with your DE. Installation will abort!"
        rm -fr ${WORKDIR}
        exit 1;
fi

# install file
echo -e -n "\Installing application \"desktop\" file... ";
sudo xdg-desktop-menu install ${WORKDIR}/dudegui.desktop --novendor --mode system
if [ $? == 0 ]; then
        echo "DONE"
else
        echo "FAILED"
        echo "The application will not be accessible through the DE's main menu!"
fi

# -------- install icon --------

# create directory if not present
if [ ! -d "${ICONLOCATION}" ]; then
        sudo mkdir -p ${ICONLOCATION}
fi

# copy file
echo -e -n "\nCopying application icon... ";
sudo cp ${WORKDIR}/dudegui.png ${ICONLOCATION}
echo "DONE"

# -------- install aplication --------

 # create directory
if [ ! -d "${EXECLOCATION}" ]; then
        sudo mkdir -p ${EXECLOCATION}
fi

# copy all necessary files
echo -e -n "\nCopying application binary and data... ";
sudo cp ${WORKDIR}/dudegui ${EXECLOCATION}
sudo cp ${WORKDIR}/dudegui.ui ${EXECLOCATION}
sudo cp ${WORKDIR}/dev2xml.lst ${EXECLOCATION}
sudo cp ${WORKDIR}/xmlfiles ${EXECLOCATION} -r
echo "DONE"

# loosen the permissions on certain files
#sudo chgrp -f users ${EXECLOCATION}/dev2xml.lst 2>/dev/null
#sudo chgrp -f users ${EXECLOCATION}/xmlfiles -R 2>/dev/null
sudo chmod -f 666 ${EXECLOCATION}/dev2xml.lst
sudo chmod -f 775 ${EXECLOCATION}/xmlfiles
sudo chmod -f 666 ${EXECLOCATION}/xmlfiles/*

# create symbolic link to executable
sudo ln -s ${EXECLOCATION}/dudegui /usr/bin/dudegui

# ----------------------------------------------------------------------------
# make serial programmers accessible (add user in necessary groups)
# ----------------------------------------------------------------------------

echo -e "\nSome hardware programmers communicate through serial port."
echo "Permissions on these devices are managed with the use of distribution"
echo "specific user groups. The script will do it's best to guess what"
echo -e "those groups are and to make sure that your account is properly configured...\n"

# get necessary groups (names will be stored in NEW_GROUPS)
which_groups

for NEW_GROUP in ${NEW_GROUPS}; do
        #echo ${NEW_GROUP}
        # check if groups exists
        is_group
        # get return value
        RET_VAL=$?
        # proceed to next loop iteration, if group not present
        if [ RET_VAL != 1 ]; then
                continue
        fi
        # check if user is a member
        in_group
        # get return value
        RET_VAL=$?
        # proceed to next loop iteration, if already member of group
        if [ RET_VAL == 1 ]; then
                continue
        fi
        # add user in group
        add_user_in_group
done

echo -e "If you cannot access a programmer that is connected through a serial port,"
echo -e "check the permissions of the relative device file (/dev/ttySx)."

# ----------------------------------------------------------------------------
# make USB programmers accessible (add udev rules)
# ----------------------------------------------------------------------------

echo -e "\nSome hardware programmers communicate through USB."
echo "Access on such devices can be granted automatically"
echo "with the help of the appropriate udev rules. The script"
echo "will add a set of rules, that matches all the well known"
echo -e "hardware programmers...\n"

udev_rules > ${WORKDIR}/rules.txt
sudo cp ${WORKDIR}/rules.txt "/etc/udev/rules.d/99-dudegui.rules"

# ----------------------------------------------------------------------------
# clean-up and exit
# ----------------------------------------------------------------------------

rm -fr ${WORKDIR}
exit 0
