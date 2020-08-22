SHELL_FOLDER=$(cd $(dirname $0); pwd)
xueyelingshuang=$SHELL_FOLDER/../../..

rm -rf "$xueyelingshuang/include/BigNumber/"
mv "$xueyelingshuang/include/FastNumber/" "$xueyelingshuang/include/BigNumber/"