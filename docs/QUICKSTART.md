# 🚀 MachinaMindAIAgent Quick Start Guide

## Schnellstart (5 Minuten)

### ⚡ Das Wichtigste vorweg

Die **UI startet das Backend automatisch!** Du musst nur:
1. Python Environment einrichten (einmalig)
2. Daten generieren (einmalig)
3. UI starten → **Backend startet automatisch** 🎉

---

### Option 1: Demo-Modus (Empfohlen)

**Windows:**
```powershell
# Einfach Demo starten
.\demo.bat
```

Das Script:
1. Erstellt Python Virtual Environment
2. Installiert Dependencies
3. Startet Datensimulator + Backend
4. Baut und startet Frontend

---

### Option 2: Manuelle Einrichtung (Mehr Kontrolle)

**Manuelle Schritte:**

### Schritt 1: Backend starten

```powershell
# Virtual Environment erstellen
cd backend
python -m venv venv
.\venv\Scripts\Activate.ps1

# Dependencies installieren
pip install -r requirements.txt

# Datenbank initialisieren
python database/db_handler.py --init

# Demo-Daten erstellen UND RAG indexieren (alles in einem!)
python data_simulator.py --once

# Backend starten
python api/main.py
```

**Was passiert beim ersten Start von `data_simulator.py`:**
- ✅ Sucht automatisch nach PDF-Dateien im `backend/` Ordner
- ✅ Indexiert PDFs für RAG (einmalig, ~20 Sekunden)
- ✅ Erstellt Vector Store in `backend/vector_store/`
- ✅ Generiert Demo-Daten (Maschinen, Messungen, Events)
- ✅ Bei weiteren Starts: Lädt bestehenden Vector Store (~100ms)

**Hinweis:** Wenn sich PDF-Inhalte ändern, verwende:
```powershell
python data_simulator.py --once --reindex
```

**Backend Status prüfen (optional):**
```powershell
# Backend manuell starten (für Debugging)
python api/main.py
# Sollte zeigen: "Application startup complete"
# Erreichbar auf: http://localhost:8000
```

### Schritt 2: Frontend bauen

**Neues Terminal öffnen:**

```powershell
cd cpp_gui

# Build-Verzeichnis erstellen
mkdir build
cd build

# CMake konfigurieren
cmake .. -DCMAKE_BUILD_TYPE=Release

# Bauen
cmake --build . --config Release

# Starten
.\Release\MachinaMindAIAgent.exe
```

🎉 **Das Backend startet automatisch!** Die UI:
- ✅ Startet `python api/main.py` im Hintergrund
- ✅ Wartet 5 Sekunden (Initialisierung)
- ✅ Verbindet automatisch
- ✅ Zeigt Status: 🟢 "Verbunden"

**Details:** Siehe [UI Startup Guide](UI_STARTUP_GUIDE.md) für Backend-Management.

---

### Schritt 3: Demo-Workflow

1. **UI starten**: `.\Release\MachinaMindAIAgent.exe`
   - Backend startet automatisch im Hintergrund
   - Status ändert sich zu 🟢 "Verbunden"
   
2. **Maschine auswählen**: Dropdown → z.B. "CNC-Mill-01"

3. **Chat testen**: 
   ```
   "Was ist der Status dieser Maschine?"
   "Wie oft sollte eine Wartung durchgeführt werden?"
   ```
   → Antwort kommt mit **Quellenangaben** aus RAG!

4. **Analyse starten**: Button "🔍 Analysieren" klicken
   → Zeigt Anomalie-Erkennung

5. **Backend manuell prüfen** (optional):
   - Browser: http://localhost:8000/health
   - Sollte zeigen: `{"status": "healthy", "machines": 3}`

---

## Option 2: Docker (Production-Like)

```powershell
# Alle Services starten
cd deploy
docker-compose up -d

# Logs anschauen
docker-compose logs -f backend

# Stoppen
docker-compose down
```

Services:
- Backend: http://localhost:8000
- Ollama (lokales LLM): http://localhost:11434

---

## 🎓 Nächste Schritte

### 1. API testen (curl/Postman)

```powershell
# Health Check
curl http://localhost:8000/health

# Maschinen abrufen
curl http://localhost:8000/machines

# Chat
curl -X POST http://localhost:8000/chat `
  -H "Content-Type: application/json" `
  -d '{"message": "Zeige mir alle Maschinen"}'
```

### 2. Eigene Daten hinzufügen

