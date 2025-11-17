:InitialInfo
@ECHO
ECHO Updating build ver for next build (pre)
PUSHD ..
buildVerUpdate --buildPreview --updateMinor "code\core\buildVer.h"
POPD
@ECHO ON


