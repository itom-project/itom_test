TITLE compile debug x64 - %~dp0
CALL "%VS140COMNTOOLS%\vsvars32.bat"

msbuild.exe "%~dp0\ALL_BUILD.vcxproj" /p:configuration=debug /p:platform=x64

pause

REM for rebuild add "/t:rebuild" after /p:platform=win32