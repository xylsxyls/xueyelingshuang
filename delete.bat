attrib -a -h -s -r "%~dp0*.sdf" /s
del "%~dp0*.sdf" /s
attrib -a -h -s -r "%~dp0*.suo" /s
del "%~dp0*.suo" /s
attrib -a -h -s -r "%~dp0*.user" /s
del "%~dp0*.user" /s
attrib -a -h -s -r "%~dp0*.aps" /s
del "%~dp0*.aps" /s
attrib -a -h -s -r "%~dp0*.opensdf" /s
del "%~dp0*.opensdf" /s

rmdir /q /s "%~dp0src\temp\"

::-------------------ɾ��MFC��ʱ�ļ�--------------------------
rmdir /q /s "%~dp0src\MessageTest\ipch\"
rmdir /q /s "%~dp0src\MessageTest\MessageTest\Debug\"
rmdir /q /s "%~dp0src\MessageTest\MessageTest\Release\"
::------------------------------------------------------------
rmdir /q /s "%~dp0src\MFCQueryIntroduce\ipch\"
rmdir /q /s "%~dp0src\MFCQueryIntroduce\MFCQueryIntroduce\Debug\"
rmdir /q /s "%~dp0src\MFCQueryIntroduce\MFCQueryIntroduce\Release\"
::------------------------------------------------------------
rmdir /q /s "%~dp0src\MFCSaveIntroduce\ipch\"
rmdir /q /s "%~dp0src\MFCSaveIntroduce\MFCSaveIntroduce\Debug\"
rmdir /q /s "%~dp0src\MFCSaveIntroduce\MFCSaveIntroduce\Release\"
::------------------------------------------------------------
rmdir /q /s "%~dp0src\CalcTest\ipch\"
rmdir /q /s "%~dp0src\CalcTest\CalcTest\Debug\"
rmdir /q /s "%~dp0src\CalcTest\CalcTest\Release\"
::------------------------------------------------------------
rmdir /q /s "%~dp0src\MFCTest\ipch\"
rmdir /q /s "%~dp0src\MFCTest\MFCTest\Debug\"
rmdir /q /s "%~dp0src\MFCTest\MFCTest\Release\"
::------------------------------------------------------------
rmdir /q /s "%~dp0src\MFCTest1\ipch\"
rmdir /q /s "%~dp0src\MFCTest1\MFCTest\Debug\"
rmdir /q /s "%~dp0src\MFCTest1\MFCTest\Release\"
::------------------------------------------------------------
rmdir /q /s "%~dp0src\FindText\ipch\"
rmdir /q /s "%~dp0src\FindText\FindText\Debug\"
rmdir /q /s "%~dp0src\FindText\FindText\Release\"
::------------------------------------------------------------
rmdir /q /s "%~dp0src\FundInvest\ipch\"
rmdir /q /s "%~dp0src\FundInvest\FundInvest\Debug\"
rmdir /q /s "%~dp0src\FundInvest\FundInvest\Release\"
::------------------------------------------------------------
rmdir /q /s "%~dp0src\OneKey\ipch\"
rmdir /q /s "%~dp0src\OneKey\OneKey\Debug\"
rmdir /q /s "%~dp0src\OneKey\OneKey\Release\"
::------------------------------------------------------------
rmdir /q /s "%~dp0src\11Controls\11Controls\GeneratedFiles\"
rmdir /q /s "%~dp0src\11Controls\11Controls\include\"
::------------------------------------------------------------
rmdir /q /s "%~dp0src\QtTest\QtTest\GeneratedFiles\"
rmdir /q /s "%~dp0src\QtTest1\QtTest1\GeneratedFiles\"
::------------------------------------------------------------

::-------------------ɾ������̨��ʱ�ļ�-----------------------
rmdir /q /s "%~dp0src\ConsoleTest\ConsoleTest\Debug\"
rmdir /q /s "%~dp0src\ConsoleTest\ConsoleTest\Release\"
::------------------------------------------------------------
rmdir /q /s "%~dp0src\ConsoleTest1\ConsoleTest\Debug\"
rmdir /q /s "%~dp0src\ConsoleTest1\ConsoleTest\Release\"
::------------------------------------------------------------
rmdir /q /s "%~dp0src\CloudRebuild\CloudRebuild\Debug\"
rmdir /q /s "%~dp0src\CloudRebuild\CloudRebuild\Release\"
::------------------------------------------------------------

::-------------------ɾ����ʱͼƬ-----------------------------
rmdir /q /s "%~dp0common\AutoPlayGame\ScreenScriptTemp\"

::-------------------ɾ����ʱ����-----------------------------
del "%~dp0src\ProtoMessage\ProtoMessage\src\ProtoMessage.pb.h"
del "%~dp0src\ProtoMessage\ProtoMessage\src\ProtoMessage.pb.cc"