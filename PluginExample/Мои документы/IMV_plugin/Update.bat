set "PATH=c:\Windows\Microsoft.NET\Framework\v4.0.30319\"
del "/Q %~dp0Generator.exe"
csc.exe /target:exe /out:"%~dp0Generator.exe" Generator\Main.cs
Generator.exe
del /Q "%~dp0Generator.exe"
rmdir /Q /S "%~dp0Generator\.vs"
rmdir /Q /S "%~dp0Generator\bin"
rmdir /Q /S "%~dp0Generator\obj"
pause
