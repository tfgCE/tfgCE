REM %1 dir
REM %2 temp dir
REM %3 filter

:Start

:GetParameters
IF %1 EQU "" GOTO Failed
IF %2 EQU "" GOTO Failed
IF %3 EQU "" GOTO Failed
GOTO FlattenSubDir

:FlattenSubDir
MKDIR "%~2" >NUL
CALL flatten_subdir_worker.bat "%~1" "%~2" "%~3"
REM only if empty, leave whole structure
XCOPY "%~2\*.*" "%~1\*.*" /E /Y >NUL
RMDIR /S /Q "%~2"

:CleanUp
GOTO End

:Failed
@ECHO Missing parameters %1 dir %2 temp %3 filter
GOTO End

:End
