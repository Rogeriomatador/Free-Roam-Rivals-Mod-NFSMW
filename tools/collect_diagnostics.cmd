@echo off
where py >nul 2>nul
if not errorlevel 1 (
  py -3 "%~dp0collect_diagnostic_bundle.py" "%~dp0.."
  pause
  exit /b
)
where python >nul 2>nul
if not errorlevel 1 (
  python "%~dp0collect_diagnostic_bundle.py" "%~dp0.."
  pause
  exit /b
)
echo Python nao encontrado. Envie scripts\FreeRoamRivals\FreeRoamRivals.log depois da captura F9.
pause
