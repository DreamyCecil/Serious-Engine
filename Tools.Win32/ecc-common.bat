REM This script serves as a common way of compiling any entity source file without the need
REM to edit every single entry in Visual Studio project files in order to change ECC behavior

REM Working directory argument missing
if [%1]==[] goto usage

REM Entity source file argument missing
if [%2]==[] goto usage

pushd "%~dp1"
ecc "%~2" -cfg "ecc-common.inl"
popd
goto :end

:usage
@echo Expected two arguments: ecc-common.bat ^<working dir^> ^<ES file^>
exit /B 1

:end
exit /B 0
