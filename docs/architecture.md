# MachinaMindAIAgent Systemarchitektur

## Überblick

MachinaMindAIAgent ist ein vollständiges Industrial AI System mit:

- **Backend**: Python (FastAPI, LangChain, RAG)
- **Frontend**: C++ Qt (MVP-Pattern)
- **Datenspeicherung**: SQLite/PostgreSQL
- **AI**: LLM + RAG für intelligente Analyse

---

## Systemarchitektur

```
┌────────────────────────────────────────────────────────────┐
│                      MachinaMindAIAgent                    │
└────────────────────────────────────────────────────────────┘

┌─────────────────────┐              ┌──────────────────────┐
│  C++ Qt Frontend    │              │  Python Backend      │
│  (MVP Pattern)      │◄───REST/────►│  (FastAPI)           │
└─────────────────────┘    JSON      └──────────────────────┘
         │                                      │
         │                             ┌────────┴────────┐
         │                             │                 │
         │                        ┌────▼────┐      ┌────▼─────┐
         │                        │   DB    │      │   RAG    │
         │                        │(SQLite) │      │ (FAISS)  │
         │                        └─────────┘      └──────────┘
         │                                              │
         └──────────────────────────────────────────────┴────
                              LLM (OpenAI/Ollama)
```

---

## Backend-Architektur (Python)

### Module

```
backend/
├── api/
│   └── main.py                 # FastAPI REST Endpoints
├── agents/
│   ├── data_agent.py          # Daten-Validierung & Kontext
│   ├── analysis_agent.py      # Anomalieerkennung (ML)
│   └── llm_agent.py           # LLM Wrapper (OpenAI/Anthropic)
├── database/
│   └── db_handler.py          # SQLite ORM
├── rag_engine/
│   └── rag_manager.py         # Vector Store & Retrieval
├── data_simulator.py          # Maschinendaten-Simulator
├── prompt_templates.py        # Zentrale Prompts
└── config.py                  # Configuration Management
```

### REST API Endpoints

| Endpoint | Methode | Beschreibung |
|----------|---------|--------------|
| `/health` | GET | Health Check + DB Stats |
| `/machines` | GET | Alle Maschinen |
| `/machines/{id}` | GET | Einzelne Maschine |
| `/measurements/{machine_id}` | GET | Sensor-Messungen |
| `/events` | GET | Events (Warnungen, Fehler) |
| `/chat` | POST | AI Chat Interface |
| `/analyze` | POST | Anomalie-Analyse |
| `/reports` | GET/POST | AI-generierte Reports |

### Datenbank-Schema

```sql
-- Maschinen
CREATE TABLE machines (
    id INTEGER PRIMARY KEY,
    name TEXT UNIQUE NOT NULL,
    type TEXT NOT NULL,
    location TEXT,
    meta_json TEXT
);

-- Sensordaten
CREATE TABLE measurements (
    id INTEGER PRIMARY KEY,
    machine_id INTEGER REFERENCES machines(id),
    timestamp TIMESTAMP NOT NULL,
    sensor_type TEXT NOT NULL,
    value REAL NOT NULL,
    unit TEXT
);

-- Events (Warnungen, Fehler)
CREATE TABLE events (
    id INTEGER PRIMARY KEY,
    machine_id INTEGER REFERENCES machines(id),
    timestamp TIMESTAMP NOT NULL,
    level TEXT CHECK(level IN ('INFO', 'WARNING', 'ERROR', 'CRITICAL')),
    message TEXT NOT NULL
);

-- AI Reports
CREATE TABLE reports (
    id INTEGER PRIMARY KEY,
    machine_id INTEGER REFERENCES machines(id),
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    report_type TEXT NOT NULL,
    report_text TEXT NOT NULL
);
```

---

## Frontend-Architektur (C++ Qt)

### MVP Pattern

```
┌─────────────────────────────────────────────────────────┐
│                   MVP Architecture                      │
└─────────────────────────────────────────────────────────┘

┌──────────┐         ┌────────────┐         ┌────────────┐
│  View    │◄────────┤ Presenter  │────────►│   Model    │
│ (UI nur) │ IView   │ (Logic)    │         │ (Data)     │
└──────────┘         └────────────┘         └────────────┘
      │                    │                       │
      │                    │                       │
  MainWindow        MainPresenter            DataModel
  - Charts          - Event Handling        - Cache
  - Tables          - Validation            - DTOs
  - Chat UI         - API Calls             - ApiClient
```

