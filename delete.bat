attrib -a -h -s -r "%~dp0*.sdf" /s
del "%~dp0*.sdf" /s
attrib -a -h -s -r "%~dp0*.suo" /s
del "%~dp0*.suo" /s
attrib -a -h -s -r "%~dp0*.user" /s
del "%~dp0*.user" /s
attrib -a -h -s -r "%~dp0*.aps" /s
del "%~dp0*.aps" /s

::-------------------ɾ��MFC��ʱ�ļ�--------------------------
rmdir /q /s "%~dp0src\MessageTest\ipch\"
rmdir /q /s "%~dp0src\MessageTest\MessageTest\Debug\"
rmdir /q /s "%~dp0src\MessageTest\MessageTest\Release\"
::------------------------------------------------------------
rmdir /q /s "%~dp0src\MFCQueryIntroduce\ipch\"
rmdir /q /s "%~dp0src\MFCQueryIntroduce\MFCTest\Debug\"
rmdir /q /s "%~dp0src\MFCQueryIntroduce\MFCTest\Release\"
::------------------------------------------------------------
rmdir /q /s "%~dp0src\MFCSaveIntroduce\ipch\"
rmdir /q /s "%~dp0src\MFCSaveIntroduce\MFCTest\Debug\"
rmdir /q /s "%~dp0src\MFCSaveIntroduce\MFCTest\Release\"
::------------------------------------------------------------
rmdir /q /s "%~dp0src\MFCTest\ipch\"
rmdir /q /s "%~dp0src\MFCTest\MFCTest\Debug\"
rmdir /q /s "%~dp0src\MFCTest\MFCTest\Release\"
::------------------------------------------------------------
rmdir /q /s "%~dp0src\MFCTest1\ipch\"
rmdir /q /s "%~dp0src\MFCTest1\MFCTest\Debug\"
rmdir /q /s "%~dp0src\MFCTest1\MFCTest\Release\"
::------------------------------------------------------------

::-------------------ɾ������̨��ʱ�ļ�-----------------------
rmdir /q /s "%~dp0src\ConsoleTest\ConsoleTest\Debug\"
rmdir /q /s "%~dp0src\ConsoleTest\ConsoleTest\Release\"
::------------------------------------------------------------
rmdir /q /s "%~dp0src\ConsoleTest1\ConsoleTest\Debug\"
rmdir /q /s "%~dp0src\ConsoleTest1\ConsoleTest\Release\"
::------------------------------------------------------------