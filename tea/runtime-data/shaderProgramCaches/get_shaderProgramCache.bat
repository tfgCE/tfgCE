:InitialInfo
@ECHO OFF
ECHO ....................................................................................................
ECHO .                                                                                                  .
ECHO . Usage                                                                                            .
ECHO . get_shaderProgramCache [system-precise-name]                                                     .
ECHO .                                                                                                  .
ECHO ....................................................................................................
ECHO .                                                                                                  .
ECHO . [system-precise-name] case sensitive, should be the name under which the shaderProgramCache      .
ECHO .      is stored                                                                                   .
ECHO .           quest     for quest                                                                    .
ECHO .           quest2    for quest2                                                                   .
ECHO .                                                                                                  .
ECHO . All parameters are required!                                                                     .
ECHO .                                                                                                  .
ECHO ....................................................................................................
ECHO.

:GetSystemPreciseName
IF "%1" EQU "" GOTO NoFileName
SET Upload_File=%1

:Download
ECHO Downloading "%1" shader program cache [shaderProgramCache.%1]
adb pull -p /sdcard/Android/data/com.voidroom.TeaForGod/files/_auto/shaderProgramCache.%1%
GOTO End

:NoFileName
ECHO #######################################
ECHO #                                     #
ECHO #   No system precise name provided   #
ECHO #                                     #
ECHO #######################################
GOTO End

:End
REM PAUSE
