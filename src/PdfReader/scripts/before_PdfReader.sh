#!/bin/sh

:<<!EOF!
#--------------------------------------------------------------------
DllRelyTest_bit=$1
DllRelyTest_dlllib=$2
DllRelyTest_debugRelease=$3
DllRelyTest_allSame=$4
"$CLOUD_REBUILD" DllRelyTest $DllRelyTest_bit $DllRelyTest_dlllib $DllRelyTest_debugRelease $DllRelyTest_allSame
!EOF!

#--------------------------------------------------------------------
CDump_bit=$1
CDump_dlllib=$2
CDump_debugRelease=$3
CDump_allSame=$4
"$CLOUD_REBUILD" CDump $CDump_bit $CDump_dlllib $CDump_debugRelease $CDump_allSame

#--------------------------------------------------------------------
LogManager_bit=$1
LogManager_dlllib=$2
LogManager_debugRelease=$3
LogManager_allSame=$4
"$CLOUD_REBUILD" LogManager $LogManager_bit $LogManager_dlllib $LogManager_debugRelease $LogManager_allSame

#--------------------------------------------------------------------
PdfEngine_bit=$1
PdfEngine_dlllib=$2
PdfEngine_debugRelease=$3
PdfEngine_allSame=$4
"$CLOUD_REBUILD" PdfEngine $PdfEngine_bit $PdfEngine_dlllib $PdfEngine_debugRelease $PdfEngine_allSame
