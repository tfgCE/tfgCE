@ECHO OFF
ECHO ........................................................................................................
ECHO .                                                                                                      .
ECHO .  In case of a failuer, look for:                                                                     .
ECHO .     Build failed.                                                                                    .
ECHO .                                                                                                      .
ECHO ........................................................................................................
ECHO .                                                                                                      .
ECHO . Usage                                                                                                .
ECHO . tool_create_build_all                                                                                .
ECHO .                                                                                                      .
ECHO ........................................................................................................
ECHO . Other variables that are used:                                                                       .
ECHO .  CBA_GameName                  name of the game                                                      .
ECHO .  CBA_GameProjectFolder         folder of project in the solution file                                .
ECHO .  CBA_WindowsProfilerWithSourceAndConfigFiles                                                         .
ECHO .    yes                         will create a profiler build with source and config files             .
ECHO .  CBA_WindowsWithSourceFiles                                                                          .
ECHO .    yes                         will create a build with source files                                 .
ECHO .  CBA_ForWindows                                                                                      .
ECHO .    yes                         will create a windows build for PC, generic [itch.io, download, etc]  .
ECHO .  CBA_ForWindows_projectFile    project file to use for build above (always have to be provided!)     .
ECHO .  CBA_ForQuest                                                                                        .
ECHO .    yes                         will create an android build for Quest                                .
ECHO .  CBA_ForQuest_projectFile      project file to use for build above                                   .
ECHO .  CBA_ForVive                                                                                         .
ECHO .  CBA_ForVive_China                                                                                   .
ECHO .    yes                         will create an android build for Vive                                 .
ECHO .  CBA_ForVive_projectFile       project file to use for build above                                   .
ECHO .  CBA_ForPico                                                                                         .
ECHO .  CBA_ForPico_China                                                                                   .
ECHO .    yes                         will create an android build for Pico                                 .
ECHO .  CBA_ForPico_projectFile       project file to use for build above                                   .
ECHO .  CBA_ForSteam                                                                                        .
ECHO .    yes                         will create a windows build for Steam                                 .
ECHO .  CBA_ForSteam_projectFile      project file to use for build above                                   .
ECHO .  CBA_ForOculusPC                                                                                     .
ECHO .    yes                         will create a windows build for Oculus store                          .
ECHO .  CBA_ForOculusPC_projectFile   project file to use for build above                                   .
ECHO .  CBA_ForVivePC                                                                                       .
ECHO .  CBA_ForVivePC_China                                                                                 .
ECHO .    yes                         will create a windows build for Vive store (viveport)                 .
ECHO .  CBA_ForVivePC_projectFile     project file to use for build above                                   .
ECHO ........................................................................................................
ECHO.

SET allStartTime=%time%

:GetParameters
GOTO PrepareBuildParams

