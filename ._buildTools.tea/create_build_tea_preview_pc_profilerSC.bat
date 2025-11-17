@ECHO OFF

SET CBA_GameName=tea
SET CBA_GameProjectFolder=teaForGod

SET CB_AndroidPackageName=com.voidroom.TeaForGodEmperor
SET CB_DefaultAndroidPackageName=com.voidroom.TeaForGodEmperor

REM do not build for windows
REM SET CBA_ForWindows=yes
SET CBA_ForWindows_projectFile=teaForGodWin
SET CBA_WindowsProfilerWithSourceAndConfigFiles=yes

PUSHD ..\._buildTools
call tool_create_build_all
POPD
@ECHO ON