```python
# Python
from backend.database.db_handler import DatabaseHandler

db = DatabaseHandler("MachinaData.db")

# Maschine hinzufügen
machine_id = db.add_machine("Meine-CNC", "CNC", "Halle 1")

# Messwert hinzufügen
db.add_measurement(machine_id, "temperature", 45.5, "°C")
```

### 3. RAG-Dokumente indexieren

```python
# Python
from backend.rag_engine.rag_manager import RAGManager

rag = RAGManager()

# Dokumente hinzufügen
docs = ["Wartungsanleitung für CNC-Maschinen...", "Sicherheitsprotokoll..."]
rag.add_documents(docs)

# Oder ganzes Verzeichnis
rag.index_directory("docs/manuals", file_extensions=[".pdf", ".txt"])
```

### 4. LLM konfigurieren

Erstelle `.env` im `backend/` Ordner:

```env
# OpenAI
OPENAI_API_KEY=sk-your-key-here
LLM_PROVIDER=openai
LLM_MODEL=gpt-4

# Oder Anthropic
ANTHROPIC_API_KEY=sk-ant-your-key-here
LLM_PROVIDER=anthropic
LLM_MODEL=claude-3-sonnet-20240229

# Oder lokales Ollama
LLM_PROVIDER=ollama
LLM_MODEL=llama2
```

---

## 🔧 Troubleshooting

### Backend startet nicht

```powershell
# Port 8000 bereits belegt?
netstat -ano | findstr :8000

# Process beenden
taskkill /PID <PID> /F

# Dependencies neu installieren
pip install -r requirements.txt --force-reinstall
```

### Frontend baut nicht

```powershell
# Qt nicht gefunden?
# Qt 6.9+ installieren von: https://www.qt.io/download

# CMake-Cache löschen
cd cpp_gui/build
rm CMakeCache.txt
cmake .. -DCMAKE_PREFIX_PATH="C:/Qt/6.9.0/msvc2019_64"
```

### Datenbank-Fehler

```powershell
# Datenbank neu initialisieren
rm MachinaData.db
python backend/database/db_handler.py --init --demo
```

### API Calls schlagen fehl

1. Backend läuft? → http://localhost:8000/health
2. CORS-Fehler? → In `backend/api/main.py` CORS-Origins prüfen
3. Firewall? → Port 8000 freigeben

---

## 📚 Weiterführende Dokumentation

- **Architektur**: [docs/architecture.md](docs/architecture.md)
- **API Docs**: http://localhost:8000/docs (wenn Backend läuft)
- **Contributing**: [CONTRIBUTING.md](CONTRIBUTING.md)

---

## 🎯 Demo-Szenarien

### Szenario 1: Anomalie-Erkennung

1. Simulator laufen lassen: `python data_simulator.py 300` (5 Minuten)
2. In GUI: Maschine auswählen
3. "Analysieren" klicken
4. System erkennt statistische Anomalien
5. Chat-Frage: "Was bedeutet diese Anomalie?"

### Szenario 2: Wartungsplanung

1. Chat: "Welche Maschinen brauchen Wartung?"
2. System analysiert Events und Messwerte
3. Gibt priorisierte Liste zurück
4. Report generieren (speichert in DB)

### Szenario 3: Echtzeit-Monitoring

1. Auto-Refresh aktivieren (alle 10 Sekunden)
2. Charts zeigen Live-Daten
3. Events-Tabelle aktualisiert sich
4. Bei kritischen Events → Notification

---

## ⚡ Performance-Tipps

**Backend:**
- SQLite → PostgreSQL für Produktion
- Redis-Cache für häufige Queries
- Gunicorn statt Uvicorn (Multi-Worker)

**Frontend:**
- Chart-Updates throttlen (max 1/sec)
- Lazy Loading für große Tabellen
- WebSocket statt Polling

**LLM:**
- Prompt-Caching nutzen
- Lokales Ollama für schnellere Antworten
- Streaming-Responses aktivieren

---

## ✅ Erfolgs-Checkliste

- [ ] Backend läuft (`/health` = healthy)
- [ ] Frontend zeigt Maschinen-Liste
- [ ] Chart zeigt Messwerte
- [ ] Events-Tabelle gefüllt
- [ ] Chat antwortet (auch ohne LLM im Demo-Mode)
- [ ] Analyse findet Anomalien
- [ ] Auto-Refresh funktioniert

**Alles grün? Herzlichen Glückwunsch! 🎉**

---

**Fragen? Probleme?**  
→ Issue öffnen: https://github.com/Osamaiqji89/MachinaMind_AI_Agent/issues
