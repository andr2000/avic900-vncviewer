for /f "delims=" %%i in ('"c:\program files\git\bin\git.exe" rev-parse --short HEAD') do @set REV="%%i"
echo #define COMMIT_ID %REV%> version.inc
