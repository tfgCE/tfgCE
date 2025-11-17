:InitialInfo
@ECHO OFF
ECHO ....................................................................................................
ECHO .                                                                                                  .
ECHO .                                                                                                  .
ECHO . Usage                                                                                            .
ECHO . tool_create_build [GameName] [VSProjectName] [Configuration] [Platform] [PlatformOut] \          .
ECHO .              [AdditionalBuildSetting] [BuildDir] [SuffixForExe]                                  .
ECHO .                                                                                                  .
ECHO ....................................................................................................
ECHO . Other variables that are used:                                                                   .
ECHO .  CB_BuildType that can be set to                                                                 .
ECHO .    --buildPreview to create a preview version                                                    .
ECHO .    --buildPreviewRelease to create a public preview release version                              .
ECHO .    --buildPreviewPublicReleaseCandidate to create a public preview of public release candidate   .
ECHO .    --buildPublicRelease to create a public release version                                       .
ECHO .    --buildPublicDemoRelease to create a public demo release version                              .
ECHO .  CB_AndroidPackageName that is used to alter package name for AndroidManifest.xml                .
ECHO .  CB_DefaultAndroidPackageName to revert afterwards                                               .
ECHO .  CB_KeepSourceFiles that can be set to [used by remove_auto_files.bat]                           .
ECHO .    yes	will keep xml, txt and asset files                                                      .
ECHO .  CB_KeepDevConfigFiles that can be set to [used by remove_auto_files.bat]                        .
ECHO .    yes	will keep various config files                                                          .
ECHO .  CB_SkipCompileBuild that can be set to                                                          .
ECHO .    yes	will skip compiling a build                                                             .
ECHO .  CB_ForAndroid_projectFile required for ARM64 builds (native code)                               .
ECHO ....................................................................................................
ECHO . Don't forget to call next_build_no.bat [unless you use create_build_xxx_win.bat etc]             .
ECHO . ARM64 requires Windows to be built first                                                         .
ECHO ....................................................................................................
ECHO.
SET startTime=%time%
SET errorCount=0

ECHO ....................................................................................................
ECHO . IMPORTANT                                                                                        .
ECHO . There are some hardcoded paths, because I can't be bothered with that                            .
ECHO . I just gave up and use hardoded path, they are marked HARDCODED_PATH                             .
ECHO ....................................................................................................
ECHO.

REM here is a HARDCODED_PATH used to get access to apksigner
SET PATH=%ProgramFiles(x86)%\Microsoft Visual Studio\2019\Community\MSBuild\Current\Bin;C:\Microsoft\AndroidSDK\25\build-tools\25.0.3;%PATH%

:GetParameters
IF "%1" EQU "" GOTO AssumeGameName
SET CB_GameName=%1
IF "%2" EQU "" GOTO AssumeVSProjectFolder
SET CB_VSProjectFolder=%2
IF "%3" EQU "" GOTO AssumeVSProjectName
SET CB_VSProjectName=%3
IF "%4" EQU "" GOTO AssumeConfiguration
SET CB_Configuration=%4
IF "%5" EQU "" GOTO AssumePlatform
SET CB_Platform=%5
IF "%6" EQU "" GOTO AssumePlatformOut
SET CB_PlatformOut=%6
IF "%7" EQU "" GOTO AssumeAdditionalBuildSetting
SET CB_AdditionalBuildSetting=%7
IF "%8" EQU "" GOTO AssumeBuildDir
SET CB_BuildDir=%8
IF "%9" EQU "" GOTO AssumeBuildSuffix
SET CB_BuildSuffix=%9
GOTO PreBuildInfo

REM using CB_ prefix as otherwise msbuild uses those settings and everything lands in one place
:AssumeGameName
SET CB_GameName=xxx
:AssumeVSProjectFolder
SET CB_VSProjectFolder=%CB_GameName%
:AssumeVSProjectName
SET CB_VSProjectName=%CB_GameName%
:AssumeConfiguration
SET CB_Configuration=Release
:AssumePlatform
SET CB_Platform=x64
:AssumePlatformOut
SET CB_PlatformOut=windows
:AssumeAdditionalBuildSetting
SET CB_AdditionalBuildSetting=none
:AssumeBuildDir
SET CB_BuildDir=build\%CB_GameName%\%CB_PlatformOut%
:AssumeBuildSuffix
SET CB_BuildSuffix=
GOTO PreBuildInfo

