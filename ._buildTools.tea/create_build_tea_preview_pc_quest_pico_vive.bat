@ECHO OFF

SET CBA_GameName=tea
SET CBA_GameProjectFolder=teaForGod

SET CB_BuildType=--buildPreviewRelease
SET CB_AndroidPackageName=com.voidroom.TeaForGodEmperor
SET CB_DefaultAndroidPackageName=com.voidroom.TeaForGodEmperor

REM Other projects required
SET CBA_ForAndroid_projectFile=teaForGodAndroid
SET CBA_ForWindows_projectFile=teaForGodWin

SET CBA_ForWindows=yes
SET CBA_ForWindows_projectFile=teaForGodWin
SET CBA_ForQuest=yes
SET CBA_ForQuest_projectFile=teaForGodQuest
SET CBA_ForVive=yes
SET CBA_ForVive_projectFile=teaForGodVive
SET CBA_ForPico=yes
SET CBA_ForPico_projectFile=teaForGodPico

REM only proper preview has additional "source files"
REM SET CBA_WindowsWithSourceFiles=yes

REM do additional profiler build with source files and config files
SET CBA_WindowsProfilerWithSourceAndConfigFiles=yes

PUSHD ..\._buildTools
call tool_create_build_all
POPD
@ECHO ON


