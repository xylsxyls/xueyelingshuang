
:<<!EOF!
#--------------------------------------------------------------------
DllRelyTest_bit=$1
DllRelyTest_dlllib=$2
DllRelyTest_debugRelease=$3
DllRelyTest_allSame=$4
"$CLOUD_REBUILD" DllRelyTest $DllRelyTest_bit $DllRelyTest_dlllib $DllRelyTest_debugRelease $DllRelyTest_allSame
!EOF!

#--------------------------------------------------------------------
QssString_bit=$1
QssString_dlllib=$2
QssString_debugRelease=$3
QssString_allSame=$4
"$CLOUD_REBUILD" QssString $QssString_bit $QssString_dlllib $QssString_debugRelease $QssString_allSame

#--------------------------------------------------------------------
CSystem_bit=$1
CSystem_dlllib=$2
CSystem_debugRelease=$3
CSystem_allSame=$4
"$CLOUD_REBUILD" CSystem $CSystem_bit $CSystem_dlllib $CSystem_debugRelease $CSystem_allSame

#--------------------------------------------------------------------
EO_XmlSax2Parser_bit=$1
EO_XmlSax2Parser_dlllib=$2
EO_XmlSax2Parser_debugRelease=$3
EO_XmlSax2Parser_allSame=$4
"$CLOUD_REBUILD" EO_XmlSax2Parser $EO_XmlSax2Parser_bit $EO_XmlSax2Parser_dlllib $EO_XmlSax2Parser_debugRelease $EO_XmlSax2Parser_allSame