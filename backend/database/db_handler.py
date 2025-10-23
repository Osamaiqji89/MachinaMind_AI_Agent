"""
Database Handler für MachinaMindAIAgent
Verwaltet Schema, Connections und CRUD Operationen für Maschinendaten
"""

import sqlite3
from contextlib import contextmanager
from datetime import datetime
from pathlib import Path
from typing import Any

from loguru import logger


class DatabaseHandler:
    """Zentrale DB-Verwaltung mit SQLite (erweiterbar für PostgreSQL)"""

    def __init__(self, db_path: str = "MachinaData.db"):
        self.db_path = Path(db_path)
        self._ensure_db_exists()

    def _ensure_db_exists(self) -> None:
        """Erstellt DB-Datei falls nicht vorhanden"""
        if not self.db_path.exists():
            logger.info(f"Creating new database at {self.db_path}")
            self._init_schema()

    @contextmanager
    def get_connection(self):
        """Context Manager für sichere DB-Connections"""
        conn = sqlite3.connect(self.db_path)
        conn.row_factory = sqlite3.Row  # Dict-like access
        try:
            yield conn
            conn.commit()
        except Exception as e:
            conn.rollback()
            logger.error(f"Database error: {e}")
            raise
        finally:
            conn.close()

    def _init_schema(self) -> None:
        """Initialisiert DB-Schema"""
        with self.get_connection() as conn:
            cursor = conn.cursor()

            # Machines Table
            cursor.execute(
                """
                CREATE TABLE IF NOT EXISTS machines (
                    id INTEGER PRIMARY KEY AUTOINCREMENT,
                    name TEXT NOT NULL UNIQUE,
                    type TEXT NOT NULL,
                    location TEXT,
                    meta_json TEXT,
                    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
                    updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
                )
            """
            )

            # Measurements Table (Sensor-Daten)
            cursor.execute(
                """
                CREATE TABLE IF NOT EXISTS measurements (
                    id INTEGER PRIMARY KEY AUTOINCREMENT,
                    machine_id INTEGER NOT NULL,
                    timestamp TIMESTAMP NOT NULL,
                    sensor_type TEXT NOT NULL,
                    value REAL NOT NULL,
                    unit TEXT,
                    FOREIGN KEY (machine_id) REFERENCES machines(id) ON DELETE CASCADE
                )
            """
            )
            cursor.execute(
                "CREATE INDEX IF NOT EXISTS idx_measurements_machine_time "
                "ON measurements(machine_id, timestamp DESC)"
            )

            # Events Table (Warnungen, Fehler)
            cursor.execute(
                """
                CREATE TABLE IF NOT EXISTS events (
                    id INTEGER PRIMARY KEY AUTOINCREMENT,
                    machine_id INTEGER NOT NULL,
                    timestamp TIMESTAMP NOT NULL,
                    level TEXT NOT NULL CHECK(level IN ('INFO', 'WARNING', 'ERROR', 'CRITICAL')),
                    message TEXT NOT NULL,
                    details_json TEXT,
                    FOREIGN KEY (machine_id) REFERENCES machines(id) ON DELETE CASCADE
                )
            """
            )
            cursor.execute(
                "CREATE INDEX IF NOT EXISTS idx_events_machine_time "
                "ON events(machine_id, timestamp DESC)"
            )

            # Reports Table (AI-generierte Analysen)
            cursor.execute(
                """
                CREATE TABLE IF NOT EXISTS reports (
                    id INTEGER PRIMARY KEY AUTOINCREMENT,
                    machine_id INTEGER,
                    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
                    report_type TEXT NOT NULL,
                    report_text TEXT NOT NULL,
                    metadata_json TEXT,
                    FOREIGN KEY (machine_id) REFERENCES machines(id) ON DELETE SET NULL
                )
            """
            )

            logger.info("Database schema initialized successfully")

    # ====================== MACHINES ======================

    def add_machine(
        self,
        name: str,
        machine_type: str,
        location: str | None = None,
        meta: str | None = None,
    ) -> int:
        """Fügt neue Maschine hinzu"""
        with self.get_connection() as conn:
            cursor = conn.cursor()
            cursor.execute(
                "INSERT INTO machines (name, type, location, meta_json) VALUES (?, ?, ?, ?)",
                (name, machine_type, location, meta),
            )
            return cursor.lastrowid

    def get_machine(self, machine_id: int) -> dict[str, Any] | None:
        """Holt Maschine nach ID"""
        with self.get_connection() as conn:
            cursor = conn.cursor()
            cursor.execute("SELECT * FROM machines WHERE id = ?", (machine_id,))
            row = cursor.fetchone()
            return dict(row) if row else None

    def get_all_machines(self) -> list[dict[str, Any]]:
        """Holt alle Maschinen"""
        with self.get_connection() as conn:
            cursor = conn.cursor()
            cursor.execute("SELECT * FROM machines ORDER BY name")
            return [dict(row) for row in cursor.fetchall()]

    # ====================== MEASUREMENTS ======================

    def add_measurement(
        self,
        machine_id: int,
        sensor_type: str,
        value: float,
        unit: str | None = None,
        timestamp: datetime | None = None,
    ) -> int:
        """Fügt Messwert hinzu"""
        if timestamp is None:
            timestamp = datetime.now()

        with self.get_connection() as conn:
            cursor = conn.cursor()
            cursor.execute(
                "INSERT INTO measurements (machine_id, timestamp, sensor_type, value, unit) "
                "VALUES (?, ?, ?, ?, ?)",
                (machine_id, timestamp, sensor_type, value, unit),
            )
            return cursor.lastrowid

    def get_measurements(
        self,
        machine_id: int,
        sensor_type: str | None = None,
        limit: int = 100,
        start_time: datetime | None = None,
        end_time: datetime | None = None,
    ) -> list[dict[str, Any]]:
        """Holt Messwerte mit optionalen Filtern"""
        query = "SELECT * FROM measurements WHERE machine_id = ?"
        params: list[Any] = [machine_id]

        if sensor_type:
            query += " AND sensor_type = ?"
            params.append(sensor_type)
        if start_time:
            query += " AND timestamp >= ?"
            params.append(start_time)
        if end_time:
            query += " AND timestamp <= ?"
            params.append(end_time)

        query += " ORDER BY timestamp DESC LIMIT ?"
        params.append(limit)

        with self.get_connection() as conn:
            cursor = conn.cursor()
            cursor.execute(query, params)
            return [dict(row) for row in cursor.fetchall()]

    def get_latest_measurement(self, machine_id: int, sensor_type: str) -> dict[str, Any] | None:
        """Holt aktuellsten Messwert für Sensor"""
        measurements = self.get_measurements(machine_id, sensor_type, limit=1)
        return measurements[0] if measurements else None

    # ====================== EVENTS ======================

    def add_event(
        self,
        machine_id: int,
        level: str,
        message: str,
        details: str | None = None,
        timestamp: datetime | None = None,
    ) -> int:
        """Fügt Event hinzu"""
        if timestamp is None:
            timestamp = datetime.now()

        with self.get_connection() as conn:
            cursor = conn.cursor()
            cursor.execute(
                "INSERT INTO events (machine_id, timestamp, level, message, details_json) "
                "VALUES (?, ?, ?, ?, ?)",
                (machine_id, timestamp, level.upper(), message, details),
            )
            return cursor.lastrowid

    def get_events(
        self,
        machine_id: int | None = None,
        level: str | None = None,
        limit: int = 50,
    ) -> list[dict[str, Any]]:
        """Holt Events mit optionalen Filtern"""
        query = "SELECT * FROM events WHERE 1=1"
        params: list[Any] = []

        if machine_id:
            query += " AND machine_id = ?"
            params.append(machine_id)
        if level:
            query += " AND level = ?"
            params.append(level.upper())

        query += " ORDER BY timestamp DESC LIMIT ?"
        params.append(limit)

        with self.get_connection() as conn:
            cursor = conn.cursor()
            cursor.execute(query, params)
            return [dict(row) for row in cursor.fetchall()]

    # ====================== REPORTS ======================

    def add_report(
        self,
        report_type: str,
        report_text: str,
        machine_id: int | None = None,
        metadata: str | None = None,
    ) -> int:
        """Fügt AI-generierten Report hinzu"""
        with self.get_connection() as conn:
            cursor = conn.cursor()
            cursor.execute(
                "INSERT INTO reports (machine_id, report_type, report_text, metadata_json) "
                "VALUES (?, ?, ?, ?)",
                (machine_id, report_type, report_text, metadata),
            )
            return cursor.lastrowid

    def get_reports(self, machine_id: int | None = None, limit: int = 20) -> list[dict[str, Any]]:
        """Holt Reports"""
        query = "SELECT * FROM reports"
        params: list[Any] = []

        if machine_id:
            query += " WHERE machine_id = ?"
            params.append(machine_id)

        query += " ORDER BY created_at DESC LIMIT ?"
        params.append(limit)

        with self.get_connection() as conn:
            cursor = conn.cursor()
            cursor.execute(query, params)
            return [dict(row) for row in cursor.fetchall()]

    # ====================== STATS & UTILITIES ======================

    def get_stats(self) -> dict[str, Any]:
        """Holt DB-Statistiken"""
        with self.get_connection() as conn:
            cursor = conn.cursor()

            cursor.execute("SELECT COUNT(*) FROM machines")
            machine_count = cursor.fetchone()[0]

            cursor.execute("SELECT COUNT(*) FROM measurements")
            measurement_count = cursor.fetchone()[0]

            cursor.execute("SELECT COUNT(*) FROM events")
            event_count = cursor.fetchone()[0]

            cursor.execute("SELECT COUNT(*) FROM reports")
            report_count = cursor.fetchone()[0]

            return {
                "machines": machine_count,
                "measurements": measurement_count,
                "events": event_count,
                "reports": report_count,
            }


# CLI für DB-Init
if __name__ == "__main__":
    import sys

    db = DatabaseHandler("MachinaData.db")

    if "--init" in sys.argv:
        logger.info("Database initialized")
        logger.info(f"Stats: {db.get_stats()}")

    elif "--demo" in sys.argv:
        # Demo-Daten
        machine_id = db.add_machine("CNC-Mill-01", "CNC", "Hall A")
        db.add_measurement(machine_id, "temperature", 42.5, "°C")
        db.add_measurement(machine_id, "vibration", 0.8, "mm/s")
        db.add_event(machine_id, "WARNING", "Temperature above threshold")
        logger.info(f"Demo data created. Stats: {db.get_stats()}")