### Klassen-Struktur

**Model Layer** (`cpp_gui/model/`)
- `DTOs.h`: Data Transfer Objects (Machine, Measurement, Event, etc.)
- `DataModel`: Daten-Cache, Business Logic
- `ApiClient`: REST API Client (async)

**Presenter Layer** (`cpp_gui/presenter/`)
- `MainPresenter`: Orchestrierung, Event-Handling
- `IMainView`: View-Interface (Dependency Inversion)

**View Layer** (`cpp_gui/view/`)
- `MainWindow`: Qt Widgets, reine UI-Logik

### Code-Style Regeln (C++)

```cpp
// Klassen: UpperCamelCase
class MainPresenter { };

// Methoden: lowerCamelCase
void onMachineSelected(int id);

// Member-Variablen: m_ Prefix
DataModel* m_model;

// Konstanten: kUpperSnakeCase
const int kMaxRetries = 3;

// Interface: I Prefix
class IMainView { };
```

---

## AI/RAG Pipeline

### RAG Workflow

```
1. Dokument-Indexierung (Automatisch bei Simulator-Start):
   PDF/Text → Text-Extraktion → Chunking (500 chars, 50 overlap)
   → Embeddings (all-MiniLM-L6-v2, 384-dim) → FAISS Index (L2)
   → Persistierung (vector_store/faiss.index + documents.txt + metadata.txt)

2. Query Processing:
   User Question → Sentence-Transformer Embedding → FAISS Vector Search
   → Top-3 relevante Dokumente mit Score → Sortierung nach Relevanz

3. LLM Context Building:
   System Prompt + RAG Dokumente (Priorität) + DB Context (Maschinen, Messungen, Events)
   + User Question → Strukturierter Prompt für LLM

4. LLM Response:
   OpenAI/Anthropic → Antwort + Quellenangaben (Dokument-Namen + Scores)
```

### RAG Technologie-Stack

| Komponente | Technologie | Details |
|------------|-------------|---------|
| **Vector Database** | FAISS (CPU) | L2-Distanz, 384-dimensionale Vektoren |
| **Embedding Model** | sentence-transformers/all-MiniLM-L6-v2 | 384-dim, mehrsprachig, optimiert für Suche |
| **PDF Extraktion** | pypdf | Text-Extraktion aus PDF-Dokumenten |
| **Chunking** | Custom Logic | 500 Zeichen, 50 Zeichen Überlappung, Satzgrenzen |
| **Persistenz** | File System | `vector_store/` mit Index, Dokumente, Metadata |
| **Indexierung** | Automatisch | Bei Simulator-Start, Smart Caching, --reindex Flag |

### RAG Integration in Data Simulator

**Automatische Indexierung bei Startup:**

```python
# data_simulator.py - Zeilen 193-228
if __name__ == "__main__":
    # Check for --reindex flag
    force_reindex = "--reindex" in sys.argv
    
    # Find PDFs in backend directory
    pdfs = [f for f in os.listdir(".") if f.endswith(".pdf")]
    
    if pdfs:
        print(f"📄 Found {len(pdfs)} PDF(s) for RAG...")
        
        # Check if vector store exists
        vector_store_path = "vector_store/faiss.index"
        
        if os.path.exists(vector_store_path) and not force_reindex:
            print("ℹ️  Vector store exists. Use --reindex to rebuild.")
            print("✅ RAG ready (use existing index)")
        else:
            # Index PDFs
            rag_manager = RAGManager()
            for pdf in pdfs:
                rag_manager.index_directory(".", file_pattern="*.pdf")
            print(f"✅ RAG indexing complete ({len(pdfs)} PDF(s))")
    
    # Continue with simulation...
```

**Features:**
- ✅ **Smart Caching**: Prüft ob `vector_store/faiss.index` existiert
- ✅ **Auto-Indexing**: Indexiert PDFs automatisch beim ersten Start
- ✅ **Performance**: Lädt bestehenden Index in ~100ms statt ~20s neu zu indexieren
- ✅ **Manual Rebuild**: `--reindex` Flag erzwingt Neu-Indexierung
- ✅ **Production-Ready**: Keine manuelle Indexierung notwendig

### RAG Manager API

