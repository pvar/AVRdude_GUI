#!/bin/bash

# ****************************************************************************
# FUNCTION DEFINITIONS
# ****************************************************************************

# ----------------------------------------------------------------------------
# function to get necessary user groups, for accessing serial devices
# ----------------------------------------------------------------------------

function which_groups ()
{
        [[ -r  /etc/os-release ]] && . /etc/os-release
        case "$ID" in
                debian|ubuntu)
                        NEW_GROUPS="dialout tty"
                        ;;
                opensuse)
                        NEW_GROUPS="dialout uucp lock"
                        ;;
                archlinux)
                        NEW_GROUPS="uucp lock"
                        ;;
                *)
                        # will have to guess necessary groups..."
                        TTYS_GROUP=$([[ -e /dev/ttyS0 ]] && ls -lh /dev/ttyS0 | awk '{print $4}')
                        case "${TTYS_GROUP}" in
                                uucp)
                                        # suppose it's an ARCH-based distribution
                                        NEW_GROUPS="uucp lock"
                                        ;;
                                dialout)
                                        # suppose it's a DEBIAN-based distribution
                                        NEW_GROUPS="dialout tty"
                                        ;;
                                *)
                                        # don't have a clue... :-)
                                        NEW_GROUPS=${TTYS_GROUP}
                                        ;;
                        exit 1
                        ;;
        esac
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
ICONLOCATION=~/.local/share/icons/hicolor/256x256/apps/
LINKLOCATION=~/.local/share/applications/
EXECLOCATION=~/bin/dudegui/
# working directories
RND=$(($RANDOM  + 128))
DIR=${0%`basename $0`}
WORKDIR=${DIR}/dudetmp${RND}
BINZIP=binary.zip
# number of lines in this script file (plus 1)
SCRIPT_LINES=129
# run /bin/sum on your binary and put the two values here
SUM1=09722
SUM2=8525

# ----------------------------------------------------------------------------
# what to do in case of abnormal/unexpected termination
# ----------------------------------------------------------------------------

trap 'rm -fr ${WORKDIR}; exit 1' HUP INT QUIT TERM

# ----------------------------------------------------------------------------
# prepare archive working space and archive
# ----------------------------------------------------------------------------

echo -n "Unpacking \"bin\" file in temporary directory... ";
# create working directory
mkdir -p ${WORKDIR}
# unzip archive
tail -n +${SCRIPT_LINES} "$0" > ${WORKDIR}/${BINZIP}
echo "DONE"

# ----------------------------------------------------------------------------
# validate zip file with specified checksums
# ----------------------------------------------------------------------------

echo -n "Checking archive integrity with checksums... ";
SUM=`sum ${WORKDIR}/${BINZIP}`
ASUM1=`echo "${SUM}" | awk '{print $1}'`
ASUM2=`echo "${SUM}" | awk '{print $2}'`
if [ ${ASUM1} -ne ${SUM1} ] || [ ${ASUM2} -ne ${SUM2} ]; then
        echo "FAILED"
        echo "The binary archive appears to be corrupted. Please, download"
        echo "the file again and re-try the installation..."
        rm -fr ${WORKDIR}
        exit 1
fi
echo "DONE (archive untainted!)"

# ----------------------------------------------------------------------------
# uncompress zip file
# ----------------------------------------------------------------------------

unzip -qq ${WORKDIR}/${BINZIP} -d ${WORKDIR}

# ----------------------------------------------------------------------------
# place files in proper locations
# ----------------------------------------------------------------------------

# create icon directory if not present
if [ ! -d "${ICONLOCATION}" ]; then
        mkdir -p ${ICONLOCATION}
fi

# copy icon file
echo -n "Copying application icon... ";
cp ${WORKDIR}/dudegui.png ${ICONLOCATION}
echo "DONE"

# create application-link directory if not present
if [ ! -d "${LINKLOCATION}" ]; then
        mkdir -p ${LINKLOCATION}
fi

# copy "desktop" file
echo -n "Copying application \"desktop\" file... ";
cp ${WORKDIR}/dudegui.desktop ${LINKLOCATION}
echo "DONE"

# create folder in ~/bin if not present
if [ ! -d "${EXECLOCATION}" ]; then
        mkdir -p ${EXECLOCATION}
else
        rm -fr ${EXECLOCATION}
        mkdir -p ${EXECLOCATION}
fi

# copy binary and other necessary files
echo -n "Copying application binary and data... ";
cp ${WORKDIR}/dudegui ${EXECLOCATION}
cp ${WORKDIR}/dudegui.ui ${EXECLOCATION}
cp ${WORKDIR}/dev2xml.lst ${EXECLOCATION}
cp ${WORKDIR}/xmlfiles ${EXECLOCATION} -r
echo "DONE"

# ----------------------------------------------------------------------------
# make serial programmers accessible (add user in necessary groups)
# ----------------------------------------------------------------------------

echo "\nSome hardware programmers communicate through serial port."
echo "In order to be able use such a programmer, your account must"
echo "be a member of the appropriate user groups. The script will"
echo "make sure that your system is properly configured...\n"

# check if group dialout exists
DIALOUT=1
if [ `/usr/bin/grep -c "dialout" /etc/group` == "0"  ]; then
        echo "This is weird! It seems that your system lacks dialout group.";
        while true; do
                echo "Would you like this group to be created now? [Y/n]"
                read -s -n 1 USR_RESPONSE
                case ${USR_RESPONSE} in
                        [Yy]|"" )
                                echo -n "Creating dialout group... "
                                if ! sudo groupadd dialout 2>/dev/null; then
                                        echo "FAILED"
                                        DIALOUT=0
                                else
                                        echo "DONE"
                                        DIALOUT=1
                                fi
                                break;;
                        [Nn] )
                                DIALOUT=0
                                break;;
                        * )
                                echo "Please answer y or n."
                                break;;
                esac
        done
fi

# only proceed if dialout group exists or was created
if [ ${DIALOUT} == 1 ]; then
        IN_DIALOUT=0
        # get groups the current user belongs to
        USR_GROUPS="`groups $(whoami)`"
        # check dialout is one of them
        for USR_GROUP in ${USR_GROUPS}; do
                if [ ${USR_GROUP} == "dialout" ]; then
                        IN_DIALOUT=1;
                fi
        done
        # proceed if user is not a member of dialout
        if [ ${IN_DIALOUT} == 0 ]; then
                echo "It seems that your account in not a member of dialout group.";
                while true; do
                        echo "Would you like to be added in this group now? [Y/n]"
                        read -s -n 1 USR_RESPONSE
                        case ${USR_RESPONSE} in
                                [Yy]|"" )
                                        echo -n "Adding user in group dialout... "
                                        if ! sudo usermod $(whoami) -a -G dialout; then
                                                echo "FAILED"
                                                DIALOUT=false
                                        else
                                                echo "DONE"
                                                DIALOUT=true
                                        fi
                                        break;;
                                [Nn] )
                                        DIALOUT=false
                                        break;;
                                * )
                                        echo "Please answer y or n."
                                        break;;
                        esac
                done
        fi
fi

# ----------------------------------------------------------------------------
# make USB programmers accessible (add udev rules)
# ----------------------------------------------------------------------------



# ----------------------------------------------------------------------------
# clean-up and exit
# ----------------------------------------------------------------------------

rm -fr ${WORKDIR}
exit 0
