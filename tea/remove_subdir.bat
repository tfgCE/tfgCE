REM %1 dir
REM %2 file filter

:Start

:GetParameters
IF %1 EQU "" GOTO Failed
IF %2 EQU "" GOTO Failed
GOTO RemoveSubDir

:RemoveSubDir
FOR /D %%d IN (%1\*.*) DO (
	IF "%%~nd" EQU "%2" (
		CALL clean_dir.bat "%%d" >NUL
		RMDIR "%%d" /Q /S >NUL
	)
	IF "%%~nd" NEQ "%2" (
		CALL remove_subdir.bat "%%d" %2
	)
)

:CleanUp
GOTO End

:Failed
@ECHO Missing parameters %1 inDir %2 subDirToRemove
GOTO End

:End