:PreBuildInfo
ECHO Creating build %CB_Configuration% in dir %CB_BuildDir%.
ECHO Compile project %CB_VSProjectName%
ECHO Main project %CB_GameName%
ECHO.
ECHO.

:MakeSureBuildUpdateVerExists
CALL build_buildVerUpdate.bat

:ShouldSkipCompileBuild
IF "%CB_SkipCompileBuild%" == "yes" (GOTO CheckBuildResult)

:CheckAndroidPackageName
IF DEFINED CB_AndroidPackageName (
	SET CB_AndroidPackageNameParam=--androidPackageName=
	IF NOT DEFINED CB_DefaultAndroidPackageName (
		SET CB_DefaultAndroidPackageName=%CB_AndroidPackageName%
	)
) ELSE (
	SET CB_AndroidPackageNameParam=
	SET CB_AndroidPackageName=
	SET CB_DefaultAndroidPackageName=
)

:CheckIfDemo
IF DEFINED CB_BuildType (
	IF "%CB_BuildType%" == "--buildPublicDemoRelease" (
		SET DT_Demo=yes
	)
)
IF DEFINED DT_Demo (
	REM ok
) ELSE (
	SET DT_Demo=no
) 

:CopyAndroidCommonFiles
IF "%CB_Platform%" == "ARM64" (
	IF "%CB_ForAndroid_projectFile%" EQU "" (
		ECHO Build failed.
		ECHO Parameter CB_ForAndroid_projectFile missing!
		GOTO Error
	) ELSE (
		ECHO Copying common android files
		CALL remove_subdir.bat ..\code\%CB_VSProjectName%\ res
		CALL remove_file.bat ..\code\%CB_VSProjectName%\ build.xml
		CALL remove_file.bat ..\code\%CB_VSProjectName%\ project.properties
		PUSHD ..\code\%CB_VSProjectName%\
		MKDIR res
		POPD
		XCOPY ..\code\%CB_ForAndroid_projectFile%\res\*.* ..\code\%CB_VSProjectName%\res\*.* /E /Y >NUL
		XCOPY ..\code\%CB_ForAndroid_projectFile%\build.* ..\code\%CB_VSProjectName%\build.* /E /Y >NUL
		XCOPY ..\code\%CB_ForAndroid_projectFile%\project.* ..\code\%CB_VSProjectName%\project.* /E /Y >NUL
	)
)

:UpdateVersionToRelease
IF DEFINED CB_BuildType (
	ECHO Updating build ver to %CB_BuildType%
) ELSE (
	ECHO Updating build ver to default - public release
	SET CB_BuildType=--buildPublicRelease
) 
PUSHD ..
IF "%CB_Platform%" == "ARM64" (
	buildVerUpdate %CB_BuildType% %CB_AndroidPackageNameParam%%CB_AndroidPackageName% --additionalBuildSetting %CB_AdditionalBuildSetting% --setActiveProject %CB_GameName% "code\core\buildVer.h" "code\%CB_VSProjectName%\project.properties" "code\%CB_VSProjectName%\AndroidManifest.xml"
	buildVerUpdate %CB_BuildType% %CB_AndroidPackageNameParam%%CB_AndroidPackageName% --additionalBuildSetting %CB_AdditionalBuildSetting% --setActiveProject %CB_GameName% "code\core\buildVer.h" "code\%CB_ForAndroid_projectFile%\project.properties" "code\%CB_VSProjectName%\AndroidManifest.xml"
) ELSE (
	buildVerUpdate %CB_BuildType% --additionalBuildSetting %CB_AdditionalBuildSetting% --setActiveProject %CB_GameName% "code\core\buildVer.h"
)
POPD

:MakeSureThereIsNoExec
IF "%CB_Platform%" == "x64" (
	ERASE "..\runtime\%CB_Platform%\%CB_Configuration%\%CB_VSProjectName%.exe" >NUL
)

