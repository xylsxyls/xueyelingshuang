#--------------------------------------------------------------------
SHELL_FOLDER=$(cd $(dirname $0); pwd)
xueyelingshuang=$SHELL_FOLDER/../../..

#��д��֧��Ŀ¼�º�����Ŀ¼�Ϳո�
srcpath="$xueyelingshuang/src/DialogManager/DialogManager/src/res/"
destpath="$xueyelingshuang/lib/res/"
if [ ! -d "$destpath" ]
then
    mkdir -p "$destpath"
fi
for file in $(ls "$srcpath")
do
    cp -rf "$srcpath$file" "$destpath"
done