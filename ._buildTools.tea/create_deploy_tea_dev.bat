@ECHO OFF
REM with dev configs
PUSHD ..\._buildTools
call tool_create_deploy tea dev
POPD
@ECHO ON


