# 📖 MachinaMind AI Agent - Dokumentation

Willkommen zur Dokumentation des MachinaMind AI Agent Projekts!

---

## 📚 Dokumentations-Übersicht

### 🚀 Schnellstart

- **[QUICKSTART.md](QUICKSTART.md)** - 5-Minuten Setup
  - Demo-Modus für sofortigen Start
  - Manuelle Einrichtung mit Details
  - **Automatischer Backend-Start durch UI**
  - Docker Deployment

### 🏗️ Architektur & Design

- **[architecture.md](architecture.md)** - Vollständige System-Architektur
  - Backend Module (Python/FastAPI)
  - Frontend Design (C++/Qt MVP Pattern)
  - **RAG Pipeline im Detail**
  - Datenbank-Schema
  - API Endpoints
  - Deployment-Strategien

### 🤖 RAG System

- **[RAG_GUIDE.md](RAG_GUIDE.md)** - Umfassende RAG-Dokumentation ⭐ **NEU**
  - Was ist RAG? (Retrieval-Augmented Generation)
  - Technologie-Stack: FAISS + sentence-transformers
  - Embedding Model: all-MiniLM-L6-v2 (384-dim)
  - Chunking-Strategie (500 chars, 50 overlap)
  - RAG Manager Implementation
  - Automatische Indexierung im Simulator
  - Performance-Optimierung
  - Troubleshooting & Best Practices

### 🖥️ UI & Backend Management

- **[UI_STARTUP_GUIDE.md](UI_STARTUP_GUIDE.md)** - UI Startup & Backend-Management ⭐ **NEU**
  - Wie die UI das Backend automatisch startet
  - Voraussetzungen & Setup
  - Verzeichnis-Struktur
  - Troubleshooting (Port-Konflikte, Prozess-Management)
  - Monitoring & Logging
  - Best Practices für Entwicklung vs. Produktion

### 🔧 Setup & Konfiguration

- **[OPENAI_SETUP.md](OPENAI_SETUP.md)** - OpenAI API Integration
  - API Key Konfiguration
  - Model-Auswahl
  - Rate Limits & Kosten

- **[HUGGINGFACE_SETUP.md](HUGGINGFACE_SETUP.md)** - HuggingFace Integration
  - Lokale Modelle
  - Embedding Models
  - Alternative zu OpenAI

### 📊 Projekt-Übersicht

- **[PROJECT_SUMMARY.md](PROJECT_SUMMARY.md)** - Vollständige Projekt-Zusammenfassung
  - Implementierte Features (15+ Module)
  - Design Patterns
  - Feature-Matrix mit RAG Integration
  - Code-Statistiken (8.600+ Zeilen)

---

## 🎯 Für verschiedene Zielgruppen

### 👨‍💻 Entwickler (Backend)

**Start hier:**
1. [QUICKSTART.md](QUICKSTART.md) - Entwicklungs-Umgebung einrichten
2. [architecture.md](architecture.md) - Backend-Architektur verstehen
3. [RAG_GUIDE.md](RAG_GUIDE.md) - RAG-System implementieren/erweitern

**Relevante Dateien:**
```
backend/
├── agents/llm_agent.py          # LLM + RAG Integration
├── rag_engine/rag_manager.py    # Vector Store Management
├── data_simulator.py            # Auto-Indexierung
└── api/main.py                  # REST Endpoints
```

### 🖼️ Entwickler (Frontend)

**Start hier:**
1. [QUICKSTART.md](QUICKSTART.md) - Qt Umgebung einrichten
2. [UI_STARTUP_GUIDE.md](UI_STARTUP_GUIDE.md) - Backend-Management verstehen
3. [architecture.md](architecture.md) - MVP Pattern & API Integration

**Relevante Dateien:**
```
cpp_gui/
├── view/MainWindow.cpp          # UI + Backend-Start-Logik
├── presenter/MainPresenter.cpp  # Business Logic
└── model/ApiClient.cpp          # REST API Kommunikation
```

### 🎓 Lernende

**Lernpfad:**
1. [PROJECT_SUMMARY.md](PROJECT_SUMMARY.md) - Was wurde gebaut?
2. [architecture.md](architecture.md) - Wie funktioniert das System?
3. [RAG_GUIDE.md](RAG_GUIDE.md) - Wie funktioniert RAG im Detail?
4. [QUICKSTART.md](QUICKSTART.md) - Hands-on: System starten

### 🚀 Produktions-Deployment

**Checkliste:**
1. [QUICKSTART.md](QUICKSTART.md) → Docker Deployment
2. [UI_STARTUP_GUIDE.md](UI_STARTUP_GUIDE.md) → Production Best Practices
3. [RAG_GUIDE.md](RAG_GUIDE.md) → Performance-Optimierung
4. [OPENAI_SETUP.md](OPENAI_SETUP.md) → API Keys sicher konfigurieren

---

## 🔑 Wichtigste Features (Update 2025-10)

### ✨ Neu: RAG Integration

