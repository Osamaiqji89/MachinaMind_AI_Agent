# 📚 RAG (Retrieval-Augmented Generation) Guide

## Was ist RAG?

**RAG** kombiniert Information Retrieval mit Large Language Models, um präzise, faktenbasierte Antworten zu generieren, die auf spezifischen Dokumenten basieren.

### Vorteile

✅ **Faktentreue**: Antworten basieren auf echten Dokumenten  
✅ **Transparenz**: Quellenangaben zeigen, woher die Information kommt  
✅ **Aktualität**: Neue Dokumente ohne LLM-Retraining  
✅ **Domain-Spezifisch**: Spezialisiertes Wissen (z.B. Wartungsprotokolle)  

---

## Architektur

### Pipeline-Übersicht

```
┌─────────────────────────────────────────────────────────────┐
│                     RAG Pipeline                            │
└─────────────────────────────────────────────────────────────┘

1. INDEXIERUNG (Einmalig)
   ┌──────────┐    ┌──────────┐    ┌──────────┐    ┌──────────┐
   │   PDF    │───►│ Chunking │───►│Embeddings│───►│  FAISS   │
   │   Docs   │    │ (500chr) │    │ (384-dim)│    │  Index   │
   └──────────┘    └──────────┘    └──────────┘    └──────────┘
                                                          │
                                                          ▼
                                                   vector_store/
                                                   ├── faiss.index
                                                   ├── documents.txt
                                                   └── metadata.txt

2. QUERY (Jede Anfrage)
   ┌──────────┐    ┌──────────┐    ┌──────────┐    ┌──────────┐
   │  User    │───►│ Embedding│───►│  Vector  │───►│  Top-3   │
   │  Frage   │    │  (384d)  │    │  Search  │    │  Chunks  │
   └──────────┘    └──────────┘    └──────────┘    └──────────┘
                                                          │
                                                          ▼
3. GENERATION                                      Context Building
   ┌──────────┐    ┌──────────┐                   ┌────────────┐
   │   LLM    │◄───│ Prompt + │◄──────────────────│RAG Docs +  │
   │ (OpenAI) │    │ Context  │                   │DB Data     │
   └──────────┘    └──────────┘                   └────────────┘
        │
        ▼
   Antwort + Quellen
```

---

## Technologie-Stack

### 1. Vector Database: FAISS

**FAISS** (Facebook AI Similarity Search) ist eine hochoptimierte Library für Vektor-Ähnlichkeitssuche.

**Warum FAISS?**
- ✅ Extrem schnell (< 50ms für 10.000 Dokumente)
- ✅ CPU-optimiert (keine GPU notwendig)
- ✅ Produktionserprobt (von Meta entwickelt)
- ✅ Geringe Memory-Footprint

**Index-Typ:** `IndexFlatL2` (exakte L2-Distanz)

```python
import faiss

# Index erstellen (384 Dimensionen)
index = faiss.IndexFlatL2(384)

# Vektoren hinzufügen
index.add(embeddings)  # numpy array (n, 384)

# Suche durchführen
distances, indices = index.search(query_vector, k=3)
```

### 2. Embedding Model: all-MiniLM-L6-v2

**sentence-transformers/all-MiniLM-L6-v2** ist ein kompaktes, schnelles Embedding-Modell.

**Eigenschaften:**
- **Dimensionen**: 384
- **Größe**: ~80 MB
- **Geschwindigkeit**: ~1000 Sätze/Sekunde (CPU)
- **Sprachen**: Mehrsprachig (Deutsch + Englisch optimiert)
- **Training**: Semantic Similarity (STSB, SNLI)

**Warum dieses Modell?**
- ✅ Gutes Balance zwischen Qualität und Geschwindigkeit
- ✅ Kleine Modellgröße (praktikabel für Deployment)
- ✅ State-of-the-art Performance für Semantic Search

```python
from sentence_transformers import SentenceTransformer

model = SentenceTransformer('all-MiniLM-L6-v2')
embeddings = model.encode(["Wartung durchführen", "Maschine prüfen"])
# Shape: (2, 384)
```

### 3. Chunking-Strategie

**Problem:** LLMs haben begrenzte Context-Länge, ganze Dokumente überfordern das System.

**Lösung:** Dokumente in semantisch sinnvolle Chunks aufteilen.

**Parameter:**
- **Chunk-Größe**: 500 Zeichen
- **Overlap**: 50 Zeichen (für Kontext-Kontinuität)
- **Split-Logik**: Bevorzugt Satzgrenzen (`.`, `!`, `?`)

**Warum 500 Zeichen?**
- ✅ Passt in typische Embeddings
- ✅ Enthält genug Kontext
- ✅ Nicht zu klein (würde Kontext verlieren)
- ✅ Nicht zu groß (würde unnötige Info enthalten)

