:InitialInfo
@ECHO OFF
ECHO ....................................................................................................
ECHO .                                                                                                  .
ECHO .                                                                                                  .
ECHO . Usage                                                                                            .
ECHO . tool_create_deploy [GameName] [PrepareData] [PlatformOut] [SystemName] [BuildDir]                .
ECHO .                                                                                                  .
ECHO . note that it uses latest exec from build dir to create main config file and prepare data!        .
ECHO .                                                                                                  .
ECHO . PrepareData values: no - yes - baseConfig - dev - devSmall                                       .
ECHO .                                                                                                  .
ECHO ....................................................................................................
ECHO . Other variables that are used:                                                                   .
ECHO .  DT_Demo will remove all sub dirs with .fullGame extension                                       .
ECHO .          set to yes                                                                              .
ECHO ....................................................................................................
ECHO . We assume the compilation used for deploying is in 'windows' dir                                 .
ECHO ....................................................................................................
ECHO.

SET startTime=%time%
SET errorCount=0

:GetParameters
IF "%1" EQU "" GOTO AssumeGameName
SET DT_GameName=%1
IF "%2" EQU "" GOTO AssumePrepareData
SET DT_PrepareData=%2
IF "%3" EQU "" GOTO AssumePlatformOut
SET DT_PlatformOut=%3
IF "%4" EQU "" GOTO AssumeSystem
SET DT_System=%4
IF "%5" EQU "" GOTO AssumeBuildDir
SET DT_BuildDir=%5
GOTO PostProvidedParams

REM using DT_ prefix as otherwise msbuild uses those settings and everything lands in one place
:AssumeGameName
SET DT_GameName=xxx
:AssumePrepareData
SET DT_PrepareData=no
:AssumePlatformOut
SET DT_PlatformOut=android
:AssumeSystem
SET DT_System=androidHeadset
:AssumeBuildDir
SET DT_BuildDir=deploy\%DT_GameName%\%DT_PlatformOut%
GOTO PostProvidedParams

:PostProvidedParams

:OtherParams
SET DT_PrepareDataAny=false
SET DT_PrepareDataBaseConfig=false
SET DT_PrepareDataDevConfig=false
IF "%DT_PrepareData%" == "yes" (
	SET DT_PrepareDataAny=true
)
IF "%DT_PrepareData%" == "baseConfig" (
	SET DT_PrepareDataAny=true
	SET DT_PrepareDataBaseConfig=true
)
IF "%DT_PrepareData%" == "dev" (
	SET DT_PrepareDataAny=true
	SET DT_PrepareDataBaseConfig=true
	SET DT_PrepareDataDevConfig=true
)
IF "%DT_PrepareData%" == "devSmall" (
	SET DT_PrepareDataAny=true
	SET DT_PrepareDataBaseConfig=true
	SET DT_PrepareDataDevConfig=true
)
IF DEFINED DT_Demo (
	REM ok
) ELSE (
	SET DT_Demo=no
) 

:PreDeployInfo
ECHO Creating deploy data in dir %DT_BuildDir% for %DT_GameName%.
ECHO.
ECHO.

:ClearDestinationDir
ECHO Clean up
ERASE ..\%DT_BuildDir% /Q /S >NUL
ERASE ..\%DT_BuildDir%\*.* /Q /S >NUL
CALL clean_dir.bat ..\%DT_BuildDir%
IF NOT EXIST ..\%DT_BuildDir% MKDIR ..\%DT_BuildDir% >NUL

:RemoveLogFiles
REM this is done to speed up copying
ECHO Remove log files [from source]
CALL remove_subdir.bat ..\%DT_GameName%\runtime-data _logs

:RemoveVRConfigFiles
REM this is done to speed up copying
ECHO Remove vr config files [from source]
CALL remove_subdir.bat ..\%DT_GameName%\runtime-data openvr

:RemoveOtherFiles
REM as above + gives a fresh start after rebuilding
ECHO Remove assets and cxs [from source]
CALL remove_file.bat ..\%DT_GameName%\runtime-data *.snd
CALL remove_file.bat ..\%DT_GameName%\runtime-data *.tex
CALL remove_file.bat ..\%DT_GameName%\runtime-data *.cx
CALL remove_file.bat ..\%DT_GameName%\runtime-data *.mesh
CALL remove_file.bat ..\%DT_GameName%\runtime-data *.skel
CALL remove_file.bat ..\%DT_GameName%\runtime-data *.animSet
CALL remove_file.bat ..\%DT_GameName%\runtime-data system.tags

:CopyData
ECHO Copy files
XCOPY ..\%DT_GameName%\runtime-data\*.* ..\%DT_BuildDir%\*.* /E /Y >NUL

:RemoveFullGameForDemo
IF "%DT_Demo%" == "yes" (
	ECHO Remove .fullGame from deploy [building demo]
	CALL remove_subdir_ext.bat ..\%DT_BuildDir%\library .fullGame
) ELSE (
	ECHO Keep .fullGame in deploy [building full game]
)

