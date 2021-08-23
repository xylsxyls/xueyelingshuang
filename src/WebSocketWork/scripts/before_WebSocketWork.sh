
:<<!EOF!
#--------------------------------------------------------------------
DllRelyTest_bit=$1
DllRelyTest_dlllib=$2
DllRelyTest_debugRelease=$3
DllRelyTest_allSame=$4
"$CLOUD_REBUILD" DllRelyTest $DllRelyTest_bit $DllRelyTest_dlllib $DllRelyTest_debugRelease $DllRelyTest_allSame
!EOF!

#--------------------------------------------------------------------
CTaskThreadManager_bit=$1
CTaskThreadManager_dlllib=$2
CTaskThreadManager_debugRelease=$3
CTaskThreadManager_allSame=$4
"$CLOUD_REBUILD" CTaskThreadManager $CTaskThreadManager_bit $CTaskThreadManager_dlllib $CTaskThreadManager_debugRelease $CTaskThreadManager_allSame

#--------------------------------------------------------------------
CorrespondParam_bit=$1
CorrespondParam_dlllib=$2
CorrespondParam_debugRelease=$3
CorrespondParam_allSame=$4
"$CLOUD_REBUILD" CorrespondParam $CorrespondParam_bit $CorrespondParam_dlllib $CorrespondParam_debugRelease $CorrespondParam_allSame

#--------------------------------------------------------------------
SHELL_FOLDER=$(cd $(dirname $0); pwd)
xueyelingshuang=$SHELL_FOLDER/../../..

libname="websocketpp"
includecreatedir=1

#该写法支持目录下含有子目录和空格
includepath=$xueyelingshuang"/tools/"$libname"/include/"
copyincludepath=$xueyelingshuang"/include/"
if [[ $includecreatedir == 1 ]]; then
    copyincludepath=$xueyelingshuang"/include/"$libname"/"
fi
if [ ! -d $copyincludepath ]
then
    mkdir -p $copyincludepath
fi
for file in $(ls $includepath)
do
    cp -rf $includepath$file $copyincludepath
done