@echo off
REM MachinaMindAIAgent Production Startup Script (Windows)

echo ========================================
echo MachinaMindAIAgent - Production Mode
echo ========================================

REM Check if virtual environment exists
if not exist "backend\venv" (
    echo ERROR: Virtual environment not found!
    echo Please run: cd backend ^&^& python -m venv venv ^&^& .\venv\Scripts\activate ^&^& pip install -r requirements.txt
    pause
    exit /b 1
)

REM Check if .env exists
if not exist "backend\.env" (
    echo WARNING: .env file not found! Creating from template...
    copy backend\.env.example backend\.env
    echo.
    echo Please edit backend\.env and configure your settings
    echo Then run this script again.
    pause
    exit /b 1
)

REM Start backend in production mode (without demo simulator)
echo.
echo [1/2] Starting Backend (Production Mode)...
start "MachinaMindAIAgent Backend" cmd /k "cd backend && venv\Scripts\activate && echo Backend starting... && python api\main.py"

echo.
echo [2/2] Waiting for Backend to be ready (RAG model loading, ~100s)...
echo.

REM Health Check Loop - wartet bis Backend wirklich bereit ist
set ATTEMPT=0
set MAX_ATTEMPTS=60

:HEALTH_CHECK
set /a ATTEMPT+=1

REM Timeout nach 120 Sekunden
if %ATTEMPT% gtr %MAX_ATTEMPTS% (
    echo.
    echo ERROR: Backend did not respond within 120 seconds!
    echo Check the Backend window for errors.
    pause
    exit /b 1
)

REM Fortschritt anzeigen (alle 10 Sekunden)
set /a MOD=%ATTEMPT% %% 5
if %MOD%==0 (
    set /a ELAPSED=%ATTEMPT% * 2
    echo   Waiting for %ELAPSED%s / 120s...
)

REM Prüfe /health Endpoint
curl -s http://localhost:8000/health >nul 2>&1
if errorlevel 1 (
    timeout /t 2 /nobreak >nul
    goto HEALTH_CHECK
)

echo.
echo ✓ Backend is ready after %ATTEMPT% attempts (%ATTEMPT%*2 seconds)!
echo.

REM Check if frontend executable exists
if not exist "build\cpp_gui\MachinaMindAIAgent.exe" (
    echo.
    echo ERROR: Frontend not built yet!
    echo Building frontend...
    set "PATH=C:\Qt\Tools\mingw1310_64\bin;C:\Qt\Tools\Ninja;%PATH%"
    mkdir build 2>nul
    cmake -S . -B build -G Ninja -DCMAKE_PREFIX_PATH=C:/Qt/6.9.3/mingw_64 -DCMAKE_CXX_COMPILER=C:/Qt/Tools/mingw1310_64/bin/g++.exe -DCMAKE_BUILD_TYPE=Release
    cmake --build build --config Release
)

REM Start frontend
echo.
echo Starting Frontend...
start "MachinaMindAIAgent Frontend" build\cpp_gui\MachinaMindAIAgent.exe

echo.
echo ========================================
echo ✓ System started successfully!
echo ========================================
echo.
echo Backend:  http://localhost:8000
echo API Docs: http://localhost:8000/docs
echo Frontend: Qt Application Window
echo.
echo Configuration:
echo - LLM Provider: Check backend\.env
echo - Database: backend\MachinaData.db
echo.
echo Press Ctrl+C in Backend window to stop
echo ========================================

pause
