@echo off
title K-54 Windows bauen - Keller54
rem Startet sich selbst als Administrator neu (fuer die Installation der Build-Werkzeuge)
net session >nul 2>&1
if %errorlevel% neq 0 (
  echo Bitte die Administrator-Abfrage mit "Ja" bestaetigen ...
  powershell -NoProfile -Command "Start-Process -FilePath '%~f0' -Verb RunAs"
  exit /b
)
cd /d "%~dp0"
powershell -NoProfile -ExecutionPolicy Bypass -File "%~dp0build-windows.ps1"
echo.
pause