:PrepareBuildParams
SET CB_DeployAlreadyDone=
IF "%CBA_WindowsProfilerWithSourceAndConfigFiles%" == "yes" (
	SET CBA_Requires_ForWindows_projectFile=yes
)
IF "%CBA_ForWindows%" == "yes" (
	SET CBA_BuildPC=yes
	SET CBA_Requires_ForWindows_projectFile=yes
)
REM these builds require PC build to be done [although not the full build]
IF NOT "%CBA_BuildPC%" == "yes" (
	IF "%CBA_WindowsWithSourceFiles%" == "yes" (
		ECHO Forcing PC build to be done
		SET CBA_BuildPC=yes
		SET CBA_Requires_ForWindows_projectFile=yes
	)
)
IF NOT "%CBA_BuildPC%" == "yes" (
	IF "%CBA_ForQuest%" == "yes" (
		ECHO Forcing PC build to be done
		SET CBA_BuildPC=yes
		SET CBA_Requires_ForWindows_projectFile=yes
	)
)
IF NOT "%CBA_BuildPC%" == "yes" (
	IF "%CBA_ForVive%" == "yes" (
		ECHO Forcing PC build to be done
		SET CBA_BuildPC=yes
		SET CBA_Requires_ForWindows_projectFile=yes
	)
)
IF NOT "%CBA_BuildPC%" == "yes" (
	IF "%CBA_ForVive_China%" == "yes" (
		ECHO Forcing PC build to be done
		SET CBA_BuildPC=yes
		SET CBA_Requires_ForWindows_projectFile=yes
	)
)
IF NOT "%CBA_BuildPC%" == "yes" (
	IF "%CBA_ForPico%" == "yes" (
		ECHO Forcing PC build to be done
		SET CBA_BuildPC=yes
		SET CBA_Requires_ForWindows_projectFile=yes
	)
)
IF NOT "%CBA_BuildPC%" == "yes" (
	IF "%CBA_ForPico_China%" == "yes" (
		ECHO Forcing PC build to be done
		SET CBA_BuildPC=yes
		SET CBA_Requires_ForWindows_projectFile=yes
	)
)
IF "%CBA_ForQuest%" == "yes" (
	SET CBA_Requires_ForAndroid_projectFile=yes
)
IF "%CBA_ForVive%" == "yes" (
	SET CBA_Requires_ForAndroid_projectFile=yes
)
IF "%CBA_ForVive_China%" == "yes" (
	SET CBA_Requires_ForAndroid_projectFile=yes
)
IF "%CBA_ForPico%" == "yes" (
	SET CBA_Requires_ForAndroid_projectFile=yes
)
IF "%CBA_ForPico_China%" == "yes" (
	SET CBA_Requires_ForAndroid_projectFile=yes
)

:ValidateBuildParams
IF "%CBA_Requires_ForWindows_projectFile%" == "yes" (
	IF "%CBA_ForWindows_projectFile%" EQU "" (
		ECHO This setup requires CBA_ForWindows_projectFile parameter to be provided. It is empty now.
		GOTO Failed
	)
)
IF "%CBA_Requires_ForAndroid_projectFile%" == "yes" (
	IF "%CBA_ForAndroid_projectFile%" EQU "" (
		ECHO This setup requires CBA_ForAndroid_projectFile parameter to be provided. It is empty now.
		GOTO Failed
	)
)
IF "%CBA_ForQuest%" == "yes" (
	IF "%CBA_ForQuest_projectFile%" EQU "" (
		ECHO This setup requires CBA_ForQuest_projectFile parameter to be provided. It is empty now.
		GOTO Failed
	)
)
IF "%CBA_ForVive%" == "yes" (
	IF "%CBA_ForVive_projectFile%" EQU "" (
		ECHO This setup requires CBA_ForVive_projectFile parameter to be provided. It is empty now.
		GOTO Failed
	)
)
IF "%CBA_ForVive_China%" == "yes" (
	IF "%CBA_ForVive_projectFile%" EQU "" (
		ECHO This setup requires CBA_ForVive_projectFile parameter to be provided. It is empty now.
		GOTO Failed
	)
)
IF "%CBA_ForPico%" == "yes" (
	IF "%CBA_ForPico_projectFile%" EQU "" (
		ECHO This setup requires CBA_ForPico_projectFile parameter to be provided. It is empty now.
		GOTO Failed
	)
)
IF "%CBA_ForPico_China%" == "yes" (
	IF "%CBA_ForPico_projectFile%" EQU "" (
		ECHO This setup requires CBA_ForPico_projectFile parameter to be provided. It is empty now.
		GOTO Failed
	)
)
IF "%CBA_ForSteam%" == "yes" (
	IF "%CBA_ForSteam_projectFile%" EQU "" (
		ECHO This setup requires CBA_ForSteam_projectFile parameter to be provided. It is empty now.
		GOTO Failed
	)
)
IF "%CBA_ForOculusPC%" == "yes" (
	IF "%CBA_ForOculusPC_projectFile%" EQU "" (
		ECHO This setup requires CBA_ForOculusPC_projectFile parameter to be provided. It is empty now.
		GOTO Failed
	)
)

