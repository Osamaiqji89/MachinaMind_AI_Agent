"""
FastAPI Backend für MachinaMindAIAgent
REST API für Frontend-Kommunikation, Agenten-Orchestrierung und Chat
"""

import os
import sys
from contextlib import asynccontextmanager
from datetime import datetime
from pathlib import Path
from typing import Any

# Add parent directory to path for imports
backend_dir = Path(__file__).parent.parent
if str(backend_dir) not in sys.path:
    sys.path.insert(0, str(backend_dir))

from database.db_handler import DatabaseHandler
from fastapi import FastAPI, HTTPException, Query
from fastapi.middleware.cors import CORSMiddleware
from loguru import logger
from pydantic import BaseModel, Field

# Lazy imports für agents (werden später implementiert)
try:
    from agents.analysis_agent import AnalysisAgent
    from agents.data_agent import DataAgent
    from agents.llm_agent import LLMAgent
except ImportError:
    logger.warning("Agent modules not yet available - running in limited mode")
    DataAgent = None  # type: ignore
    AnalysisAgent = None  # type: ignore
    LLMAgent = None  # type: ignore


# ==================== Pydantic Models ====================


class HealthResponse(BaseModel):
    status: str
    timestamp: str
    db_stats: dict[str, int]


class MachineResponse(BaseModel):
    id: int
    name: str
    type: str
    location: str | None = None


class MeasurementResponse(BaseModel):
    id: int
    machine_id: int
    timestamp: str
    sensor_type: str
    value: float
    unit: str | None = None


class EventResponse(BaseModel):
    id: int
    machine_id: int
    timestamp: str
    level: str
    message: str


class ChatRequest(BaseModel):
    message: str = Field(..., min_length=1, max_length=1000)
    machine_id: int | None = None
    context_limit: int = Field(default=10, ge=1, le=100)


class ChatResponse(BaseModel):
    answer: str
    sources: list[str] = []
    context_used: dict[str, Any] = {}
    timestamp: str


class AnalysisRequest(BaseModel):
    machine_id: int
    sensor_type: str | None = None
    time_range_minutes: int = Field(default=60, ge=1, le=1440)


class AnalysisResponse(BaseModel):
    machine_id: int
    anomalies_detected: int
    summary: str
    details: list[dict[str, Any]]
    timestamp: str


# ==================== Application Lifecycle ====================


@asynccontextmanager
async def lifespan(app: FastAPI):
    """Startup & Shutdown Logic"""
    # Startup
    logger.info("🚀 Starting MachinaMindAIAgent Backend...")
    app.state.db = DatabaseHandler("MachinaData.db")

    # Initialize agents (if available)
    if DataAgent:
        app.state.data_agent = DataAgent(app.state.db)
    if AnalysisAgent:
        app.state.analysis_agent = AnalysisAgent(app.state.db)
    if LLMAgent:
        app.state.llm_agent = LLMAgent()

    logger.info("✅ Backend ready")

    yield

    # Shutdown
    logger.info("🛑 Shutting down MachinaMindAIAgent Backend...")


# ==================== FastAPI App ====================

app = FastAPI(
    title="MachinaMindAIAgent API",
    description="Industrial Machine Intelligence Backend",
    version="1.0.0",
    lifespan=lifespan,
)

# CORS für Frontend
app.add_middleware(
    CORSMiddleware,
    allow_origins=["*"],  # Produktiv: spezifische Origins
    allow_credentials=True,
    allow_methods=["*"],
    allow_headers=["*"],
)


# ==================== Endpoints ====================


@app.get("/", tags=["Root"])
async def root():
    """API Root"""
    return {
        "service": "MachinaMindAIAgent API",
        "version": "1.0.0",
        "docs": "/docs",
        "health": "/health",
    }


@app.get("/health", response_model=HealthResponse, tags=["Health"])
async def health_check():
    """Health Check mit DB-Status"""
    db: DatabaseHandler = app.state.db
    stats = db.get_stats()

    return HealthResponse(
        status="healthy",
        timestamp=datetime.now().isoformat(),
        db_stats=stats,
    )


# ==================== Machines ====================


@app.get("/machines", response_model=list[MachineResponse], tags=["Machines"])
async def get_machines():
    """Alle Maschinen abrufen"""
    db: DatabaseHandler = app.state.db
    machines = db.get_all_machines()
    return machines


@app.get("/machines/{machine_id}", response_model=MachineResponse, tags=["Machines"])
async def get_machine(machine_id: int):
    """Einzelne Maschine abrufen"""
    db: DatabaseHandler = app.state.db
    machine = db.get_machine(machine_id)

    if not machine:
        raise HTTPException(status_code=404, detail="Machine not found")

    return machine


# ==================== Measurements ====================


