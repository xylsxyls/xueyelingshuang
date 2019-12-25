attrib -a -h -s -r "%~dp0*.sdf" /s
del "%~dp0*.sdf" /s
attrib -a -h -s -r "%~dp0*.suo" /s
del "%~dp0*.suo" /s
attrib -a -h -s -r "%~dp0*.aps" /s
del "%~dp0*.aps" /s
attrib -a -h -s -r "%~dp0*.opensdf" /s
del "%~dp0*.opensdf" /s

::copy "%~dp0src\QtCore\QtCore\vcxproj\QtCore.vcxproj.user" "%~dp0src\QtCore\QtCore\vcxproj\QtCore.vcxproj.1user"

::copy "%~dp0src\QtControls\QtControls\vcxproj\QtControls.vcxproj.user" "%~dp0src\QtControls\QtControls\vcxproj\QtControls.vcxproj.1user"

::copy "%~dp0src\DialogManager\DialogManager\vcxproj\DialogManager.vcxproj.user" "%~dp0src\DialogManager\DialogManager\vcxproj\DialogManager.vcxproj.1user"

::attrib -a -h -s -r "%~dp0*.user" /s
::del "%~dp0*.user" /s

::copy "%~dp0src\QtCore\QtCore\vcxproj\QtCore.vcxproj.1user" "%~dp0src\QtCore\QtCore\vcxproj\QtCore.vcxproj.user"
::del "%~dp0src\QtCore\QtCore\vcxproj\QtCore.vcxproj.1user"

::copy "%~dp0src\QtControls\QtControls\vcxproj\QtControls.vcxproj.1user" "%~dp0src\QtControls\QtControls\vcxproj\QtControls.vcxproj.user"
::del "%~dp0src\QtControls\QtControls\vcxproj\QtControls.vcxproj.1user"
 
::copy "%~dp0src\DialogManager\DialogManager\vcxproj\DialogManager.vcxproj.1user" "%~dp0src\DialogManager\DialogManager\vcxproj\DialogManager.vcxproj.user"
::del "%~dp0src\DialogManager\DialogManager\vcxproj\DialogManager.vcxproj.1user"

rmdir /q /s "%~dp0src\WidgetFactory\WidgetFactory\vcxproj\GeneratedFiles\"
rmdir /q /s "%~dp0src\QssString\QssString\vcxproj\GeneratedFiles\"
rmdir /q /s "%~dp0src\HttpRequest\HttpRequest\vcxproj\GeneratedFiles\"
rmdir /q /s "%~dp0src\EO_XmlSax2Parser\EO_XmlSax2Parser\vcxproj\GeneratedFiles\"
rmdir /q /s "%~dp0src\QtControls\QtControls\vcxproj\GeneratedFiles\"
rmdir /q /s "%~dp0src\DialogManager\DialogManager\vcxproj\GeneratedFiles\"
rmdir /q /s "%~dp0src\StockDraw\StockDraw\vcxproj\GeneratedFiles\"

rmdir /q /s "%~dp0src\QtClient\QtClient\vcxproj\GeneratedFiles\" 

rmdir /q /s "%~dp0src\temp\"

::-------------------删除MFC临时文件--------------------------
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
del "%~dp0src\QtTest\QtTest\QtTest.vcxproj.user"
rmdir /q /s "%~dp0src\QtTest1\QtTest1\GeneratedFiles\"
del "%~dp0src\QtTest1\QtTest1\QtTest1.vcxproj.user"
::------------------------------------------------------------
rmdir /q /s "%~dp0src\QtClient\.vs\"
::------------------------------------------------------------
rmdir /q /s "%~dp0src\StockClient\StockClient\GeneratedFiles\"
del "%~dp0src\StockClient\StockClient\StockClient.vcxproj.user"
::------------------------------------------------------------
rmdir /q /s "%~dp0src\dtws\dtws\GeneratedFiles\"
del "%~dp0src\dtws\dtws\dtws.vcxproj.user"
::------------------------------------------------------------


::-------------------删除控制台临时文件-----------------------
rmdir /q /s "%~dp0src\ConsoleTest\ConsoleTest\Debug\"
rmdir /q /s "%~dp0src\ConsoleTest\ConsoleTest\Release\"
::------------------------------------------------------------
rmdir /q /s "%~dp0src\ConsoleTest1\ConsoleTest\Debug\"
rmdir /q /s "%~dp0src\ConsoleTest1\ConsoleTest\Release\"
::------------------------------------------------------------
rmdir /q /s "%~dp0src\CloudRebuild\CloudRebuild\Debug\"
rmdir /q /s "%~dp0src\CloudRebuild\CloudRebuild\Release\"
::------------------------------------------------------------

::-------------------删除临时图片-----------------------------
rmdir /q /s "%~dp0common\AutoPlayGame\ScreenScriptTemp\"

::-------------------删除临时代码-----------------------------
del "%~dp0src\ProtoMessage\ProtoMessage\src\ProtoMessage.pb.h"
del "%~dp0src\ProtoMessage\ProtoMessage\src\ProtoMessage.pb.cc"