:OutputBuildParams
ECHO Building params
ECHO CBA_GameProjectFolder=%CBA_GameProjectFolder%
ECHO CBA_GameName=%CBA_GameName%
ECHO CBA_WindowsWithSourceFiles=%CBA_WindowsWithSourceFiles%
ECHO CBA_ForAndroid_projectFile=%CBA_ForAndroid_projectFile%
ECHO CBA_ForWindows=%CBA_ForWindows%
ECHO CBA_ForWindows_projectFile=%CBA_ForWindows_projectFile%
ECHO CBA_ForQuest=%CBA_ForQuest%
ECHO CBA_ForQuest_projectFile=%CBA_ForQuest_projectFile%
ECHO CBA_ForVive=%CBA_ForVive%
ECHO CBA_ForVive_China=%CBA_ForVive_China%
ECHO CBA_ForVive_projectFile=%CBA_ForVive_projectFile%
ECHO CBA_ForPico=%CBA_ForPico%
ECHO CBA_ForPico_China=%CBA_ForPico_China%
ECHO CBA_ForPico_projectFile=%CBA_ForPico_projectFile%
ECHO CBA_ForSteam=%CBA_ForSteam%
ECHO CBA_ForSteam_projectFile=%CBA_ForSteam_projectFile%
ECHO CBA_ForOculusPC=%CBA_ForOculusPC%
ECHO CBA_ForOculusPC_projectFile=%CBA_ForOculusPC_projectFile%
ECHO CBA_ForVivePC=%CBA_ForVivePC%
ECHO CBA_ForVivePC_China=%CBA_ForVivePC_China%
ECHO CBA_ForVivePC_projectFile=%CBA_ForVivePC_projectFile%

:CreateBuilds
ECHO Create builds
REM each call tool_create_build builds all data
REM by default each call also compiles source files unless specified CB_SkipCompileBuild

@ECHO OFF
IF "%CBA_WindowsProfilerWithSourceAndConfigFiles%" == "yes" (
	ECHO BuildAll: Build Windows profiler with source and config files
	REM setup to keep source files and not compile build [would be the same]
	SET CB_KeepSourceFiles=yes
	SET CB_KeepDevConfigFiles=yes
	REM compile Windows build [to have a valid exe from previous as we skip compile build]
	call tool_create_build %CBA_GameName% %CBA_GameProjectFolder% %CBA_ForWindows_projectFile% Profiler x64 windows_profiler_full
	REM clear variables
	SET CB_KeepSourceFiles=
	SET CB_KeepDevConfigFiles=
)

@ECHO OFF
REM compile Windows/Win build [to have a valid exe to build data for APK]
IF "%CBA_BuildPC%" == "yes" (
	ECHO BuildAll: Build Windows
	call tool_create_build %CBA_GameName% %CBA_GameProjectFolder% %CBA_ForWindows_projectFile% Release x64 windows
)

@ECHO OFF
IF "%CBA_WindowsWithSourceFiles%" == "yes" (
	ECHO BuildAll: Build Windows with source files
	REM setup to keep source files and not compile build [would be the same]
	SET CB_KeepSourceFiles=yes
	SET CB_SkipCompileBuild=yes
	REM compile Windows build [to have a valid exe from previous as we skip compile build]
	call tool_create_build %CBA_GameName% %CBA_GameProjectFolder% %CBA_ForWindows_projectFile% Release x64 windows_fullData
	REM clear variables
	SET CB_KeepSourceFiles=
	SET CB_SkipCompileBuild=
)

@ECHO OFF
REM compile APK signed with release key - using ReleaseOculus/ReleaseVive/ReleasePico to do that, unless we use other configuration
REM perform this right after pc build [could be full data too]
REM applies to all android platforms

SET CB_ForAndroid_projectFile=%CBA_ForAndroid_projectFile%

@ECHO OFF
IF "%CBA_ForQuest%" == "yes" (
	ECHO BuildAll: Build Quest
	call tool_create_build %CBA_GameName% %CBA_GameProjectFolder% %CBA_ForQuest_projectFile% ReleaseOculus ARM64 quest
	SET CB_DeployAlreadyDone=yes
)

