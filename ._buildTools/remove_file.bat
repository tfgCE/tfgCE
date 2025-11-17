REM %1 dir
REM %2 file

:Start

:GetParameters
IF %1 EQU "" GOTO Failed
IF %2 EQU "" GOTO Failed
GOTO DeleteFile

:DeleteFile
IF EXIST %1\%2 ERASE %1\%2 /Q /S >NUL

:GoThroughSubDirs
FOR /D %%d IN (%1\*.*) DO (
	IF "%%~nd" NEQ "%2" (
		CALL remove_file.bat "%%d" %2
	)
)

:CleanUp
GOTO End

:Failed
@ECHO Missing parameters %1 inDir %2 fileToRemove
GOTO End

:End
