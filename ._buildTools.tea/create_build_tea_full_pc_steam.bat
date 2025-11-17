@ECHO OFF

SET CBA_GameName=tea
SET CBA_GameProjectFolder=teaForGod

SET CB_AndroidPackageName=com.voidroom.TeaForGodEmperor
SET CB_DefaultAndroidPackageName=com.voidroom.TeaForGodEmperor

REM Other projects required
SET CBA_ForAndroid_projectFile=teaForGodAndroid
SET CBA_ForWindows_projectFile=teaForGodWin

SET CBA_ForWindows=yes
SET CBA_ForWindows_projectFile=teaForGodWin
SET CBA_ForSteam=yes
SET CBA_ForSteam_projectFile=teaForGodWin

PUSHD ..\._buildTools
call tool_create_build_all
POPD
@ECHO ON