- **Automatische PDF-Indexierung** beim Simulator-Start
- **Smart Caching**: Vector Store wird nur einmal erstellt
- **FAISS Vector Database**: Schnelle semantische Suche (< 50ms)
- **Quellenangaben**: Chat-Antworten mit Dokumenten-Referenzen
- **384-dimensionale Embeddings**: all-MiniLM-L6-v2 (mehrsprachig)

**Beispiel-Workflow:**
```powershell
# 1. PDF ins backend/ legen
cp Wartungsprotokoll.pdf backend/

# 2. Simulator startet → indexiert automatisch
python data_simulator.py --once

# 3. Chat-Anfrage in UI
"Wie oft sollte eine CNC-Maschine gewartet werden?"
→ Antwort mit Quelle: "Wartungsprotokoll.pdf (Score: 0.876)"
```

### 🚀 Neu: Automatischer Backend-Start

- **UI startet Backend automatisch** beim Start
- **Kein manueller Start notwendig**: `.\MachinaMindAIAgent.exe` genügt
- **Prozess-Management**: Backend wird beim UI-Close beendet
- **Health Monitoring**: Automatische Verbindungsprüfung

---

## 📂 Datei-Struktur

```
docs/
├── README.md                    # Diese Datei (Navigation)
├── QUICKSTART.md               # 5-Min Setup (aktualisiert mit Auto-Start)
├── architecture.md             # System-Architektur (erweitert mit RAG)
├── RAG_GUIDE.md               # ⭐ NEU: Umfassende RAG-Dokumentation
├── UI_STARTUP_GUIDE.md        # ⭐ NEU: UI Backend-Management
├── PROJECT_SUMMARY.md          # Projekt-Zusammenfassung (aktualisiert)
├── OPENAI_SETUP.md            # OpenAI API Setup
└── HUGGINGFACE_SETUP.md       # HuggingFace Setup
```

---

## 🛠️ Schnellreferenz

### Backend starten

**Option A - Automatisch (über UI):**
```powershell
cd cpp_gui\build\Release
.\MachinaMindAIAgent.exe  # Backend startet automatisch!
```

**Option B - Manuell (für Debugging):**
```powershell
cd backend
.\venv\Scripts\Activate.ps1
python api/main.py
```

### RAG neu indexieren

```powershell
cd backend
python data_simulator.py --once --reindex
```

### Daten-Simulation

```powershell
# Einmalig
python data_simulator.py --once

# Kontinuierlich (5s Intervall)
python data_simulator.py --continuous

# Mit RAG Neuindexierung
python data_simulator.py --once --reindex
```

### API testen

```powershell
# Health Check
curl http://localhost:8000/health

# Maschinen abrufen
curl http://localhost:8000/machines

# Chat (mit RAG)
curl -X POST http://localhost:8000/chat `
  -H "Content-Type: application/json" `
  -d '{"message": "Wartungsintervall?", "machine_id": 1}'
```

---

## 🐛 Troubleshooting

