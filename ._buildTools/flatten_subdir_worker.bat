REM %1 from
REM %2 to
REM %3 filter
REM %4 flatten prefix

:Start

:GetParameters
IF %1 EQU "" GOTO Failed
IF %2 EQU "" GOTO Failed
GOTO FlattenSubDir

:FlattenSubDir
REM copy files with prefixes first
FOR %%d IN ("%~1\%~3") DO (
	COPY "%%d" "%~2\%~4%%~nd%%~xd" >NUL
	ERASE "%%d"
)

REM go through directories
FOR /D %%d IN ("%~1\*.*") DO (
	CALL flatten_subdir_worker.bat "%%d" "%~2" "%~3" "%~4%%~nd%%~xd_"
)

:CleanUp
GOTO End

:Failed
@ECHO Missing parameters %1 from %2 to %3 flatten prefix
GOTO End

:End
