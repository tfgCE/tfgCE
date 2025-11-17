:Start

:GetParameters
IF %1 EQU "" GOTO Failed
GOTO CleanDir

:CleanDir
IF EXIST "%~1\*.*" (
	FOR /D %%d IN ("%~1\*.*") DO (
		CALL clean_dir.bat "%%d"
		ERASE /Q /S "%%d" >NUL
		RMDIR /Q /S "%%d" >NUL
	)
)
ERASE /Q /S "%~1\*.*" >NUL
GOTO CleanUp

:CleanUp
GOTO End

:Failed
@ECHO Missing parameters %1
GOTO End

:End