@ECHO OFF
IF "%CBA_ForVive%" == "yes" (
	ECHO BuildAll: Build Vive
	call tool_create_build %CBA_GameName% %CBA_GameProjectFolder% %CBA_ForVive_projectFile% ReleaseVive ARM64 vive
	SET CB_DeployAlreadyDone=yes
)

@ECHO OFF
IF "%CBA_ForVive_China%" == "yes" (
	ECHO BuildAll: Build Vive China
	call tool_create_build %CBA_GameName% %CBA_GameProjectFolder% %CBA_ForVive_projectFile% ReleaseVive ARM64 vive_c AN_CHINA
	SET CB_DeployAlreadyDone=yes
)

@ECHO OFF
IF "%CBA_ForPico%" == "yes" (
	ECHO BuildAll: Build Pico
	call tool_create_build %CBA_GameName% %CBA_GameProjectFolder% %CBA_ForPico_projectFile% ReleasePico ARM64 pico
	SET CB_DeployAlreadyDone=yes
)

@ECHO OFF
IF "%CBA_ForPico_China%" == "yes" (
	ECHO BuildAll: Build Pico China
	call tool_create_build %CBA_GameName% %CBA_GameProjectFolder% %CBA_ForPico_projectFile% ReleasePico ARM64 pico_c AN_CHINA
	SET CB_DeployAlreadyDone=yes
)

@ECHO OFF
IF "%CBA_ForSteam%" == "yes" (
	ECHO BuildAll: Build Steam
	call tool_create_build %CBA_GameName% %CBA_GameProjectFolder% %CBA_ForSteam_projectFile% ReleaseSteam x64 steam
)

@ECHO OFF
IF "%CBA_ForOculusPC%" == "yes" (
	ECHO BuildAll: Build OculusPC
	call tool_create_build %CBA_GameName% %CBA_GameProjectFolder% %CBA_ForOculusPC_projectFile% ReleaseOculus x64 oculusPC
	ERASE build\%CBA_GameName%\oculusPC\run_*.bat /Q /S
)

@ECHO OFF
IF "%CBA_ForVivePC%" == "yes" (
	ECHO BuildAll: Build VivePC
	call tool_create_build %CBA_GameName% %CBA_GameProjectFolder% %CBA_ForVivePC_projectFile% ReleaseVive x64 vivePC
	ERASE build\%CBA_GameName%\vivePC\run_*.bat /Q /S
)

@ECHO OFF
IF "%CBA_ForVivePC_China%" == "yes" (
	ECHO BuildAll: Build VivePC China
	call tool_create_build %CBA_GameName% %CBA_GameProjectFolder% %CBA_ForVivePC_projectFile% ReleaseVive x64 vivePC_c AN_CHINA
	ERASE build\%CBA_GameName%\vivePC\run_*.bat /Q /S
)

SET CB_ForAndroid_projectFile=

@ECHO OFF
ECHO BuildAll: create distribution package
REM create distirbution package, move all created execs, pack them, add buildInfo, etc.
call tool_create_distribute_pack %CBA_GameName%

@ECHO OFF
REM update build number/info [even if we're building a preview build]
CALL next_build_no.bat
@ECHO OFF

:TimeMeasure
SET allEndTime=%time%

REM break down into hours:minutes:seconds
SET options="tokens=1-3 delims=:.,"
FOR /f %options% %%a IN ("%allStartTime%") DO SET start_h=%%a&SET /a start_m=100%%b %% 100&SET /a start_s=100%%c %% 100
FOR /f %options% %%a IN ("%allEndTime%") DO SET end_h=%%a&SET /a end_m=100%%b %% 100&SET /a end_s=100%%c %% 100
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
ECHO Everything done.
ECHO BuildAll: time taken %mins%:%secs%.
ECHO.
GOTO Done

:Failed
ECHO Build failed.

:Done
SET CBA_BuildPC=
SET CB_DeployAlreadyDone=
@ECHO ON