**Problem: Backend startet nicht**
→ Siehe [UI_STARTUP_GUIDE.md](UI_STARTUP_GUIDE.md#troubleshooting)

**Problem: RAG findet keine Dokumente**
→ Siehe [RAG_GUIDE.md](RAG_GUIDE.md#troubleshooting)

**Problem: NumPy Warnings**
→ Siehe [RAG_GUIDE.md](RAG_GUIDE.md#problem-numpy-compatibility-warnings)

---

## 📞 Support & Weiterführende Infos

- **GitHub Issues**: Für Bugs und Feature Requests
- **Architecture Docs**: [architecture.md](architecture.md)
- **RAG Details**: [RAG_GUIDE.md](RAG_GUIDE.md)
- **API Dokumentation**: http://localhost:8000/docs (wenn Backend läuft)

---

## 📝 Changelog (Dokumentation)

### 2025-10-22
- ✅ **RAG_GUIDE.md** hinzugefügt (umfassende RAG-Dokumentation)
- ✅ **UI_STARTUP_GUIDE.md** hinzugefügt (automatisches Backend-Management)
- ✅ **QUICKSTART.md** aktualisiert (Auto-Start Feature)
- ✅ **architecture.md** erweitert (RAG Pipeline Details)
- ✅ **PROJECT_SUMMARY.md** aktualisiert (RAG Features, neue Stats)
- ✅ **README.md** (diese Datei) neu erstellt für Navigation

### 2025-09
- ✅ Initiale Dokumentation erstellt
- ✅ Architektur-Diagramme
- ✅ Setup-Guides für OpenAI & HuggingFace

---

**Viel Erfolg mit MachinaMind AI Agent! 🚀**

---

## 🎯 Projektziel

MachinaMindAIAgent ist ein industrial-taugliches System zur Echtzeit-Überwachung und intelligenten Analyse von Maschinendaten:

- **Datenpipeline**: Sammlung & Speicherung von Sensordaten (SQL)
- **AI-Agenten**: RAG-basierte Analyse mit LLM-Backend (Python/FastAPI)
- **Qt-Frontend**: Professional C++/Qt GUI mit MVP-Pattern
- **Anomalieerkennung**: Automatische Erkennung & Diagnose von Maschinenfehlern

---

## 🏗️ Architektur

```
┌─────────────────┐         ┌──────────────────┐
│  C++ Qt GUI     │ ◄─REST──┤ Python Backend   │
│  (MVP Pattern)  │         │  (FastAPI/MCP)   │
└─────────────────┘         └──────────────────┘
        │                            │
        │                   ┌────────┴────────┐
        │                   │                 │
        │              ┌────▼────┐     ┌─────▼─────┐
        │              │   DB    │     │ RAG Engine│
        │              │ (SQLite)│     │ (FAISS)   │
        │              └─────────┘     └───────────┘
        │                                    │
        └────────────────────────────────────┴─── LLM (Hugging Face / OpenAI)
```

---

## 🚀 Quick Start

### Voraussetzungen

- **Python**: 3.10+
- **Qt**: 6.9+
- **CMake**: 3.20+
- **Compiler**: MSVC 2022 / GCC 11+ / Clang 14+

### Installation

```powershell
# 1. Backend Setup
cd backend
python -m venv venv
.\venv\Scripts\Activate.ps1
pip install -r requirements.txt

# 2. Hugging Face API Key konfigurieren
# Erstellen Sie einen kostenlosen Account: https://huggingface.co/join
# Erstellen Sie einen API Token: https://huggingface.co/settings/tokens
# Tragen Sie den Token in backend/.env ein:
# HUGGINGFACE_API_KEY=hf_your_actual_token_here

# 3. Datenbank initialisieren
python database/db_handler.py --init

# 4. Backend starten
python api/main.py

# 5. Frontend Build (in neuem Terminal)
cd cpp_gui
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build . --config Release

# 6. GUI starten
.\Release\MachinaMindAIAgent.exe
```

### Hugging Face API Key Setup

1. **Account erstellen**: Gehen Sie zu [huggingface.co/join](https://huggingface.co/join)
2. **API Token generieren**: [huggingface.co/settings/tokens](https://huggingface.co/settings/tokens)
   - Click "New token"
   - Name: "MachinaMindAIAgent"
   - Type: "Read"
3. **Token eintragen**: In `backend/.env`:
   ```env
   HUGGINGFACE_API_KEY=hf_xxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
   ```

Das Standard-Modell ist `meta-llama/Llama-3.2-3B-Instruct` - schnell und kostenlos!

### Demo Modus

```powershell
# Startet Simulator + Backend + GUI
.\demo.bat
```

---

## 📁 Projektstruktur

```
MachinaMind_AI_Agent/
├── backend/
│   ├── agents/              # AI Agenten (Data, Analysis, LLM)
│   ├── api/                 # FastAPI REST Endpoints
│   ├── database/            # DB Schema & Handler
│   ├── rag_engine/          # Vector Store & Retrieval
│   ├── data_simulator.py    # Maschinendaten-Simulator
│   └── requirements.txt
├── cpp_gui/
│   ├── model/               # MVP: Datenmodelle
│   ├── view/                # MVP: Qt UI Components
│   ├── presenter/           # MVP: Business Logic
│   ├── ui/                  # Qt Designer Files
│   ├── main.cpp
│   └── CMakeLists.txt
├── tests/
│   ├── unit/                # Unit Tests (Python & C++)
│   └── integration/         # Integration Tests
├── docs/
│   ├── architecture.md      # Systemarchitektur
│   ├── developer_guide.md   # Entwickler-Dokumentation
│   └── user_guide.md        # Benutzerhandbuch
├── deploy/
│   ├── docker-compose.yml   # Container Setup
│   └── Dockerfile
└── .github/
    └── workflows/           # CI/CD Pipeline
```

---

## 🛠️ Entwicklung

### Code Style

**C++ (Qt)**
- Formatter: `clang-format` (Google Style)
- Naming: `UpperCamelCase` (Klassen), `lowerCamelCase` (Methoden), `m_` prefix (Member)
- Pattern: **MVP (Model-View-Presenter)**

**Python**
- Formatter: `black`, Linter: `ruff`
- Type Hints: mandatory (mypy in CI)
- Tests: `pytest`

### Tests ausführen

```powershell
# Python Tests
cd backend
pytest tests/ -v

# C++ Tests
cd cpp_gui/build
ctest --output-on-failure
```

### CI/CD

GitHub Actions Pipeline:
- ✅ Linting (clang-format, ruff)
- ✅ Build (CMake, Python)
- ✅ Unit Tests
- ✅ Integration Tests
- ✅ Security Scans

---

## 📝 Lizenz

MIT License - siehe [LICENSE](LICENSE)

---

## 👥 Beitragen

Contributions willkommen! Siehe [CONTRIBUTING.md](CONTRIBUTING.md)

---

## 📞 Kontakt

**Projekt**: MachinaMindAIAgent  
**Maintainer**: Sultan  
**Repository**: https://github.com/Osamaiqji89/MachinaMind_AI_Agent
