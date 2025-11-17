@ECHO OFF
PUSHD ..\._buildTools
call tool_runtime_to_build tea profiler TeaForGodWin.exe tea.exe
POPD
@ECHO ON
