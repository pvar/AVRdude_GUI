#!/bin/bash

SCRIPT="script.txt"
ARCHIVE="dudegui.zip"
INSTALLER="dudegui.bin"
WORKDIR=${0%`basename $0`}

# create archive with application files
echo "Creating archive with application files..."
cd ${WORKDIR}
tar -cjf ${ARCHIVE} dudegui dudegui.ui dudegui.png dudegui.desktop dev2xml.lst xmlfiles
cd - 1>/dev/null

# get value for SCRIPT_LINES
LINES=$(cat ${WORKDIR}/install.sh | wc -l)

# get values for SUM1 and SUM2
SUM=`sum ${WORKDIR}/${ARCHIVE}`
SUM1=`echo "${SUM}" | awk '{print $1}'`
SUM2=`echo "${SUM}" | awk '{print $2}'`

# edit install.sh
echo "Modifying installation script..."
sed -e s/"SCRIPT_LINES=X"/"SCRIPT_LINES=$((LINES+1))"/ -e s/"SUM1=X"/"SUM1=${SUM1}"/ -e s/"SUM2=X"/"SUM2=${SUM2}"/ <${WORKDIR}/install.sh > ${WORKDIR}/${SCRIPT}

# compound installer script and application archive
echo "Creating standalone installatikon file..."
cat ${WORKDIR}/${SCRIPT} ${WORKDIR}/${ARCHIVE} > ${WORKDIR}/${INSTALLER}
chmod +x ${WORKDIR}/${INSTALLER}

# clean-up
rm ${WORKDIR}/${SCRIPT} ${WORKDIR}/${ARCHIVE}
echo "all done!"
