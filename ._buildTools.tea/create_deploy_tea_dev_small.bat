@ECHO OFF
REM with dev configs and without some data
PUSHD ..\._buildTools
call tool_create_deploy tea devSmall
POPD
@ECHO ON