```python
def chunk_text(text, chunk_size=500, overlap=50):
    chunks = []
    start = 0
    while start < len(text):
        end = start + chunk_size
        
        # Find sentence boundary
        if end < len(text):
            for char in ['. ', '! ', '? ', '\n\n']:
                pos = text.rfind(char, start, end)
                if pos != -1:
                    end = pos + 1
                    break
        
        chunks.append(text[start:end].strip())
        start = end - overlap
    
    return chunks
```

---

## RAG Manager Implementation

### Klassen-Struktur

```python
# backend/rag_engine/rag_manager.py

class RAGManager:
    def __init__(
        self,
        model_name="sentence-transformers/all-MiniLM-L6-v2",
        store_path="vector_store"
    ):
        self.model = SentenceTransformer(model_name)
        self.index = None
        self.documents = []
        self.metadata = []
        self.store_path = store_path
        self._load_index()  # Lädt existierenden Index
    
    def add_documents(self, texts, metadatas):
        """Fügt Dokumente zum Vector Store hinzu"""
        # Chunking
        chunks = [self._chunk_text(t) for t in texts]
        
        # Embeddings generieren
        embeddings = self.model.encode(chunks)
        
        # Zu FAISS Index hinzufügen
        if self.index is None:
            self.index = faiss.IndexFlatL2(384)
        self.index.add(embeddings)
        
        # Dokumente + Metadata speichern
        self.documents.extend(chunks)
        self.metadata.extend(metadatas)
        
        # Persistieren
        self._save_index()
    
    def retrieve(self, query, top_k=3):
        """Sucht relevante Dokumente"""
        # Query Embedding
        query_vector = self.model.encode([query])
        
        # FAISS Suche
        distances, indices = self.index.search(query_vector, top_k)
        
        # Ergebnisse formatieren
        results = []
        for i, idx in enumerate(indices[0]):
            score = 1.0 / (1.0 + distances[0][i])  # Distanz → Score
            results.append({
                "text": self.documents[idx],
                "score": score,
                "metadata": self.metadata[idx]
            })
        
        return results
    
    def index_directory(self, path, file_pattern="*.pdf"):
        """Indexiert alle PDFs in einem Verzeichnis"""
        from pypdf import PdfReader
        
        for pdf_file in Path(path).glob(file_pattern):
            reader = PdfReader(pdf_file)
            text = "\n".join([page.extract_text() for page in reader.pages])
            
            self.add_documents(
                texts=[text],
                metadatas=[{"source": pdf_file.name}]
            )
```

### Persistenz-Layer

**Vector Store Struktur:**

```
backend/vector_store/
├── faiss.index          # Binärer FAISS Index
├── documents.txt        # Alle Chunks (ein Chunk pro Zeile)
└── metadata.txt         # JSON Metadata pro Chunk
```

**Speichern:**

```python
def _save_index(self):
    os.makedirs(self.store_path, exist_ok=True)
    
    # FAISS Index
    faiss.write_index(
        self.index, 
        os.path.join(self.store_path, "faiss.index")
    )
    
    # Dokumente
    with open(os.path.join(self.store_path, "documents.txt"), "w", encoding="utf-8") as f:
        f.write("\n".join(self.documents))
    
    # Metadata
    with open(os.path.join(self.store_path, "metadata.txt"), "w") as f:
        json.dump(self.metadata, f)
```

**Laden:**

```python
def _load_index(self):
    index_path = os.path.join(self.store_path, "faiss.index")
    
    if not os.path.exists(index_path):
        return  # Noch kein Index vorhanden
    
    # FAISS Index
    self.index = faiss.read_index(index_path)
    
    # Dokumente
    with open(os.path.join(self.store_path, "documents.txt"), "r", encoding="utf-8") as f:
        self.documents = f.read().split("\n")
    
    # Metadata
    with open(os.path.join(self.store_path, "metadata.txt"), "r") as f:
        self.metadata = json.load(f)
```

---

## Integration mit LLM Agent

### LLM Agent mit RAG

```python
# backend/agents/llm_agent.py

class LLMAgent:
    def __init__(self):
        self.client = OpenAI(api_key=os.getenv("OPENAI_API_KEY"))
        self.rag_manager = RAGManager()  # RAG Integration
    
    async def query(self, question, context=None):
        # 1. RAG Retrieval
        rag_docs = self.rag_manager.retrieve(question, top_k=3)
        
        # 2. Context Building
        prompt = self._build_prompt(question, context, rag_docs)
        
        # 3. LLM Call
        response = await self.client.chat.completions.create(
            model="gpt-4",
            messages=[
                {"role": "system", "content": SYSTEM_PROMPT},
                {"role": "user", "content": prompt}
            ]
        )
        
        answer = response.choices[0].message.content
        
        # 4. Return mit Quellen
        sources = [
            {
                "text": doc["text"][:200] + "...",
                "score": doc["score"],
                "source": doc["metadata"]["source"]
            }
            for doc in rag_docs
        ]
        
        return answer, sources
```

