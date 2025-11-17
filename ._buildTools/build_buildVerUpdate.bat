:Start

:CheckBuildTool
IF EXIST ..\buildVerUpdate.exe GOTO CheckSDL

:BuildTool
ECHO Rebuilding buildVerUpdate.exe
SET PATH=%ProgramFiles(x86)%\Microsoft Visual Studio\2019\Community\MSBuild\Current\Bin;%PATH%
msbuild.exe ..\aironoevl.sln /m /p:Configuration=Release,Platform=x64 /target:projects\buildVerUpdate:Rebuild
copy ..\runtime\x64\Release\buildVerUpdate.exe ..\buildVerUpdate.exe > NUL

:CheckSDL
IF EXIST ..\SDL2.dll GOTO End
REM SDL2.dll required for some reason...
copy ..\externals\runtime\x64\SDL2.dll ..\SDL2.dll > NUL

:End
