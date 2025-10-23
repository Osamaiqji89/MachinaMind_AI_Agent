"""
PDF Indexierung für RAG
Lädt Wartungsprotokoll_industrielle_Maschinen.pdf und indiziert es
"""

from pathlib import Path
from loguru import logger
from rag_engine.rag_manager import RAGManager


def index_maintenance_pdf():
    """Indiziert das Wartungsprotokoll"""

    pdf_path = Path(__file__).parent / "Wartungsprotokoll_industrielle_Maschinen.pdf"

    if not pdf_path.exists():
        logger.error(f"PDF nicht gefunden: {pdf_path}")
        return

    logger.info(f"📄 Indexiere PDF: {pdf_path.name}")

    # RAG Manager initialisieren
    rag = RAGManager(
        embedding_model="sentence-transformers/all-MiniLM-L6-v2", vector_store_path="vector_store"
    )

    # PDF indizieren
    num_chunks = rag.index_directory(directory=str(pdf_path.parent), file_extensions=[".pdf"])

    # Statistiken
    stats = rag.get_stats()
    logger.info(f"✅ Indexierung abgeschlossen!")
    logger.info(f"   - Chunks erstellt: {num_chunks}")
    logger.info(f"   - Total Dokumente: {stats['total_documents']}")
    logger.info(f"   - Total Vektoren: {stats['total_vectors']}")
    logger.info(f"   - Embedding Model: {stats['embedding_model']}")
    logger.info(f"   - Vektor-Dimension: {stats['dimension']}")

    # Test-Suche
    logger.info("\n🔍 Test-Suche:")
    test_queries = [
        "Wie oft sollte eine CNC-Maschine gewartet werden?",
        "Was tun bei Überhitzung?",
        "Vibration zu hoch",
    ]

    for query in test_queries:
        results = rag.retrieve(query, k=2)
        logger.info(f"\nQuery: '{query}'")
        for doc, score in results:
            logger.info(f"  Score: {score:.3f} - {doc[:120]}...")


if __name__ == "__main__":
    index_maintenance_pdf()
