set bat=%~dp0
set xueyelingshuang=%bat%..\..\

xcopy /y /i /r /s "%xueyelingshuang%src\dtws\dtws\res\*" "%xueyelingshuang%lib\res\"
xcopy /y /i /r /s "%xueyelingshuang%src\dtws\dtws\dtws.db" "%xueyelingshuang%lib\"

call "%CLOUD_REBUILD%" ScreenScript 32 dll release same
call "%CLOUD_REBUILD%" CKeyboard 32 dll release same
call "%CLOUD_REBUILD%" ConfigManager 32 dll release same
call "%CLOUD_REBUILD%" CDump 32 dll release same
call "%CLOUD_REBUILD%" CStopWatch 32 dll release same
call "%CLOUD_REBUILD%" DialogManager 32 dll release same