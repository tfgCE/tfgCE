:InitialInfo
@ECHO
ECHO Updating build's additional build settings
PUSHD ..
buildVerUpdate --buildPreview --additionalBuildSetting none "code\core\buildVer.h"
POPD
@ECHO ON


