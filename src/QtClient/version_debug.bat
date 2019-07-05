call "%CLOUD_REBUILD%" DialogManager 0 dll debug same
call "%CLOUD_REBUILD%" HttpRequest 0 dll debug same
call "%CLOUD_REBUILD%" WidgetFactory 0 dll debug same
call "%CLOUD_REBUILD%" JsonCpp 0 dll debug same
call "%CLOUD_REBUILD%" Cini 0 dll debug same
call "%FILE_REPLACE%" -dir %~dp0..\..\include\ Macro.h -replace "#pragma comment(lib," "//#pragma comment(lib,"
call "%FILE_REPLACE%" -dir %~dp0..\..\include\ API.h -replace "#pragma comment(lib," "//#pragma comment(lib,"
call "%FILE_REPLACE%" -dir %~dp0..\..\include\ Macro.h -replace _declspec(dllimport) _declspec(dllexport)

"%DEVENV_EXE_2017%" /rebuild "Debug|X64" %~dp0QtClient.sln

call "%FILE_REPLACE%" -dir %~dp0..\..\include\ Macro.h -replace _declspec(dllexport) _declspec(dllimport)

call "%FILE_REPLACE%" -dir %~dp0..\..\include\ Macro.h -replace "//#pragma comment(lib,\"CStringManagerd.lib\")" "#pragma comment(lib,\"QtClientd.lib\")"
call "%FILE_REPLACE%" -dir %~dp0..\..\include\ API.h -replace "//#pragma comment(lib,\"CStringManagerd.lib\")" "#pragma comment(lib,\"QtClientd.lib\")"
call "%FILE_REPLACE%" -dir %~dp0..\..\include\ Macro.h -replace "//#pragma comment(lib,\"CStringManager.lib\")" "#pragma comment(lib,\"QtClient.lib\")"
call "%FILE_REPLACE%" -dir %~dp0..\..\include\ API.h -replace "//#pragma comment(lib,\"CStringManager.lib\")" "#pragma comment(lib,\"QtClient.lib\")"

call "%FILE_REPLACE%" -dir %~dp0..\..\include\ Macro.h -replace "//#pragma comment(lib,\"CSystemd.lib\")" "#pragma comment(lib,\"QtClientd.lib\")"
call "%FILE_REPLACE%" -dir %~dp0..\..\include\ API.h -replace "//#pragma comment(lib,\"CSystemd.lib\")" "#pragma comment(lib,\"QtClientd.lib\")"
call "%FILE_REPLACE%" -dir %~dp0..\..\include\ Macro.h -replace "//#pragma comment(lib,\"CSystem.lib\")" "#pragma comment(lib,\"QtClient.lib\")"
call "%FILE_REPLACE%" -dir %~dp0..\..\include\ API.h -replace "//#pragma comment(lib,\"CSystem.lib\")" "#pragma comment(lib,\"QtClient.lib\")"

call "%FILE_REPLACE%" -dir %~dp0..\..\include\ Macro.h -replace "//#pragma comment(lib,\"EO_XmlSax2Parserd.lib\")" "#pragma comment(lib,\"QtClientd.lib\")"
call "%FILE_REPLACE%" -dir %~dp0..\..\include\ API.h -replace "//#pragma comment(lib,\"EO_XmlSax2Parserd.lib\")" "#pragma comment(lib,\"QtClientd.lib\")"
call "%FILE_REPLACE%" -dir %~dp0..\..\include\ Macro.h -replace "//#pragma comment(lib,\"EO_XmlSax2Parser.lib\")" "#pragma comment(lib,\"QtClient.lib\")"
call "%FILE_REPLACE%" -dir %~dp0..\..\include\ API.h -replace "//#pragma comment(lib,\"EO_XmlSax2Parser.lib\")" "#pragma comment(lib,\"QtClient.lib\")"

call "%FILE_REPLACE%" -dir %~dp0..\..\include\ Macro.h -replace "//#pragma comment(lib,\"HttpRequestd.lib\")" "#pragma comment(lib,\"QtClientd.lib\")"
call "%FILE_REPLACE%" -dir %~dp0..\..\include\ API.h -replace "//#pragma comment(lib,\"HttpRequestd.lib\")" "#pragma comment(lib,\"QtClientd.lib\")"
call "%FILE_REPLACE%" -dir %~dp0..\..\include\ Macro.h -replace "//#pragma comment(lib,\"HttpRequest.lib\")" "#pragma comment(lib,\"QtClient.lib\")"
call "%FILE_REPLACE%" -dir %~dp0..\..\include\ API.h -replace "//#pragma comment(lib,\"HttpRequest.lib\")" "#pragma comment(lib,\"QtClient.lib\")"

