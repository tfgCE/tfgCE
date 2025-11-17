REM .
REM . Internal tool for creating distribute pack for ARM64, just one package
REM . requires to be set:
REM .    DP_SrcDir
REM .    DP_Suffix
REM .

COPY "..\%DP_MainBuildDir%\%DP_SrcDir%\%DP_GameName%.apk" "..\%DP_MainBuildDir%\distribute\%DP_GameName%%DP_Suffix%.apk" >NUL
COPY "..\%DP_MainBuildDir%\%DP_SrcDir%\%DP_GameName%.so" "..\%DP_MainBuildDir%\distribute\%DP_GameName%%DP_Suffix%.so" >NUL

REM create copy with name/build number
PUSHD ..
buildVerUpdate --setActiveProject %DP_GameName% --copyFileWithBuildName "%DP_MainBuildDir%\distribute\%DP_GameName%%DP_Suffix%.apk" "code\core\buildVer.h"
buildVerUpdate --setActiveProject %DP_GameName% --copyFileWithBuildNo "%DP_MainBuildDir%\distribute\%DP_GameName%%DP_Suffix%.so" "code\core\buildVer.h"
POPD
