# 🖥️ UI Startup Guide - Automatisches Backend Management

## Übersicht

Die **MachinaMind C++ Qt GUI** startet das Python Backend automatisch beim Start. Du musst das Backend **nicht manuell starten**.

---

## Wie funktioniert der automatische Start?

### 1. UI Start

Wenn du die GUI startest:

```powershell
cd cpp_gui\build\Release
.\MachinaMindAIAgent.exe
```

### 2. Automatischer Backend-Start

Die GUI führt **automatisch** folgende Schritte aus:

```cpp
// Intern in MainWindow::startBackend()

1. Suche Python in: backend/venv/Scripts/python.exe
2. Setze Working Directory: backend/
3. Starte: python api/main.py
4. Warte 5 Sekunden (Backend-Initialisierung)
5. Zeige Status: "MachinaMind startet... Bitte warten..."
```

### 3. Backend-Initialisierung

Das Backend läuft nun und:
- ✅ Lädt Datenbank (`MachinaData.db`)
- ✅ Initialisiert RAG Manager (lädt Vector Store)
- ✅ Startet FastAPI Server auf Port 8000
- ✅ Ist bereit für UI-Anfragen

### 4. UI verbindet automatisch

Nach 5 Sekunden:
- ✅ UI sendet `/health` Request
- ✅ Status ändert sich zu "🟢 Verbunden"
- ✅ Lädt Maschinenliste
- ✅ Ready to use!

---

## Voraussetzungen

### 1. Python Virtual Environment muss existieren

**Einmalige Einrichtung:**

```powershell
cd backend
python -m venv venv
.\venv\Scripts\Activate.ps1
pip install -r requirements.txt
```

**Check:**
```powershell
# Sollte existieren:
dir backend\venv\Scripts\python.exe
```

### 2. Datenbank muss initialisiert sein

**Einmalige Einrichtung:**

```powershell
cd backend
.\venv\Scripts\Activate.ps1
python data_simulator.py --once
```

Dies erstellt:
- ✅ `MachinaData.db` (SQLite Datenbank)
- ✅ `vector_store/` (RAG Index)
- ✅ Demo-Daten (3 Maschinen, Messungen, Events)

**Check:**
```powershell
# Sollten existieren:
dir backend\MachinaData.db
dir backend\vector_store\faiss.index
```

### 3. PDF-Dokumente für RAG (optional)

Wenn du RAG nutzen willst:

```powershell
# Lege PDFs in backend/ ab
copy Wartungsprotokoll.pdf backend\
```

Beim nächsten Simulator-Start werden PDFs automatisch indexiert.

---

## Verzeichnis-Struktur

**Wichtig:** Die GUI erwartet diese Struktur:

```
MachinaMind_AI_Agent/
├── backend/
│   ├── venv/
│   │   └── Scripts/
│   │       └── python.exe       ← Muss existieren!
│   ├── api/
│   │   └── main.py              ← Backend Entry Point
│   ├── MachinaData.db           ← Datenbank
│   ├── vector_store/            ← RAG Index
│   │   └── faiss.index
│   └── *.pdf                    ← RAG Dokumente
└── cpp_gui/
    └── build/
        └── Release/
            └── MachinaMindAIAgent.exe  ← UI Executable
```

Die GUI berechnet den Backend-Pfad relativ:

```cpp
QString backendPath = QCoreApplication::applicationDirPath() + "/../../backend";
// Wenn UI in:  cpp_gui/build/Release/
// Backend ist: ../../backend  (relativ)
```

---

## Automatische Initialisierung

### Workflow beim ersten Start

**1. Erstmalige Einrichtung (einmalig):**

```powershell
# 1. Backend einrichten
cd backend
python -m venv venv
.\venv\Scripts\Activate.ps1
pip install -r requirements.txt

# 2. Daten + RAG vorbereiten
python data_simulator.py --once

# 3. Frontend bauen
cd ..\cpp_gui
mkdir build
cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build . --config Release
```

**2. UI starten:**

```powershell
.\Release\MachinaMindAIAgent.exe
```

**3. Was passiert automatisch:**

```
[00:00] UI startet
[00:01] Backend-Prozess wird gestartet (python api/main.py)
[00:02] Backend lädt Datenbank (MachinaData.db)
[00:03] Backend lädt RAG Index (vector_store/)
[00:04] Backend startet FastAPI Server (Port 8000)
[00:05] UI sendet /health Request
[00:06] ✅ Verbindung hergestellt
[00:07] UI lädt Maschinenliste
[00:08] 🟢 Ready!
```

