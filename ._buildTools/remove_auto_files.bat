REM %1 where

:Start

:GetParameters
IF %1 EQU "" GOTO Failed
GOTO RemoveAutoFiles

:RemoveAutoFiles
REM start here

:RemoveAuto
CALL remove_subdir.bat %1 _auto

:RemovePlayer
CALL remove_subdir.bat %1 _players

:RemoveLogs
CALL remove_subdir.bat %1 _logs

:RemoveScreenshots
CALL remove_subdir.bat %1 _screenshots

:RemoveSource
IF "%CB_KeepSourceFiles%" NEQ "yes" (
	CALL remove_subdir.bat %1 _source
)

:RemoveDocs
CALL remove_subdir.bat %1 _docs

:RemoveTest
CALL remove_subdir.bat %1 _test

:RemovePreviewMockups
CALL remove_subdir.bat %1 _previewMockups

:RemoveStats
CALL remove_subdir.bat %1 _stats

:RemoveSourceXML
rem this just assumes what should be removed/kept by subdirectories
IF "%CB_KeepSourceFiles%" NEQ "yes" (
	CALL remove_file.bat %1\library *.xml
	CALL remove_file.bat %1\settings *.xml
	CALL remove_file.bat %1\system *.xml
)

:RemoveInnerTXT
rem this just assumes what should be removed/kept by subdirectories
IF "%CB_KeepSourceFiles%" NEQ "yes" (
	CALL remove_file.bat %1\library *.txt
	CALL remove_file.bat %1\settings *.txt
	CALL remove_file.bat %1\system *.txt
)

CALL remove_file.bat %1 _info.txt

:RemoveOtherXMLFiles
CALL remove_file.bat %1 _config.xml
CALL remove_file.bat %1 _loadSetup.xml
CALL remove_file.bat %1 _tools.xml
CALL remove_file.bat %1 _tweaks.xml
IF "%CB_KeepDevConfigFiles%" NEQ "yes" (
	CALL remove_file.bat %1 _preview.xml
	CALL remove_file.bat %1 _editor.xml
	CALL remove_file.bat %1 _baseConfig.xml
	CALL remove_file.bat %1 _baseInputConfig.xml
	CALL remove_file.bat %1 _devConfig*.xml
)
CALL remove_file.bat %1 _bullshots.xml
CALL remove_file.bat %1 userConfig.xml
CALL remove_file.bat %1 system.tags

:RemoveLogs
CALL remove_file.bat %1 fmod.log
CALL remove_file.bat %1 output.log

:RemoveOther
CALL remove_file.bat %1 *.~*~
CALL remove_file.bat %1 tools.txt
CALL remove_file.bat %1 steam_appid.txt
CALL remove_file.bat %1 *.orig

:CleanUp
GOTO End

:Failed
@ECHO Missing parameters %1 inDir %2 fileToRemove
GOTO End

:End