```python
from rag_engine.rag_manager import RAGManager

# Initialisierung
rag = RAGManager(
    model_name="sentence-transformers/all-MiniLM-L6-v2",
    store_path="vector_store"
)

# Dokumente indexieren
rag.index_directory(".", file_pattern="*.pdf")

# Suche durchführen
results = rag.retrieve("Wie oft Wartung?", top_k=3)
# Returns: [{"text": "...", "score": 0.65, "metadata": {...}}, ...]

# Vector Store speichern (automatisch)
# Lädt beim nächsten Start automatisch
```

### Prompt-Struktur

```python
SYSTEM_PROMPT = """
Du bist ein spezialisierter AI-Assistent für industrielle Maschinendatenanalyse.

Regeln:
- Faktisch basierend auf Daten
- Kennzeichne Unsicherheiten
- Nutze technische Terminologie
"""

Context = f"""
Maschine: {machine.name}
Messwerte: {recent_measurements}
Events: {recent_events}
RAG Docs: {retrieved_documents}
"""

User Query = "Was ist der Status von CNC-Mill-01?"
```

### Anomalieerkennung

**Methoden:**
1. **Z-Score**: Statistische Abweichung (3σ-Regel)
2. **IsolationForest**: ML-basierte Outlier Detection

**Workflow:**
```python
def detect_anomalies(measurements):
    # Z-Score
    mean, std = compute_stats(measurements)
    z_scores = abs((values - mean) / std)
    anomalies = values[z_scores > 3.0]
    
    # IsolationForest
    model = IsolationForest(contamination=0.1)
    predictions = model.fit_predict(values)
    
    return merge_results(anomalies, predictions)
```

---

## Deployment

### Lokale Entwicklung

```powershell
# Backend
cd backend
python -m venv venv
.\venv\Scripts\Activate.ps1
pip install -r requirements.txt
python api/main.py

# Frontend
cd cpp_gui
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build . --config Release
.\Release\MachinaMindAIAgent.exe
```

### Docker Deployment

```yaml
version: '3.8'

services:
  backend:
    build: ./backend
    ports:
      - "8000:8000"
    environment:
      - DATABASE_URL=sqlite:///data/MachinaData.db
    volumes:
      - ./data:/app/data

  # Optional: Lokales LLM
  ollama:
    image: ollama/ollama
    ports:
      - "11434:11434"
```

---

## Sicherheit & Best Practices

### Backend
- ✅ Input Validation (Pydantic)
- ✅ SQL Injection Prevention (Parameterized Queries)
- ✅ API Rate Limiting (FastAPI Middleware)
- ✅ Secrets Management (.env, nicht in Git)
- ✅ CORS Configuration

### Frontend
- ✅ HTTPS für Produktion
- ✅ Error Handling (try-catch)
- ✅ Memory Management (RAII, Smart Pointers)
- ✅ Thread Safety (Async Network Calls)

---

## Performance

### Backend
- **Request Latency**: < 100ms (ohne LLM)
- **LLM Latency**: 2-5s (abhängig von Modell)
- **DB Queries**: Indiziert (< 10ms)
- **RAG Retrieval**: < 50ms (FAISS)

### Frontend
- **UI Responsiveness**: 60 FPS
- **Chart Updates**: Async (nicht blockierend)
- **Memory**: < 200MB (idle)

---

## Testing-Strategie

### Backend
```bash
pytest tests/ -v --cov=backend
```

**Test-Typen:**
- Unit Tests: Agents, DB Handler
- Integration: API Endpoints (mocked LLM)
- Performance: Latency, Throughput

### Frontend
```bash
cd cpp_gui/build
ctest --output-on-failure
```

**Test-Typen:**
- Unit Tests: Presenter Logic (Google Test)
- Integration: API Client (Mock Server)
- UI Tests: Qt Test Framework

---

## Monitoring & Logging

### Backend Logging
```python
from loguru import logger

logger.info("API request received")
logger.error("Database connection failed")
```

**Strukturierte Logs (JSON):**
```json
{
  "timestamp": "2025-10-20T10:30:00",
  "level": "INFO",
  "message": "Chat request processed",
  "machine_id": 1,
  "latency_ms": 45
}
```

### Frontend Logging
- Console Output (Debug Mode)
- Log-Datei (Production)
- Error Reports an Backend

