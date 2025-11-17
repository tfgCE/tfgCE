@ECHO OFF
PUSHD ..\._buildTools
call tool_runtime_to_build tea Release TeaForGodWin.exe tea.exe
POPD
@ECHO ON