call "%FILE_REPLACE%" -dir %~dp0..\..\include\ Macro.h -replace "//#pragma comment(lib,\"WidgetFactoryd.lib\")" "#pragma comment(lib,\"QtClientd.lib\")"
call "%FILE_REPLACE%" -dir %~dp0..\..\include\ API.h -replace "//#pragma comment(lib,\"WidgetFactoryd.lib\")" "#pragma comment(lib,\"QtClientd.lib\")"
call "%FILE_REPLACE%" -dir %~dp0..\..\include\ Macro.h -replace "//#pragma comment(lib,\"WidgetFactory.lib\")" "#pragma comment(lib,\"QtClient.lib\")"
call "%FILE_REPLACE%" -dir %~dp0..\..\include\ API.h -replace "//#pragma comment(lib,\"WidgetFactory.lib\")" "#pragma comment(lib,\"QtClient.lib\")"

call "%FILE_REPLACE%" -dir %~dp0..\..\include\ Macro.h -replace "//#pragma comment(lib,\"QssStringd.lib\")" "#pragma comment(lib,\"QtClientd.lib\")"
call "%FILE_REPLACE%" -dir %~dp0..\..\include\ API.h -replace "//#pragma comment(lib,\"QssStringd.lib\")" "#pragma comment(lib,\"QtClientd.lib\")"
call "%FILE_REPLACE%" -dir %~dp0..\..\include\ Macro.h -replace "//#pragma comment(lib,\"QssString.lib\")" "#pragma comment(lib,\"QtClient.lib\")"
call "%FILE_REPLACE%" -dir %~dp0..\..\include\ API.h -replace "//#pragma comment(lib,\"QssString.lib\")" "#pragma comment(lib,\"QtClient.lib\")"

call "%FILE_REPLACE%" -dir %~dp0..\..\include\ Macro.h -replace "//#pragma comment(lib,\"QtControlsd.lib\")" "#pragma comment(lib,\"QtClientd.lib\")"
call "%FILE_REPLACE%" -dir %~dp0..\..\include\ API.h -replace "//#pragma comment(lib,\"QtControlsd.lib\")" "#pragma comment(lib,\"QtClientd.lib\")"
call "%FILE_REPLACE%" -dir %~dp0..\..\include\ Macro.h -replace "//#pragma comment(lib,\"QtControls.lib\")" "#pragma comment(lib,\"QtClient.lib\")"
call "%FILE_REPLACE%" -dir %~dp0..\..\include\ API.h -replace "//#pragma comment(lib,\"QtControls.lib\")" "#pragma comment(lib,\"QtClient.lib\")"

call "%FILE_REPLACE%" -dir %~dp0..\..\include\ Macro.h -replace "//#pragma comment(lib,\"DialogManagerd.lib\")" "#pragma comment(lib,\"QtClientd.lib\")"
call "%FILE_REPLACE%" -dir %~dp0..\..\include\ API.h -replace "//#pragma comment(lib,\"DialogManagerd.lib\")" "#pragma comment(lib,\"QtClientd.lib\")"
call "%FILE_REPLACE%" -dir %~dp0..\..\include\ Macro.h -replace "//#pragma comment(lib,\"DialogManager.lib\")" "#pragma comment(lib,\"QtClient.lib\")"
call "%FILE_REPLACE%" -dir %~dp0..\..\include\ API.h -replace "//#pragma comment(lib,\"DialogManager.lib\")" "#pragma comment(lib,\"QtClient.lib\")"

call "%FILE_REPLACE%" -dir %~dp0..\..\include\ Macro.h -replace "//#pragma comment(lib,\"JsonCppd.lib\")" "#pragma comment(lib,\"QtClientd.lib\")"
call "%FILE_REPLACE%" -dir %~dp0..\..\include\ API.h -replace "//#pragma comment(lib,\"JsonCppd.lib\")" "#pragma comment(lib,\"QtClientd.lib\")"
call "%FILE_REPLACE%" -dir %~dp0..\..\include\ Macro.h -replace "//#pragma comment(lib,\"JsonCpp.lib\")" "#pragma comment(lib,\"QtClient.lib\")"
call "%FILE_REPLACE%" -dir %~dp0..\..\include\ API.h -replace "//#pragma comment(lib,\"JsonCpp.lib\")" "#pragma comment(lib,\"QtClient.lib\")"

call "%FILE_REPLACE%" -dir %~dp0..\..\include\ Macro.h -replace "//#pragma comment(lib,\"Cinid.lib\")" "#pragma comment(lib,\"QtClientd.lib\")"
call "%FILE_REPLACE%" -dir %~dp0..\..\include\ API.h -replace "//#pragma comment(lib,\"Cinid.lib\")" "#pragma comment(lib,\"QtClientd.lib\")"
call "%FILE_REPLACE%" -dir %~dp0..\..\include\ Macro.h -replace "//#pragma comment(lib,\"Cini.lib\")" "#pragma comment(lib,\"QtClient.lib\")"
call "%FILE_REPLACE%" -dir %~dp0..\..\include\ API.h -replace "//#pragma comment(lib,\"Cini.lib\")" "#pragma comment(lib,\"QtClient.lib\")"