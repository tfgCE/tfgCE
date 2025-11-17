@ECHO OFF
REM copy, don't prepare
PUSHD ..\._buildTools
call tool_create_deploy tea no
POPD
@ECHO ON


