:InitialInfo
@ECHO OFF

:GetParameters
IF "%1" EQU "" GOTO MissingParam
SET CTO_DestDir=%1
GOTO PostProvidedParams

:MissingParam
ECHO ....................................................................................................
ECHO .                                                                                                  .
ECHO .                                                                                                  .
ECHO . Usage                                                                                            .
ECHO . teaForGodAndroid_copy_to_other [DestDir]                                                         .
ECHO .                                                                                                  .
ECHO ....................................................................................................
ECHO .                                                                                                  .
ECHO . This is used to copy the common android files to destination dir                                 .
ECHO .                                                                                                  .
ECHO ....................................................................................................
ECHO .                                                                                                  .
ECHO . Missing parameter                                                                                .
ECHO .                                                                                                  .
ECHO ....................................................................................................
GOTO End

:PostProvidedParams

PUSHD %1
MKDIR res
POPD

XCOPY teaForGodAndroid\res\*.* %1\res\*.* /E /Y >NUL
XCOPY teaForGodAndroid\build.* %1\build.* /E /Y >NUL
XCOPY teaForGodAndroid\project.* %1\project.* /E /Y >NUL

:End
ECHO Copied

:CleanUp
@ECHO ON
