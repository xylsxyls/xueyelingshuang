#--------------------------------------------------------------------
SHELL_FOLDER=$(cd $(dirname $0); pwd)
xueyelingshuang=$SHELL_FOLDER/../..

#该写法支持目录下含有子目录和空格
srcpath="$xueyelingshuang/tools/opencv320/include/"
destpath="$xueyelingshuang/include/"
if [ ! -d "$destpath" ]
then
    mkdir -p "$destpath"
fi
for file in $(ls "$srcpath")
do
    cp -rf "$srcpath$file" "$destpath"
done

if [[ $1 == '32' ]] && [[ $2 == 'debug' ]]
then
    cp -rf "$xueyelingshuang/tools/opencv320/staticlib/x86/debug/"*.lib "$xueyelingshuang/lib/"
    cp -rf "$xueyelingshuang/tools/opencv320/staticlib/x86/"*.lib "$xueyelingshuang/lib/"
    cp -rf "$xueyelingshuang/tools/opencv320/opencv_ffmpeg320.dll" "$xueyelingshuang/lib/"
elif [[ $1 == '32' ]] && [[ $2 == 'release' ]]
then
    cp -rf "$xueyelingshuang/tools/opencv320/staticlib/x86/release/"*.lib "$xueyelingshuang/lib/"
    cp -rf "$xueyelingshuang/tools/opencv320/staticlib/x86/"*.lib "$xueyelingshuang/lib/"
    cp -rf "$xueyelingshuang/tools/opencv320/opencv_ffmpeg320.dll" "$xueyelingshuang/lib/"
elif [[ $1 == '64' ]] && [[ $2 == 'debug' ]]
then
    cp -rf "$xueyelingshuang/tools/opencv320/staticlib/x64/debug/"*.lib "$xueyelingshuang/lib/"
    cp -rf "$xueyelingshuang/tools/opencv320/staticlib/x64/"*.lib "$xueyelingshuang/lib/"
    cp -rf "$xueyelingshuang/tools/opencv320/opencv_ffmpeg320_64.dll" "$xueyelingshuang/lib/"
elif [[ $1 == '64' ]] && [[ $2 == 'release' ]]
then
    cp -rf "$xueyelingshuang/tools/opencv320/staticlib/x64/release/"*.lib "$xueyelingshuang/lib/"
    cp -rf "$xueyelingshuang/tools/opencv320/staticlib/x64/"*.lib "$xueyelingshuang/lib/"
    cp -rf "$xueyelingshuang/tools/opencv320/opencv_ffmpeg320_64.dll" "$xueyelingshuang/lib/"
fi