@app.get("/measurements/{machine_id}", response_model=list[MeasurementResponse], tags=["Data"])
async def get_measurements(
    machine_id: int,
    sensor_type: str | None = Query(None),
    limit: int = Query(100, ge=1, le=1000),
):
    """Messwerte für Maschine abrufen"""
    db: DatabaseHandler = app.state.db

    # Prüfe ob Maschine existiert
    if not db.get_machine(machine_id):
        raise HTTPException(status_code=404, detail="Machine not found")

    measurements = db.get_measurements(machine_id, sensor_type, limit)
    return measurements


# ==================== Events ====================


@app.get("/events", response_model=list[EventResponse], tags=["Events"])
async def get_events(
    machine_id: int | None = Query(None),
    level: str | None = Query(None),
    limit: int = Query(50, ge=1, le=500),
):
    """Events abrufen"""
    db: DatabaseHandler = app.state.db
    events = db.get_events(machine_id, level, limit)
    return events


# ==================== Chat ====================


@app.post("/chat", response_model=ChatResponse, tags=["AI"])
async def chat(request: ChatRequest):
    """
    Chat mit AI-Agent
    Kombiniert DB-Kontext, RAG und LLM für intelligente Antworten
    """
    db: DatabaseHandler = app.state.db

    # Kontext sammeln
    context = {}

    # Maschinen-spezifischer Kontext
    if request.machine_id:
        machine = db.get_machine(request.machine_id)
        if machine:
            context["machine"] = machine
            context["recent_measurements"] = db.get_measurements(
                request.machine_id, limit=request.context_limit
            )
            context["recent_events"] = db.get_events(
                machine_id=request.machine_id, limit=request.context_limit
            )
    else:
        context["machines"] = db.get_all_machines()
        context["recent_events"] = db.get_events(limit=request.context_limit)

    # LLM Query mit RAG-Unterstützung
    if LLMAgent and hasattr(app.state, "llm_agent"):
        llm: LLMAgent = app.state.llm_agent
        answer, sources = await llm.query(request.message, context)
    else:
        # Fallback ohne LLM
        answer = f"[Demo Mode] Ihre Frage: '{request.message}'. Kontext: {len(context)} Elemente."
        sources = []

    return ChatResponse(
        answer=answer,
        sources=sources,
        context_used={"items": len(context)},
        timestamp=datetime.now().isoformat(),
    )


# ==================== Analysis ====================


@app.post("/analyze", response_model=AnalysisResponse, tags=["AI"])
async def analyze_machine(request: AnalysisRequest):
    """
    Maschinen-Analyse
    Verwendet AnalysisAgent für Anomalieerkennung
    """
    db: DatabaseHandler = app.state.db

    # Prüfe ob Maschine existiert
    if not db.get_machine(request.machine_id):
        raise HTTPException(status_code=404, detail="Machine not found")

    # Analysis durchführen
    if AnalysisAgent and hasattr(app.state, "analysis_agent"):
        agent: AnalysisAgent = app.state.analysis_agent
        result = await agent.analyze(
            request.machine_id,
            sensor_type=request.sensor_type,
            time_range_minutes=request.time_range_minutes,
        )
    else:
        # Fallback ohne Agent
        measurements = db.get_measurements(request.machine_id, limit=100)
        result = {
            "anomalies_detected": 0,
            "summary": f"[Demo] Analysiert: {len(measurements)} Messwerte",
            "details": [],
        }

    return AnalysisResponse(
        machine_id=request.machine_id,
        anomalies_detected=result.get("anomalies_detected", 0),
        summary=result.get("summary", ""),
        details=result.get("details", []),
        timestamp=datetime.now().isoformat(),
    )


# ==================== Reports ====================


@app.get("/reports", tags=["Reports"])
async def get_reports(
    machine_id: int | None = Query(None), limit: int = Query(20, ge=1, le=100)
):
    """Reports abrufen"""
    db: DatabaseHandler = app.state.db
    reports = db.get_reports(machine_id, limit)
    return reports


@app.post("/reports", tags=["Reports"])
async def create_report(
    machine_id: int | None = None,
    report_type: str = "manual",
    report_text: str = "",
):
    """Neuen Report erstellen"""
    db: DatabaseHandler = app.state.db

    if not report_text:
        raise HTTPException(status_code=400, detail="report_text required")

    report_id = db.add_report(report_type, report_text, machine_id)

    return {"id": report_id, "status": "created"}


# ==================== Dev/Debug ====================

if __name__ == "__main__":
    import uvicorn

    reload_env = os.getenv("MACHINAMIND_RELOAD", "true").lower()
    reload_enabled = reload_env in {"1", "true", "yes", "on"}
    logger.info(f"✨ Uvicorn reload mode: {'ON' if reload_enabled else 'OFF'}")

    uvicorn.run(
        "main:app",
        host="0.0.0.0",
        port=8000,
        reload=reload_enabled,
        log_level="info",
    )
