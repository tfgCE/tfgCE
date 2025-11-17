:Start

:GetParameters
IF "%1" EQU "" GOTO AssumePlatform
Set ForPlatform=%1
IF "%2" EQU "" GOTO AssumeSolutionDir
Set ForSolution=%2
IF "%3" EQU "" GOTO AssumeProject
Set ForProject=%3
IF "%4" EQU "" GOTO AssumeConfiguration
Set ForConfiguration=%4
GOTO CopyDLLs

:AssumePlatform
SET ForPlatform=x64

:AssumeSolutionDir
SET ForSolution=

:AssumeProject
SET ForProject=

:CopyDLLs
ECHO Copying DLLs...
ECHO platform=%ForPlatform%
ECHO solution=%ForSolution%
ECHO project=%ForProject%
ECHO configuration=%ForConfiguration%
XCOPY "%ForSolution%externals\runtime\%ForPlatform%\*.dll" "%ForSolution%runtime\%ForPlatform%\%ForConfiguration%\*.dll" /D /Y
XCOPY "%ForSolution%externals\runtime\%ForPlatform%\%ForProject%\*.dll" "%ForSolution%runtime\%ForPlatform%\%ForConfiguration%\*.dll" /D /Y
XCOPY "%ForSolution%externals\runtime\%ForPlatform%\%ForConfiguration%\*.dll" "%ForSolution%runtime\%ForPlatform%\%ForConfiguration%\*.dll" /D /Y
XCOPY "%ForSolution%externals\runtime\%ForPlatform%\%ForProject%\%ForConfiguration%\*.dll" "%ForSolution%runtime\%ForPlatform%\%ForConfiguration%\*.dll" /D /Y

:CleanUp
Set ForSolution=
Set ForProject=
