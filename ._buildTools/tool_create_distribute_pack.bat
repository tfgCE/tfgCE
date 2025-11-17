:InitialInfo
ECHO ....................................................................................................
ECHO .                                                                                                  .
ECHO .                                                                                                  .
ECHO . Usage                                                                                            .
ECHO . tool_create_distribute_pack [GameName] [MainBuildDir]                                            .
ECHO .                                                                                                  .
ECHO ....................................................................................................
ECHO . Other variables that are used are just like in tool_create_builds                                .                                                                   .
ECHO ....................................................................................................
REM .
REM . Internal tool for creating distribute pack
REM . Called by tool_create_build_*
REM .

SET startTime=%time%
SET errorCount=0

:GetParameters
IF "%1" EQU "" GOTO AssumeGameName
SET DP_GameName=%1
IF "%2" EQU "" GOTO AssumeBuildDir
SET DP_MainBuildDir=%2
GOTO PreCreateInfo

REM using DP_ prefix as otherwise msbuild uses those settings and everything lands in one place
:AssumeGameName
SET DP_GameName=xxx
:AssumeBuildDir
SET DP_MainBuildDir=build\%DP_GameName%
GOTO PreCreateInfo

@ECHO OFF

:PreCreateInfo
ECHO Creating distribution packs for %DP_GameName%, main build dir %DP_MainBuildDir%.
ECHO.
ECHO.

:UpdateBuildVer
ECHO Remove pre from version
PUSHD ..
buildVerUpdate --setActiveProject %DP_GameName% "code\core\buildVer.h"
POPD
@ECHO OFF

:PackToDistribute
ECHO Ready files to distribute
IF NOT EXIST "..\%DP_MainBuildDir%\distribute" MKDIR "..\%DP_MainBuildDir%\distribute" >NUL
ERASE "..\%DP_MainBuildDir%\distribute" /Q /S >NUL
CALL clean_dir.bat "..\%DP_MainBuildDir%\distribute"

:PackToDistribute_Windows
IF "%CBA_ForWindows%" == "yes" (
	ECHO Ready Windows
	SET DP_SrcDir=Windows
	SET DP_Suffix=_windows
	CALL tool_create_distribute_pack_x64.bat
)

:PackToDistribute_Windows_fullData
IF "%CBA_WindowsProfilerWithSourceAndConfigFiles%" == "yes" (
	ECHO Ready Windows_profiler_full
	SET DP_SrcDir=Windows_profiler_full
	SET DP_Suffix=_windows_profiler_full
	CALL tool_create_distribute_pack_x64.bat
)
:PackToDistribute_Windows_fullData
IF "%CBA_WindowsWithSourceFiles%" == "yes" (
	ECHO Ready Windows_fullData
	SET DP_SrcDir=Windows_fullData
	SET DP_Suffix=_windows_fullData
	CALL tool_create_distribute_pack_x64.bat
)
:PackToDistribute_Steam
IF "%CBA_ForSteam%" == "yes" (
	ECHO Ready Steam
	SET DP_SrcDir=steam
	SET DP_Suffix=_steam
	CALL tool_create_distribute_pack_x64.bat
)
:PackToDistribute_OculusPC
IF "%CBA_ForOculusPC%" == "yes" (
	ECHO Ready OculusPC
	SET DP_SrcDir=oculusPC
	SET DP_Suffix=_oculusPC
	CALL tool_create_distribute_pack_x64.bat
)
:PackToDistribute_VivePC
IF "%CBA_ForVivePC%" == "yes" (
	ECHO Ready VivePC
	SET DP_SrcDir=vivePC
	SET DP_Suffix=_vivePC
	CALL tool_create_distribute_pack_x64.bat
)
:PackToDistribute_VivePC_China
IF "%CBA_ForVivePC_China%" == "yes" (
	ECHO Ready VivePC China
	SET DP_SrcDir=vivePC_c
	SET DP_Suffix=_vivePC_c
	CALL tool_create_distribute_pack_x64.bat
)
:PackToDistribute_Quest
IF "%CBA_ForQuest%" == "yes" (
	ECHO Ready Quest
	SET DP_SrcDir=quest
	SET DP_Suffix=_quest
	CALL tool_create_distribute_pack_arm64.bat
)
:PackToDistribute_Vive
IF "%CBA_ForVive%" == "yes" (
	ECHO Ready Vive
	SET DP_SrcDir=vive
	SET DP_Suffix=_vive
	CALL tool_create_distribute_pack_arm64.bat
)
:PackToDistribute_Vive_China
IF "%CBA_ForVive_China%" == "yes" (
	ECHO Ready Vive China
	SET DP_SrcDir=vive_c
	SET DP_Suffix=_vive_c
	CALL tool_create_distribute_pack_arm64.bat
)
:PackToDistribute_Pico
IF "%CBA_ForPico%" == "yes" (
	ECHO Ready Pico
	SET DP_SrcDir=pico
	SET DP_Suffix=_pico
	CALL tool_create_distribute_pack_arm64.bat
)
:PackToDistribute_Pico_China
IF "%CBA_ForPico_China%" == "yes" (
	ECHO Ready Pico China
	SET DP_SrcDir=pico_c
	SET DP_Suffix=_pico_c
	CALL tool_create_distribute_pack_arm64.bat
)

:CopyBuildInfoWithBuildName
REM ECHO Create build info
REM PUSHD ..
REM buildVerUpdate --setActiveProject %DP_GameName% --createBuildInfoFile "%DP_MainBuildDir%\distribute\%DP_GameName%.buildInfo" "code\core\buildVer.h"
REM POPD
REM IF "%CBA_ForQuest%" == "yes" (
REM 	COPY "..\%DP_MainBuildDir%\distribute\%DP_GameName%.buildInfo" "..\%DP_MainBuildDir%\distribute\%DP_GameName%Quest.buildInfo" >NUL
REM )

:CopyToHaveGeneralNames
REM useful for preview channels
IF "%CBA_ForWindows%" == "yes" (
	COPY "..\%DP_MainBuildDir%\distribute\%DP_GameName%_windows.zip" "..\%DP_MainBuildDir%\distribute\%DP_GameName%.zip" >NUL
)
IF "%CBA_ForQuest%" == "yes" (
	COPY "..\%DP_MainBuildDir%\distribute\%DP_GameName%_quest.apk" "..\%DP_MainBuildDir%\distribute\%DP_GameName%.apk" >NUL
)

:UpdateBuildVer
ECHO Restore version to pre
PUSHD ..
buildVerUpdate --setActiveProject %DP_GameName% --buildPreview "code\core\buildVer.h"
POPD

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
ECHO Created distribute packs.
ECHO Time taken %mins%:%secs%.
ECHO.

:CleanUp
SET DP_GameName=
SET DP_MainBuildDir=

@ECHO ON


