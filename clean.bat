@echo off
set OUT_DIR=build

if exist %OUT_DIR% (
    echo Deleting %OUT_DIR% folder...
    rmdir /s /q %OUT_DIR%
    echo Clean complete.
) else (
    echo Nothing to clean.
)
pause