REM this batch file allows to sign development builds with release key to update over offical builds

REM here is a HARDCODED_PATH used to get access to apksigner
SET PATH=%ProgramFiles(x86)%\Microsoft Visual Studio\2019\Community\MSBuild\Current\Bin;C:\Microsoft\AndroidSDK\25\build-tools\25.0.3;%PATH%

PUSHD ..
CALL apksigner.bat sign --verbose -ks code\tea_for_god_open.keystore --ks-pass pass:passwordgoeshere "runtime\ARM64\ReleasePico\teaForGodPico.apk"
POPD


