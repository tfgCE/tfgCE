:InitialInfo
@ECHO
ECHO Updating build's active project
PUSHD ..
buildVerUpdate --buildPreview --setActiveProject tea "code\core\buildVer.h"
POPD
@ECHO ON


