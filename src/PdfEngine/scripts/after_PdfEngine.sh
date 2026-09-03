#!/bin/sh
SHELL_FOLDER=$(cd "$(dirname "$0")"; pwd)
rootpath=$SHELL_FOLDER/../../..

mkdir -p $rootpath/include/PdfEngine
cp -f $SHELL_FOLDER/../PdfEngine/src/PdfEngineAPI.h $rootpath/include/PdfEngine/PdfEngineAPI.h
cp -f $SHELL_FOLDER/../PdfEngine/src/PdfEngine.h $rootpath/include/PdfEngine/PdfEngine.h
cp -f $SHELL_FOLDER/../PdfEngine/src/PdfDocument.h $rootpath/include/PdfEngine/PdfDocument.h
cp -f $SHELL_FOLDER/../PdfEngine/src/PdfEngineBitmap.h $rootpath/include/PdfEngine/PdfEngineBitmap.h
cp -f $SHELL_FOLDER/../PdfEngine/src/PdfEnginePageRef.h $rootpath/include/PdfEngine/PdfEnginePageRef.h
cp -f $SHELL_FOLDER/../PdfEngine/src/PdfEnginePageSize.h $rootpath/include/PdfEngine/PdfEnginePageSize.h