:MakeSureThereIsNoBuild
ECHO Make sure there is no build
ERASE "..\runtime\%CB_Platform%\%CB_Configuration%" /Q /S >NUL
ERASE "..\runtime\%CB_Platform%\%CB_Configuration%\*.*" /Q /S >NUL
CALL clean_dir.bat "..\runtime\%CB_Platform%\%CB_Configuration%\"

:CreateDeployData
IF "%CB_Platform%" == "ARM64" (
	IF "%CB_DeployAlreadyDone%" EQU "" (
		REM We have to deploy data first as it is packed into the apk during compilation
		ECHO Deploy data for %CB_GameName%
		CALL tool_create_deploy.bat %CB_GameName% yes
		REM restore after deploy
		@ECHO OFF
	) ELSE (
		ECHO No need to deploy data for %CB_GameName%, already deployed
	)
)

:CopyCorrectIcons
IF "%CB_Platform%" == "ARM64" (
	IF "%DT_Demo%" == "yes" (
		IF EXIST "..\code\%CB_ForAndroid_projectFile%\res.demo" (
			ECHO Copy demo icons
			CALL clean_dir.bat "..\code\%CB_ForAndroid_projectFile%\res\"
			XCOPY ..\code\%CB_ForAndroid_projectFile%\res.demo\*.* ..\code\%CB_ForAndroid_projectFile%\res\*.* /E /Y >NUL
		)
	) ELSE (
		IF EXIST "..\code\%CB_ForAndroid_projectFile%\res.full" (
			ECHO Copy full icons
			CALL clean_dir.bat "..\code\%CB_ForAndroid_projectFile%\res\"
			XCOPY ..\code\%CB_ForAndroid_projectFile%\res.full\*.* ..\code\%CB_ForAndroid_projectFile%\res\*.* /E /Y >NUL
		)
	)
)

:CompileBuild
ECHO Compiling build %CB_Configuration% using ms build [vs2019, current].
msbuild.exe ..\aironoevl.sln /m /p:Configuration=%CB_Configuration%,Platform=%CB_Platform% -target:projects\%CB_VSProjectFolder%\platforms\%CB_VSProjectName%:Rebuild
ECHO Compiled build.

:CopyFullIconsBack
IF "%CB_Platform%" == "ARM64" (
	IF "%DT_Demo%" == "yes" (
		IF EXIST "..\code\%CB_ForAndroid_projectFile%\res.full" (
			ECHO Copy full icons back
			CALL clean_dir.bat "..\code\%CB_ForAndroid_projectFile%\res\"
			XCOPY ..\code\%CB_ForAndroid_projectFile%\res.full\*.* ..\code\%CB_ForAndroid_projectFile%\res\*.* /E /Y >NUL
		)
	)
)

:CheckBuildResult
IF "%CB_Platform%" == "x64" (
	ECHO Check if exists: "runtime\%CB_Platform%\%CB_Configuration%\%CB_VSProjectName%.exe"
	IF EXIST "..\runtime\%CB_Platform%\%CB_Configuration%\%CB_VSProjectName%.exe" (GOTO BuildExists) ELSE (GOTO Error)
)
IF "%CB_Platform%" == "ARM64" (
	ECHO Check if exists: "runtime\%CB_Platform%\%CB_Configuration%\%CB_VSProjectName%.apk"
	IF EXIST "..\runtime\%CB_Platform%\%CB_Configuration%\%CB_VSProjectName%.apk" (GOTO BuildExists) ELSE (GOTO Error)
)

:BuildExists
REM follow with other steps
ECHO Build exists.

