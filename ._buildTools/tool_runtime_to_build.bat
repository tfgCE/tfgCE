:InitialInfo
@ECHO OFF
ECHO ....................................................................................................
ECHO .                                                                                                  .
ECHO .                                                                                                  .
ECHO . Usage                                                                                            .
ECHO . runtime_to_build [GameName] [Build] [SourceExe] [DestExe]                                        .
ECHO .                                                                                                  .
ECHO ....................................................................................................
ECHO .                                                                                                  .
ECHO . This is used to copy the latest compilation result to the directory used for deploying data      .
ECHO . We assume it is Windows                                                                          .
ECHO .                                                                                                  .
ECHO ....................................................................................................
ECHO.

SET startTime=%time%
SET errorCount=0

:GetParameters
IF "%1" EQU "" GOTO AssumeGameName
SET RTB_GameName=%1
IF "%2" EQU "" GOTO AssumeBuild
SET RTB_Build=%2
IF "%3" EQU "" GOTO AssumeSourceExe
SET RTB_SourceExe=%3
IF "%4" EQU "" GOTO AssumeDestExe
SET RTB_DestExe=%4
GOTO PostProvidedParams

REM using RTB_ prefix as otherwise msbuild uses those settings and everything lands in one place
:AssumeGameName
SET RTB_GameName=xxx
:AssumeBuild
SET RTB_Build=release
:AssumeSourceExe
SET RTB_SourceExe=%RTB_GameName%.exe
:AssumeDestExe
SET RTB_DestExe=%RTB_GameName%.exe
GOTO PostProvidedParams

:PostProvidedParams
SET RTB_BuildDir=build\%RTB_GameName%\windows\
SET RTB_RuntimeDir=runtime\x64\%RTB_Build%\
pushd ..
mkdir build
pushd build
mkdir %RTB_GameName%
pushd %RTB_GameName%
mkdir windows
popd
popd
popd

:ClearDestination
ERASE "..\%RTB_BuildDir%\%RTB_DestExe%" /Q /S >NUL

:CopyExecFiles
Copy "..\%RTB_RuntimeDir%\%RTB_SourceExe%" "..\%RTB_BuildDir%\%RTB_DestExe%" >NUL

:Info
ECHO Copied "..\%RTB_RuntimeDir%\%RTB_SourceExe%" to "..\%RTB_BuildDir%\%RTB_DestExe%"

:CleanUp
@ECHO ON


