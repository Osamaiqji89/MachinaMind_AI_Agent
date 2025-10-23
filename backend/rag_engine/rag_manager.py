"""
RAG Manager
Verwaltet Embeddings, Vector Store und Retrieval
"""

from pathlib import Path

from loguru import logger

# Lazy-load RAG dependencies to avoid torch DLL issues on Windows
_deps_loaded = False
_deps_available = False
_deps_error = None
_faiss = None
_np = None
_SentenceTransformer = None


def _ensure_rag_deps():
    """Lazy import of RAG dependencies - only loads when needed"""
    global _deps_loaded, _deps_available, _deps_error, _faiss, _np, _SentenceTransformer

    if _deps_loaded:
        return _deps_available

    _deps_loaded = True

    try:
        import faiss as faiss_mod
        import numpy as np_mod
        from sentence_transformers import SentenceTransformer as ST

        _faiss = faiss_mod
        _np = np_mod
        _SentenceTransformer = ST
        _deps_available = True
        logger.info("✓ RAG dependencies loaded successfully")
        return True
    except Exception as e:
        _deps_error = str(e)
        _deps_available = False
        logger.warning(f"⚠️  RAG dependencies unavailable: {e}")
        logger.warning("Backend will run without RAG support")
        return False


from config import settings


class RAGManager:
    """Managed Retrieval-Augmented Generation Pipeline"""

    def __init__(
        self,
        embedding_model: str | None = None,
        vector_store_path: str = "vector_store",
    ):
        self.embedding_model_name = embedding_model or settings.embedding_model
        self.vector_store_path = Path(vector_store_path)
        self.vector_store_path.mkdir(exist_ok=True)

        # Initialize components
        self.embedder = None
        self.index = None
        self.documents: list[str] = []  # Speichert Original-Dokumente
        self.metadata: list[dict] = []  # Speichert Metadaten
        self.dimension = 0
        self.rag_available = False

        # Try to load RAG dependencies
        if not _ensure_rag_deps():
            logger.warning("⚠️  RAG disabled (dependencies missing)")
            return

        try:
            logger.info(f"Loading embedding model: {self.embedding_model_name}")
            if _SentenceTransformer is not None:
                self.embedder = _SentenceTransformer(self.embedding_model_name)
            self.dimension = (
                self.embedder.get_sentence_embedding_dimension() if self.embedder else 0
            )
            self.rag_available = True
            logger.info(f"✓ Embedding model loaded, dimension: {self.dimension}")
        except Exception as e:
            logger.warning(f"⚠️  Failed to load embedding model: {e}")
            self.embedder = None
            self.rag_available = False
            return

        # Load existing index if available
        self._load_index()

    def _load_index(self) -> None:
        """Lädt existierenden Index von Disk"""
        index_path = self.vector_store_path / "faiss.index"
        docs_path = self.vector_store_path / "documents.txt"
        meta_path = self.vector_store_path / "metadata.txt"

        if index_path.exists() and _faiss and self.embedder:
            try:
                self.index = _faiss.read_index(str(index_path))
                if self.index:
                    logger.info(f"Loaded FAISS index with {self.index.ntotal} vectors")

                # Load documents
                if docs_path.exists():
                    self.documents = docs_path.read_text(encoding="utf-8").split("\n---\n")

                # Load metadata
                if meta_path.exists():
                    self.metadata = [
                        line.strip() for line in meta_path.read_text(encoding="utf-8").split("\n")
                    ]

            except Exception as e:
                logger.error(f"Failed to load index: {e}")
                self._init_new_index()
        else:
            self._init_new_index()

    def _init_new_index(self) -> None:
        """Initialisiert neuen FAISS Index"""
        if _faiss and self.embedder:
            self.index = _faiss.IndexFlatL2(self.dimension)
            logger.info("Initialized new FAISS index")

    def add_documents(
        self,
        documents: list[str],
        metadata: list[str] | None = None,
        chunk_size: int | None = None,
    ) -> int:
        """
        Fügt Dokumente zum Index hinzu

        Args:
            documents: Liste von Texten
            metadata: Optionale Metadaten (z.B. Dateinamen, IDs)
            chunk_size: Optional, für Text-Chunking

        Returns:
            Anzahl hinzugefügter Dokumente
        """
        if not self.embedder or not self.index:
            logger.warning("RAG not initialized - cannot add documents")
            return 0

        chunk_size = chunk_size or settings.chunk_size

        # Optional: Text chunking
        chunks = []
        chunk_metadata = []

        for i, doc in enumerate(documents):
            doc_chunks = self._chunk_text(doc, chunk_size, settings.chunk_overlap)
            chunks.extend(doc_chunks)

            # Metadata für jeden Chunk
            meta = metadata[i] if metadata and i < len(metadata) else f"doc_{i}"
            chunk_metadata.extend([f"{meta}_chunk_{j}" for j in range(len(doc_chunks))])

        if not chunks:
            return 0

        try:
            # Embeddings generieren
            logger.info(f"Generating embeddings for {len(chunks)} chunks...")
            embeddings = self.embedder.encode(chunks, show_progress_bar=True)

            # Zum Index hinzufügen
            if _np:
                embeddings = _np.array(embeddings).astype("float32")
                self.index.add(embeddings)

            self.documents.extend(chunks)
            self.metadata.extend(chunk_metadata)

            logger.info(f"Added {len(chunks)} chunks to index (total: {self.index.ntotal})")

            # Speichern
            self._save_index()

            return len(chunks)

        except Exception as e:
            logger.error(f"Failed to add documents: {e}")
            return 0

    def _chunk_text(self, text: str, chunk_size: int, overlap: int) -> list[str]:
        """
        Teilt Text in überlappende Chunks
        """
        if len(text) <= chunk_size:
            return [text]

        chunks = []
        start = 0

        while start < len(text):
            end = start + chunk_size
            chunk = text[start:end]

            # Versuche an Satzende zu brechen
            if end < len(text):
                last_period = chunk.rfind(".")
                last_newline = chunk.rfind("\n")
                break_point = max(last_period, last_newline)

                if break_point > chunk_size // 2:  # Nur wenn sinnvoll
                    chunk = chunk[: break_point + 1]
                    end = start + break_point + 1

            chunks.append(chunk.strip())
            start = end - overlap

        return [c for c in chunks if c]  # Filter empty

    def retrieve(
        self, query: str, k: int = 5, score_threshold: float | None = None
    ) -> list[tuple[str, float]]:
        """
        Retrieves relevante Dokumente für Query

        Args:
            query: Suchanfrage
            k: Anzahl Ergebnisse
            score_threshold: Optional, minimaler Ähnlichkeits-Score

        Returns:
            Liste von (document, score) Tupeln
        """
        if not self.embedder or not self.index or self.index.ntotal == 0:
            logger.warning("RAG not initialized or empty - returning empty results")
            return []

        try:
            # Query Embedding
            query_embedding = self.embedder.encode([query])

            if _np:
                query_embedding = _np.array(query_embedding).astype("float32")

            # Suche
            distances, indices = self.index.search(query_embedding, k)

            # Ergebnisse zusammenstellen
            results = []
            for dist, idx in zip(distances[0], indices[0], strict=False):
                if idx < len(self.documents):
                    score = float(1 / (1 + dist))  # Convert distance to similarity

                    if score_threshold is None or score >= score_threshold:
                        results.append((self.documents[idx], score))

            return results

        except Exception as e:
            logger.error(f"Retrieval failed: {e}")
            return []

    def _save_index(self) -> None:
        """Speichert Index auf Disk"""
        if not self.index:
            return

        try:
            index_path = self.vector_store_path / "faiss.index"
            docs_path = self.vector_store_path / "documents.txt"
            meta_path = self.vector_store_path / "metadata.txt"

            if _faiss:
                _faiss.write_index(self.index, str(index_path))

            docs_path.write_text("\n---\n".join(self.documents), encoding="utf-8")
            meta_path.write_text("\n".join(self.metadata), encoding="utf-8")

            logger.info("Saved index to disk")

        except Exception as e:
            logger.error(f"Failed to save index: {e}")

    def index_directory(self, directory: str, file_extensions: list[str] | None = None) -> int:
        """
        Indexiert alle Dateien in einem Verzeichnis

        Args:
            directory: Pfad zum Verzeichnis
            file_extensions: Liste erlaubter Extensions (z.B. ['.txt', '.md'])

        Returns:
            Anzahl indexierter Dateien
        """
        dir_path = Path(directory)
        if not dir_path.exists():
            logger.error(f"Directory not found: {directory}")
            return 0

        file_extensions = file_extensions or [".txt", ".md", ".pdf"]

        documents = []
        metadata = []

        for file_path in dir_path.rglob("*"):
            if file_path.is_file() and file_path.suffix in file_extensions:
                try:
                    if file_path.suffix == ".pdf":
                        text = self._extract_pdf_text(file_path)
                    else:
                        text = file_path.read_text(encoding="utf-8", errors="ignore")

                    if text.strip():
                        documents.append(text)
                        metadata.append(str(file_path.relative_to(dir_path)))

                except Exception as e:
                    logger.warning(f"Failed to read {file_path}: {e}")

        logger.info(f"Found {len(documents)} documents in {directory}")
        return self.add_documents(documents, metadata)

    def _extract_pdf_text(self, pdf_path: Path) -> str:
        """Extrahiert Text aus PDF (benötigt pypdf)"""
        try:
            from pypdf import PdfReader

            reader = PdfReader(str(pdf_path))
            text = ""
            for page in reader.pages:
                text += page.extract_text() + "\n"
            return text
        except ImportError:
            logger.warning("pypdf not installed - cannot extract PDF text")
            return ""
        except Exception as e:
            logger.error(f"PDF extraction failed: {e}")
            return ""

    def get_stats(self) -> dict:
        """Liefert Statistiken zum Index"""
        total_vectors = 0
        if self.index is not None:
            total_vectors = self.index.ntotal
        return {
            "total_documents": len(self.documents),
            "total_vectors": total_vectors,
            "embedding_model": self.embedding_model_name,
            "dimension": self.dimension if self.embedder else 0,
        }


# CLI für Testing
if __name__ == "__main__":
    rag = RAGManager()

    # Demo: Dokumente hinzufügen
    demo_docs = [
        "CNC-Maschinen sollten bei Temperaturen über 60°C gestoppt werden.",
        "Hydraulikpressen benötigen regelmäßige Wartung der Dichtungen.",
        "Vibrationswerte über 2 mm/s deuten auf Lagerschäden hin.",
        "Die Kühlflüssigkeit sollte monatlich überprüft werden.",
    ]

    rag.add_documents(demo_docs, metadata=["doc1", "doc2", "doc3", "doc4"])

    # Demo: Retrieval
    results = rag.retrieve("Was tun bei hoher Temperatur?", k=3)
    print("\nRetrieval Results:")
    for doc, score in results:
        print(f"Score: {score:.3f} - {doc[:100]}")

    print(f"\nStats: {rag.get_stats()}")