:CopyExecFiles
Copy "..\Build\%DT_GameName%\windows\*.dll" "..\%DT_BuildDir%" >NUL
Copy "..\Build\%DT_GameName%\windows\*.exe" "..\%DT_BuildDir%" >NUL

:PrepareDataAndMainConfig
IF "%DT_PrepareDataAny%" == "true" (
	IF "%DT_PrepareData%" == "devSmall" (
		ECHO Remove files that shouldn't be part of devSmall
		CALL remove_subdir.bat ..\%DT_BuildDir%\library pilgrimage
		CALL remove_file.bat ..\%DT_BuildDir%\library\testVR region.xml
		CALL remove_subdir.bat ..\%DT_BuildDir%\library places
		REM CALL remove_subdir.bat ..\%DT_BuildDir%\library npcs
	)
	PUSHD ..\%DT_BuildDir%
	ECHO Prepare data and main config
	CALL %DT_GameName%.exe --loadLibraryAndExit --createMainConfigFile %DT_System%
	POPD
) ELSE (
	PUSHD ..\%DT_BuildDir%
	ECHO Prepare main config
	CALL %DT_GameName%.exe --createMainConfigFileAndExit %DT_System%
	POPD
)

:RemoveExecFiles
CALL remove_file.bat ..\%DT_BuildDir% *.exe
CALL remove_file.bat ..\%DT_BuildDir% *.dll

:RemoveAutoFiles
ECHO Remove files that should be not part of the deploy
CALL remove_auto_files.bat ..\%DT_BuildDir%

:RemoveCodeInfo
CALL remove_file.bat ..\%DT_BuildDir%\system codeInfo.*

:RemoveTXTs
CALL remove_file.bat ..\%DT_BuildDir% *.txt

:RemoveBatchFiles
CALL remove_file.bat ..\%DT_BuildDir% *.bat
CALL remove_file.bat ..\%DT_BuildDir%\shaderProgramCaches *.bat

:RemoveEXEDLLS
CALL remove_file.bat ..\%DT_BuildDir% *.dll
CALL remove_file.bat ..\%DT_BuildDir% *.exe

:CopyDevConfigs
IF "%DT_PrepareDataBaseConfig%" == "true" (
	ECHO Copy base configs
	Copy "..\%DT_GameName%\runtime-data\_baseConfig.xml" "..\%DT_BuildDir%" >NUL
	Copy "..\%DT_GameName%\runtime-data\_baseInputConfig.xml" "..\%DT_BuildDir%" >NUL
)

IF "%DT_PrepareDataDevConfig%" == "true" (
	ECHO Copy dev configs
	Copy "..\%DT_GameName%\runtime-data\_devConfig*.xml" "..\%DT_BuildDir%" >NUL
)

:FlattenDirs
ECHO Flatten cx files [keeping original filepaths]
REM Flatten only cx files
CALL flatten_subdir.bat "..\%DT_BuildDir%\settings" "..\%DT_BuildDir%\temp" "*.cx"
CALL flatten_subdir.bat "..\%DT_BuildDir%\library" "..\%DT_BuildDir%\temp" "*.cx"
CALL flatten_subdir.bat "..\%DT_BuildDir%\system" "..\%DT_BuildDir%\temp" "*.cx"

REM skip error
GOTO End

:Error
SET errorCount=1

:End
REM show error or nothing

:TimeMeasure
SET endTime=%time%

REM break down into hours:minutes:seconds
SET options="tokens=1-3 delims=:.,"
FOR /f %options% %%a IN ("%startTime%") DO SET start_h=%%a&SET /a start_m=100%%b %% 100&SET /a start_s=100%%c %% 100
FOR /f %options% %%a IN ("%endTime%") DO SET end_h=%%a&SET /a end_m=100%%b %% 100&SET /a end_s=100%%c %% 100
SET /a hours=%end_h%-%start_h%
SET /a mins=%end_m%-%start_m%
SET /a secs=%end_s%-%start_s%

REM if we get negative value, assume one more minute/hour has passed
IF %secs% lss 0 SET /a mins = %mins% - 1 & SET /a secs = 60%secs%
IF %mins% lss 0 SET /a hours = %hours% - 1 & SET /a mins = 60%mins%
IF %hours% lss 0 SET /a hours = 24%hours%

REM just minutes + seconds
SET /a mins=%mins% + 60 * %hours%

REM format seconds into two digits
set secs=00%secs%
set secs=%secs:~-2%

:PostBuildInfo
ECHO.
ECHO.
IF %errorCount% LSS 1 (
	ECHO Deploy for %DT_GameName% has been created in dir %DT_BuildDir%.
) ELSE (
	ECHO Deploy failed.
)
ECHO Time taken %mins%:%secs%.
ECHO.

:CleanUp
SET DT_GameName=
SET DT_PlatformOut=
SET DT_BuildDir=
SET DT_System=
@ECHO ON