---

## Backend Status überprüfen

### In der UI

**Status-Anzeige:**
- 🟢 **Verbunden** - Backend läuft, alles OK
- 🟡 **Startet...** - Backend initialisiert sich
- 🔴 **Getrennt** - Backend nicht erreichbar

### Manuell testen

**PowerShell:**

```powershell
# Health Check
curl http://localhost:8000/health

# Sollte antworten:
# {
#   "status": "healthy",
#   "machines": 3,
#   "measurements": 209,
#   "events": 109
# }
```

**Browser:**
- API Docs: http://localhost:8000/docs
- Health: http://localhost:8000/health

---

## Troubleshooting

### Problem 1: "Backend konnte nicht gestartet werden"

**Symptom:** UI zeigt Fehlermeldung beim Start.

**Diagnose:**

```powershell
# Check 1: Python existiert?
dir backend\venv\Scripts\python.exe

# Check 2: Dependencies installiert?
backend\venv\Scripts\python.exe -c "import fastapi; print('OK')"

# Check 3: Backend manuell starten?
cd backend
.\venv\Scripts\Activate.ps1
python api/main.py
```

**Lösung:** Virtual Environment neu erstellen (siehe Voraussetzungen).

---

### Problem 2: "Port 8000 bereits belegt"

**Symptom:** Backend startet nicht, Port-Fehler.

**Diagnose:**

```powershell
# Check welcher Prozess Port 8000 nutzt
netstat -ano | findstr :8000
```

**Lösung Option A - Prozess beenden:**

```powershell
# PID aus netstat notieren, dann:
taskkill /PID <PID> /F
```

**Lösung Option B - Anderen Port nutzen:**

```python
# backend/api/main.py ändern:
if __name__ == "__main__":
    uvicorn.run("main:app", host="0.0.0.0", port=8001)  # Port 8001
```

Dann in UI:
```cpp
// Server-Adresse ändern:
m_serverInput->setText("http://localhost:8001");
```

---

### Problem 3: "Datenbank leer / keine Maschinen"

**Symptom:** UI zeigt keine Maschinen in der Liste.

**Diagnose:**

```powershell
cd backend
.\venv\Scripts\Activate.ps1
python -c "import sqlite3; db = sqlite3.connect('MachinaData.db'); print('Maschinen:', db.execute('SELECT COUNT(*) FROM machines').fetchone()[0])"
```

**Lösung:** Daten generieren:

```powershell
python data_simulator.py --once
```

Dies erstellt 3 Maschinen + Messungen + Events.

---

### Problem 4: "RAG funktioniert nicht"

**Symptom:** Chat-Antworten ohne Quellenangaben.

**Diagnose:**

```powershell
# Check 1: Vector Store existiert?
dir backend\vector_store\faiss.index

# Check 2: Dokumente indexiert?
python -c "from rag_engine.rag_manager import RAGManager; rag = RAGManager(); print('Dokumente:', len(rag.documents))"
```

**Lösung:** RAG indexieren:

```powershell
cd backend
# Option 1: Via Simulator
python data_simulator.py --once --reindex

# Option 2: Manuell
python index_pdf.py
```

---

### Problem 5: Backend-Prozess bleibt nach UI-Schließen

**Symptom:** Backend läuft weiter, auch wenn UI geschlossen.

**Check:**

```powershell
# Python-Prozesse anzeigen
Get-Process python
```

**Lösung (manuell):**

```powershell
# Alle Python-Prozesse beenden
taskkill /IM python.exe /F
```

**Lösung (automatisch in Code):**

Die UI sollte dies bereits tun in `MainWindow::stopBackend()`:

```cpp
void MainWindow::stopBackend() {
    if (m_backendProcess) {
        m_backendProcess->terminate();
        if (!m_backendProcess->waitForFinished(3000)) {
            m_backendProcess->kill(); // Force kill nach 3s
        }
        delete m_backendProcess;
        m_backendProcess = nullptr;
    }
}
```

---

## Best Practices

### 1. Entwicklungs-Workflow

**Terminal 1 (Backend manuell, für Debugging):**
```powershell
cd backend
.\venv\Scripts\Activate.ps1
python api/main.py
# Backend läuft, Logs sichtbar
```

