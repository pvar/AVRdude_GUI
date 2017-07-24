#!/bin/bash

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
# check dependencies
# ----------------------------------------------------------------------------

# check if libgtkmm 3.0 exists...
#sudo ldconfig -p | grep libgtkmm-3.0 | wc -l
# check if libxml 2.6  exists...
#sudo ldconfig -p | grep libxml++-2.6 | wc -l

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
# make serial programmers accessible (add user in group dialout)
# ----------------------------------------------------------------------------

# ----------------------------------------------------------------------------
# make USB programmers accessible (add udev rules)
# ----------------------------------------------------------------------------


# ----------------------------------------------------------------------------
# clean-up and exit
# ----------------------------------------------------------------------------

rm -fr ${WORKDIR}
exit 0
