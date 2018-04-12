setlocal
call "C:\Program Files\Microsoft Visual Studio 9.0\VC\vcvarsall.bat" x86
set PATH=..\nant\bin;%PATH%
REM msbuild /t:Build /p:Configuration=Release NetAsm.sln
nant
pause