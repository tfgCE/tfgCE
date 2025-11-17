:InitialInfo
@ECHO
ECHO Updating build's active project
PUSHD ..
buildVerUpdate --buildPreview --androidPackageName=com.voidroom.TeaForGodEmperor --setActiveProject tea "code\core\buildVer.h" "code\teaForGodQuest\project.properties" "code\teaForGodQuest\AndroidManifest.xml"
POPD
@ECHO ON


