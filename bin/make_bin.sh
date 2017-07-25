#!/bin/bash

SCRIPT1=script.tmp
SCRIPT2=script.txt
ARCHIVE=dudegui.zip
INSTALLER=dudegui.bin

# create archive with application files
tar -cjf ${ARCHIVE} dudegui dudegui.ui dudegui.png dudegui.desktop dev2xml.lst xmlfiles

# use sed to properly set SCRIPT_LINES in install.sh
cp install.sh ${SCRIPT1}
LINES=$(cat ${SCRIPT1} | wc -l)
sed s/"SCRIPT_LINES=X"/"SCRIPT_LINES=$((LINES+1))"/ <${SCRIPT1} > ${SCRIPT2}

# use sed to properly set SUM values in install.sh
SUM=`sum ${ARCHIVE}`
SUM1=`echo "${SUM}" | awk '{print $1}'`
SUM2=`echo "${SUM}" | awk '{print $2}'`
cp ${SCRIPT2} ${SCRIPT1}
sed s/"SUM1=X"/"SUM1=${SUM1}"/ <${SCRIPT1} > ${SCRIPT2}
cp ${SCRIPT2} ${SCRIPT1}
sed s/"SUM2=X"/"SUM2=${SUM2}"/ <${SCRIPT1} > ${SCRIPT2}

# compound installer script and application archive
cat ${SCRIPT2} ${ARCHIVE} > ${INSTALLER}
chmod +x ${INSTALLER}

# clean-up
rm ${SCRIPT1} ${SCRIPT2} ${ARCHIVE}
