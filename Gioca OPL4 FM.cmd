@echo off
powershell -NoProfile -ExecutionPolicy Bypass -File "%~dp0scripts\run.ps1" -Audio OPL4
if errorlevel 1 pause