### Prompt Building mit RAG

**Hinweis:** Das Projekt nutzt zentrale Prompt-Templates aus `backend/prompt_templates.py`:

```python
from prompt_templates import (
    build_chat_prompt_with_rag,  # MIT RAG-Dokumenten
    build_chat_prompt,            # OHNE RAG (Fallback)
)

# Beispiel: Prompt mit RAG erstellen
user_prompt = build_chat_prompt_with_rag(
    user_question="Wie oft CNC-Maschine warten?",
    context={
        "machine": {"name": "CNC-Mill-01", "type": "CNC"},
        "recent_measurements": [...],
        "recent_events": [...]
    },
    rag_documents=[
        {
            "content": "CNC-Fräsen sollten monatlich gewartet werden...",
            "score": 0.85,
            "source": "Wartungsprotokoll.pdf"
        }
    ]
)

# Resultat:
"""
**Verfügbare Maschinendaten:**
Maschine: CNC-Mill-01
Messwerte: 10 Einträge
Events: 5 Einträge

**Relevante Dokumentation (aus Wissensdatenbank):**

**Dokument 1** (Relevanz: 0.85, Quelle: Wartungsprotokoll.pdf):
CNC-Fräsen sollten monatlich gewartet werden...

**Benutzerfrage:**
Wie oft CNC-Maschine warten?

**Antwort:**
Beantworte die Frage basierend auf den bereitgestellten Dokumenten...
"""
```

**Legacy Beispiel (veraltet - nicht mehr verwenden):**

```python
# VERALTET - nur zur Referenz
def _build_prompt_legacy(self, question, context, rag_docs):
    prompt_parts = []
    
    # RAG Dokumente (höchste Priorität)
    if rag_docs:
        prompt_parts.append("📚 Relevante Dokumente:")
        for i, doc in enumerate(rag_docs, 1):
            prompt_parts.append(f"{i}. [{doc['metadata']['source']}]")
            prompt_parts.append(f"   {doc['text'][:300]}...")
        prompt_parts.append("")
    
    # Datenbank Context
    if context:
        prompt_parts.append("🗄️ Aktuelle Maschinendaten:")
        prompt_parts.append(f"Maschine: {context['machine']['name']}")
        prompt_parts.append(f"Messungen: {len(context['measurements'])} Werte")
        prompt_parts.append("")
    
    # User Frage
    prompt_parts.append("❓ Frage:")
    prompt_parts.append(question)
    
    return "\n".join(prompt_parts)
```

**Empfehlung:** Nutze immer `build_chat_prompt_with_rag()` aus `prompt_templates.py` für konsistente Prompts!

---

## Automatische Indexierung

### Integration in Data Simulator

**Problem:** Manuelle Indexierung ist fehleranfällig und umständlich.

**Lösung:** Automatische Indexierung beim Simulator-Start mit Smart Caching.

```python
# backend/data_simulator.py

def setup_rag():
    """Automatisches RAG Setup mit Caching"""
    force_reindex = "--reindex" in sys.argv
    
    # PDFs finden
    pdfs = [f for f in os.listdir(".") if f.endswith(".pdf")]
    
    if not pdfs:
        print("⚠️  Keine PDFs gefunden. RAG nicht verfügbar.")
        return
    
    print(f"📄 Found {len(pdfs)} PDF(s) for RAG...")
    
    # Prüfen ob Index existiert
    vector_store_path = "vector_store/faiss.index"
    
    if os.path.exists(vector_store_path) and not force_reindex:
        print("ℹ️  Vector store exists. Use --reindex to rebuild.")
        print("✅ RAG ready (use existing index)")
        return
    
    # Indexierung durchführen
    print("🔄 Indexing PDFs... (this may take ~20 seconds)")
    rag_manager = RAGManager()
    rag_manager.index_directory(".", file_pattern="*.pdf")
    print(f"✅ RAG indexing complete ({len(pdfs)} PDF(s))")

if __name__ == "__main__":
    setup_rag()
    # ... continue with simulation
```

**Features:**
- ✅ Läuft automatisch bei jedem Simulator-Start
- ✅ Erkennt existierenden Vector Store (< 100ms)
- ✅ Überspringt Neu-Indexierung wenn nicht nötig
- ✅ `--reindex` Flag für manuelle Neu-Indexierung
- ✅ Robustes Error Handling (fehlende PDFs = kein Crash)

---

## API Integration

### Chat Endpoint mit RAG Sources

