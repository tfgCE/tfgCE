@ECHO OFF
SET DT_Demo=yes
PUSHD ..\._buildTools
call tool_create_deploy tea yes
POPD
@ECHO ON


