@ECHO OFF
ECHO .
ECHO . Use _output\OutputMonitor.exe to check contents of output.log
ECHO .
MKDIR ..\._output
PUSHD ..\._buildTools
CALL tool_prepare_subfilename.bat
POPD
create_build_tea_full_pc_pico.bat >..\._output\output.log
COPY "..\._output\output.log" "..\._output\output_%SUBFILENAME%.log" >NUL