:SignAPK
IF "%CB_Platform%" == "ARM64" (
	IF "%CB_Configuration%" == "ReleaseOculus" (
	 	ECHO Sign APK using apksigner
		REM keystore and password reflect what is setup in code\xxxQuest\build.xml
		CALL apksigner.bat sign --verbose -ks ..\code\tea_for_god_open.keystore --ks-pass pass:passwordgoeshere "..\runtime\%CB_Platform%\%CB_Configuration%\%CB_VSProjectName%.apk"
	)
	IF "%CB_Configuration%" == "ReleaseVive" (
	 	ECHO Sign APK using apksigner
		REM keystore and password reflect what is setup in code\xxxVive\build.xml
		CALL apksigner.bat sign --verbose -ks ..\code\tea_for_god_open.keystore --ks-pass pass:passwordgoeshere "..\runtime\%CB_Platform%\%CB_Configuration%\%CB_VSProjectName%.apk"
	)
	IF "%CB_Configuration%" == "ReleasePico" (
	 	ECHO Sign APK using apksigner
		REM keystore and password reflect what is setup in code\xxxPico\build.xml
		CALL apksigner.bat sign --verbose -ks ..\code\tea_for_god_open.keystore --ks-pass pass:passwordgoeshere "..\runtime\%CB_Platform%\%CB_Configuration%\%CB_VSProjectName%.apk"
	)
)

:ClearDestinationDir
ECHO Clean up
ERASE ..\%CB_BuildDir% /Q /S >NUL
ERASE ..\%CB_BuildDir%\*.* /Q /S >NUL
CALL clean_dir.bat ..\%CB_BuildDir%
IF NOT EXIST ..\%CB_BuildDir% MKDIR ..\%CB_BuildDir% >NUL

:ChoosePlatformSpecific
IF "%CB_Platform%" == "ARM64" GOTO ARM64_Start

GOTO x64_Start

REM .........
REM .       .
REM .  x64  .
REM .       .
REM .........

:x64_Start

:x64_CopyExternals
ECHO Copy files
COPY "..\externals\runtime\%CB_Platform%\*.dll" "..\%CB_BuildDir%\*.dll" >NUL
COPY "..\externals\runtime\%CB_Platform%\%CB_Configuration%\*.dll" "..\%CB_BuildDir%\*.dll" >NUL
COPY "..\externals\runtime\%CB_Platform%\%CB_VSProjectName%\*.dll" "..\%CB_BuildDir%\*.dll" >NUL
COPY "..\externals\runtime\%CB_Platform%\%CB_VSProjectName%\%CB_Configuration%\*.dll" "..\%CB_BuildDir%\*.dll" >NUL

:x64_CopyExec
COPY "..\runtime\%CB_Platform%\%CB_Configuration%\%CB_VSProjectName%.exe" "..\%CB_BuildDir%\%CB_GameName%%CB_BuildSuffix%.exe" >NUL

:x64_CopyPDB
IF NOT EXIST "..\%CB_BuildDir%\pdb" MKDIR "..\%CB_BuildDir%\pdb"
COPY "..\runtime\%CB_Platform%\%CB_Configuration%\%CB_VSProjectName%.pdb" "..\%CB_BuildDir%\pdb\%CB_GameName%%CB_BuildSuffix%.pdb" >NUL

:x64_CopyData
XCOPY ..\%CB_GameName%\runtime-data\*.* ..\%CB_BuildDir%\*.* /E /Y >NUL

:x64_RemoveBuiltFiles
ECHO Remove files that will be built
CALL remove_file.bat ..\%CB_BuildDir% *.snd
CALL remove_file.bat ..\%CB_BuildDir% *.tex
CALL remove_file.bat ..\%CB_BuildDir% *.cx
CALL remove_file.bat ..\%CB_BuildDir% *.mesh
CALL remove_file.bat ..\%CB_BuildDir% *.skel
CALL remove_file.bat ..\%CB_BuildDir% *.animSet

:x64_RemoveFullGameForDemo
IF "%DT_Demo%" == "yes" (
	ECHO Remove .fullGame from deploy [building demo]
	CALL remove_subdir_ext.bat ..\%CB_BuildDir%\library .fullGame
)

:x64_CreateCodeBase_BuildFiles_SaveMainConfigFile
ECHO Create code info, build files, save main config file
PUSHD ..\%CB_BuildDir%
CALL %CB_GameName%%CB_BuildSuffix%.exe --createCodeInfo --loadLibraryAndExit --createMainConfigFile
POPD

:x64_RemoveAutoFiles
ECHO Remove files that should not be part of the build
CALL remove_auto_files.bat ..\%CB_BuildDir%

