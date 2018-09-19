#!/bin/bash

FILENAME=ChatScript-7.6.zip
TMPFOLDER=INSTALL
BASEFOLDER=`pwd`

# CREATES TMP DIR
echo "Creting tempfolder $TMPFOLDER"
mkdir -p $TMPFOLDER
cd $TMPFOLDER

echo -n "Getting ChatScript..."
wget -O $FILENAME http://downloads.sourceforge.net/project/chatscript/ChatScript-7.6.zip >wget.log 2>&1
cd $BASEFOLDER
unzip $TMPFOLDER/$FILENAME
chmod +x BINARIES/LinuxChatScript64 BINARIES/MacChatScript
echo " done!"

echo  -n "Cleaning up..."
rm -rf $TMPFOLDER
echo " done!"
