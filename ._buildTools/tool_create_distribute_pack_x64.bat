REM .
REM . Internal tool for creating distribute pack for x64, just one package
REM . requires to be set:
REM .    DP_SrcDir
REM .    DP_Suffix
REM .

IF NOT EXIST "..\%DP_MainBuildDir%\distribute\pack" MKDIR "..\%DP_MainBuildDir%\distribute\pack" >NUL
XCOPY "..\%DP_MainBuildDir%\%DP_SrcDir%\*.*" "..\%DP_MainBuildDir%\distribute\pack\*.*" /E /-Y >NUL
COPY "..\%DP_MainBuildDir%\distribute\pack\pdb\%DP_GameName%.pdb" "..\%DP_MainBuildDir%\distribute\%DP_GameName%%DP_Suffix%.pdb" >NUL
ERASE "..\%DP_MainBuildDir%\distribute\pack\pdb" /Q /S >NUL
CALL remove_subdir.bat ..\%DP_MainBuildDir%\distribute\pack pdb
COPY "..\%DP_MainBuildDir%\distribute\pack\system\codeInfo" "..\%DP_MainBuildDir%\distribute\%DP_GameName%%DP_Suffix%.codeInfo" >NUL
PUSHD "..\%DP_MainBuildDir%\distribute\pack\"
jar -cMf "%DP_GameName%%DP_Suffix%.zip" .
POPD
COPY "..\%DP_MainBuildDir%\distribute\pack\%DP_GameName%%DP_Suffix%.zip" "..\%DP_MainBuildDir%\distribute\%DP_GameName%%DP_Suffix%.zip" >NUL
ERASE "..\%DP_MainBuildDir%\distribute\pack" /Q /S >NUL
CALL clean_dir.bat "..\%DP_MainBuildDir%\distribute\pack"
IF EXIST "..\%DP_MainBuildDir%\distribute\pack" RMDIR "..\%DP_MainBuildDir%\distribute\pack" >NUL

REM create copy with name/build number
PUSHD ..
buildVerUpdate --setActiveProject %DP_GameName% --copyFileWithBuildName "%DP_MainBuildDir%\distribute\%DP_GameName%%DP_Suffix%.zip" "code\core\buildVer.h"
buildVerUpdate --setActiveProject %DP_GameName% --copyFileWithBuildNo "%DP_MainBuildDir%\distribute\%DP_GameName%%DP_Suffix%.pdb" "code\core\buildVer.h"
POPD