:x64_RemoveBatchFiles
ECHO Remove batch files that should not be part of the build
CALL remove_file.bat ..\%CB_BuildDir%\shaderProgramCaches *.bat

:x64_RemoveShaderProgramCaches
ECHO Remove shader program caches for x64 builds
CALL remove_subdir.bat ..\%CB_BuildDir% shaderProgramCaches

:x64_RemoveBuildSpecific
REM use cpp rest for windows, more sure logs get through
REM CALL remove_file.bat ..\%CB_BuildDir% cpprest141_2_10.dll

GOTO FinaliseBuild

REM ...........
REM .         .
REM .  ARM64  .
REM .         .
REM ...........

:ARM64_Start

:ARM64_CopyAPK
ECHO copy apk
COPY "..\runtime\%CB_Platform%\%CB_Configuration%\%CB_VSProjectName%.apk" "..\%CB_BuildDir%\%CB_GameName%%CB_BuildSuffix%.apk"

:ARM64_CopySO
ECHO copy so (using native for_android)
COPY "..\temp\intermediate\%CB_Platform%\%CB_Configuration%\%CB_ForAndroid_projectFile%.NativeActivity\lib%CB_ForAndroid_projectFile%.so" "..\%CB_BuildDir%\%CB_GameName%%CB_BuildSuffix%.so"

GOTO FinaliseBuild

REM ..............
REM .            .
REM .  Finalise  .
REM .            .
REM ..............

:FinaliseBuild
REM this is end point for build

:UpdateBuildVer
ECHO Restore version to pre
PUSHD ..
IF "%CB_Platform%" == "ARM64" (
	buildVerUpdate --buildPreview %CB_AndroidPackageNameParam%%CB_DefaultAndroidPackageName% --additionalBuildSetting none --setActiveProject %CB_GameName% "code\core\buildVer.h" "code\%CB_VSProjectName%\project.properties" "code\%CB_VSProjectName%\AndroidManifest.xml"
	buildVerUpdate --buildPreview %CB_AndroidPackageNameParam%%CB_DefaultAndroidPackageName% --additionalBuildSetting none --setActiveProject %CB_GameName% "code\core\buildVer.h" "code\%CB_ForAndroid_projectFile%\project.properties" "code\%CB_VSProjectName%\AndroidManifest.xml"
) ELSE (
	buildVerUpdate --buildPreview %CB_AndroidPackageNameParam%%CB_DefaultAndroidPackageName% --additionalBuildSetting none --setActiveProject %CB_GameName% "code\core\buildVer.h"
)
POPD

REM skip error
GOTO End

:Error
SET errorCount=1
ECHO Encountered problem.
ECHO Restore version to pre
PUSHD ..
IF "%CB_Platform%" == "ARM64" (
	buildVerUpdate --buildPreview %CB_AndroidPackageNameParam%%CB_DefaultAndroidPackageName% --additionalBuildSetting none --setActiveProject %CB_GameName% "code\core\buildVer.h" "code\%CB_VSProjectName%\project.properties" "code\%CB_VSProjectName%\AndroidManifest.xml"
	buildVerUpdate --buildPreview %CB_AndroidPackageNameParam%%CB_DefaultAndroidPackageName% --additionalBuildSetting none --setActiveProject %CB_GameName% "code\core\buildVer.h" "code\%CB_ForAndroid_projectFile%\project.properties" "code\%CB_VSProjectName%\AndroidManifest.xml"
) ELSE (
	buildVerUpdate --buildPreview %CB_AndroidPackageNameParam%%CB_DefaultAndroidPackageName% --additionalBuildSetting none --setActiveProject %CB_GameName% "code\core\buildVer.h"
)
POPD

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
	ECHO Build based on %CB_Configuration%, %CB_Platform% has been created in dir %CB_BuildDir%.
) ELSE (
	ECHO Build failed.
)
ECHO Time taken %mins%:%secs%.
ECHO.

:CleanUp
SET CB_GameName=
SET CB_VSProjectName=
SET CB_Platform=
SET CB_PlatformOut=
SET CB_Configuration=
SET CB_BuildDir=
SET CB_BuildSuffix=
SET CL=
@ECHO ON