**Terminal 2 (UI ohne Auto-Start):**
```powershell
cd cpp_gui\build\Release
.\MachinaMindAIAgent.exe
# UI verbindet zu bestehendem Backend
```

**Vorteil:** Du siehst Backend-Logs in Echtzeit.

### 2. Produktions-Workflow

**Einfach UI starten:**
```powershell
.\MachinaMindAIAgent.exe
# Backend startet automatisch im Hintergrund
```

**Vorteil:** Ein Klick, alles läuft.

### 3. Docker Deployment

**Für Production:**

```yaml
# docker-compose.yml
version: '3.8'

services:
  backend:
    build: ./backend
    ports:
      - "8000:8000"
    volumes:
      - ./backend/MachinaData.db:/app/MachinaData.db
      - ./backend/vector_store:/app/vector_store
    restart: always

  # Optional: Frontend als Windows Service
  # (Qt App kann nicht direkt in Docker, nur Backend)
```

---

## Erweiterte Konfiguration

### Backend-Parameter anpassen

**Environment Variables:**

Die UI setzt automatisch:

```cpp
env.insert("MACHINAMIND_RELOAD", "0");  // Auto-Reload deaktiviert
```

**Weitere Environment Variables (optional):**

```powershell
# In System-Umgebung setzen:
$env:OPENAI_API_KEY = "sk-..."
$env:DATABASE_PATH = "custom.db"
$env:RAG_STORE_PATH = "custom_vector_store"
```

### Backend-Start anpassen

**In C++ Code (MainWindow.cpp):**

```cpp
// Zeile 414-416: Python-Pfad
QString program = backendPath + "/venv/Scripts/python.exe";
QStringList arguments;
arguments << "api/main.py";

// Zusätzliche Parameter hinzufügen:
arguments << "--host" << "0.0.0.0";
arguments << "--port" << "8000";
```

**In Python Code (api/main.py):**

```python
if __name__ == "__main__":
    import sys
    port = int(sys.argv[2]) if len(sys.argv) > 2 else 8000
    
    uvicorn.run(
        "main:app",
        host="0.0.0.0",
        port=port,
        reload=False  # Wichtig für UI-Start!
    )
```

---

## Monitoring & Logging

### Backend-Logs in UI anzeigen

**Aktuell:** Logs gehen nach `qDebug()`.

**In QtCreator sichtbar:**
```
Backend Output: INFO:     Started server process [12345]
Backend Output: INFO:     Waiting for application startup.
Backend Output: INFO:     Application startup complete.
```

**Logs in Datei schreiben:**

```cpp
// MainWindow.cpp - in startBackend()
connect(m_backendProcess, &QProcess::readyReadStandardOutput, this, [this]() {
    QString output = m_backendProcess->readAllStandardOutput();
    
    // In Datei schreiben
    QFile logFile("backend_log.txt");
    if (logFile.open(QIODevice::Append)) {
        logFile.write(output.toUtf8());
        logFile.close();
    }
    
    qDebug() << "Backend Output:" << output;
});
```

### Health Monitoring

**Periodischer Health Check:**

```cpp
// MainWindow.cpp - in Constructor
QTimer* healthCheckTimer = new QTimer(this);
connect(healthCheckTimer, &QTimer::timeout, this, [this]() {
    // Alle 30 Sekunden Health Check
    m_presenter->checkHealth();
});
healthCheckTimer->start(30000);  // 30s
```

---

## Zusammenfassung

### Was die UI automatisch macht:

✅ **Backend starten** (`python api/main.py`)  
✅ **Datenbank laden** (`MachinaData.db`)  
✅ **RAG initialisieren** (`vector_store/`)  
✅ **Server starten** (Port 8000)  
✅ **Verbindung herstellen** (nach 5s)  
✅ **Backend beenden** (beim UI-Close)  

### Was du manuell machen musst (einmalig):

1️⃣ **Virtual Environment erstellen** (`python -m venv venv`)  
2️⃣ **Dependencies installieren** (`pip install -r requirements.txt`)  
3️⃣ **Daten generieren** (`python data_simulator.py --once`)  

### Danach:

🚀 **Einfach UI starten** - alles läuft automatisch!

```powershell
.\MachinaMindAIAgent.exe
```

---

## Nächste Schritte

- [Quickstart Guide](QUICKSTART.md) - Komplette Einrichtung
- [RAG Guide](RAG_GUIDE.md) - RAG-System im Detail
- [Architecture](architecture.md) - System-Architektur
