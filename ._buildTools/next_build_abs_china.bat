:InitialInfo
@ECHO
ECHO Updating build's additional build settings
PUSHD ..
buildVerUpdate --buildPreview --additionalBuildSetting AN_CHINA "code\core\buildVer.h"
POPD
@ECHO ON