```python
# backend/api/main.py

class ChatResponse(BaseModel):
    answer: str
    sources: List[dict] = []

@app.post("/chat", response_model=ChatResponse)
async def chat_endpoint(request: ChatRequest):
    llm = LLMAgent()
    
    # Context aus DB
    context = {
        "machine": db.get_machine(request.machine_id),
        "measurements": db.get_measurements(request.machine_id, limit=100)
    }
    
    # LLM Query mit RAG
    answer, sources = await llm.query(request.message, context)
    
    return ChatResponse(answer=answer, sources=sources)
```

**Beispiel Response:**

```json
{
  "answer": "Die CNC-Maschine sollte alle 3 Monate gewartet werden...",
  "sources": [
    {
      "text": "Wartungsintervall: Alle 3 Monate müssen folgende Punkte...",
      "score": 0.876,
      "source": "Wartungsprotokoll_industrielle_Maschinen.pdf"
    },
    {
      "text": "Bei intensiver Nutzung kann das Intervall auf 2 Monate...",
      "score": 0.654,
      "source": "Wartungsprotokoll_industrielle_Maschinen.pdf"
    }
  ]
}
```

---

## Performance-Optimierung

### Benchmark

| Operation | Kalt-Start | Warm-Start | Optimierung |
|-----------|------------|------------|-------------|
| PDF Indexierung | ~20s | - | Einmalig, dann gecacht |
| Index Laden | - | ~100ms | Automatisch bei Start |
| Embedding (Query) | ~50ms | ~50ms | CPU-optimiert |
| FAISS Search | ~5ms | ~5ms | In-Memory |
| **Gesamt (Query)** | ~20s | **~155ms** | **129x schneller** |

### Optimierungs-Tipps

1. **Index Caching**: Immer persistieren und laden
   ```python
   # ✅ Gut
   rag = RAGManager()  # Lädt automatisch
   
   # ❌ Schlecht
   rag.index_directory()  # Jedes Mal neu indexieren
   ```

2. **Batch Embeddings**: Mehrere Dokumente gleichzeitig
   ```python
   # ✅ Gut (batch)
   embeddings = model.encode(all_texts)  # 1x API Call
   
   # ❌ Schlecht (loop)
   for text in all_texts:
       embedding = model.encode([text])  # N API Calls
   ```

3. **Top-K Limitierung**: Nicht zu viele Dokumente
   ```python
   # ✅ Gut
   results = rag.retrieve(query, top_k=3)  # Genug für Context
   
   # ❌ Übertrieben
   results = rag.retrieve(query, top_k=100)  # Überflutet LLM
   ```

4. **Chunk-Größe**: Balance zwischen Kontext und Präzision
   - Zu klein (< 200 chars): Verliert Kontext
   - Optimal (500 chars): Gute Balance
   - Zu groß (> 1000 chars): Zu viel Rauschen

---

## Troubleshooting

### Problem: "No module named 'sentence_transformers'"

**Lösung:**
```powershell
pip install sentence-transformers faiss-cpu pypdf
```

### Problem: NumPy Compatibility Warnings

**Symptom:**
```
RuntimeWarning: numpy.core is frozen... __getattr__ on numpy.core
```

**Lösung (optional):**
```powershell
pip install "numpy<2.0"
```

**Hinweis:** Nicht kritisch, System funktioniert trotzdem.

### Problem: Vector Store wird immer neu erstellt

**Check:**
```python
import os
print(os.path.exists("vector_store/faiss.index"))  # Sollte True sein
```

**Lösung:** Stelle sicher, dass Working Directory korrekt ist:
```powershell
cd backend
python data_simulator.py --once
```

### Problem: RAG findet keine relevanten Dokumente

**Debug:**
```python
results = rag.retrieve("test query", top_k=3)
for r in results:
    print(f"Score: {r['score']:.3f} - {r['text'][:100]}")
```

**Typische Scores:**
- `> 0.7`: Sehr relevant
- `0.5 - 0.7`: Relevant
- `< 0.5`: Wenig relevant

**Lösung:** Verbessere Query-Formulierung oder füge mehr Dokumente hinzu.

---

## Nächste Schritte

1. **Mehr Dokumente hinzufügen**: Lege weitere PDFs in `backend/` ab
2. **Frontend Integration**: Zeige Sources in der UI an
3. **Query-Optimierung**: Experimentiere mit verschiedenen Frage-Formulierungen
4. **Monitoring**: Logge RAG-Performance (Latency, Scores)

**Weiterführende Ressourcen:**
- [FAISS Documentation](https://github.com/facebookresearch/faiss)
- [Sentence Transformers](https://www.sbert.net/)
- [RAG Paper (Original)](https://arxiv.org/abs/2005.11401)
