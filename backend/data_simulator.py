"""
Maschinendaten-Simulator
Generiert realistische Sensordaten für Testing & Demo
"""

import json
import random
import time
from dataclasses import dataclass

from database.db_handler import DatabaseHandler
from loguru import logger


@dataclass
class SensorConfig:
    """Konfiguration für einen Sensor"""

    name: str
    unit: str
    min_value: float
    max_value: float
    normal_mean: float
    normal_std: float
    anomaly_probability: float = 0.05


class MachineSimulator:
    """Simuliert eine Maschine mit mehreren Sensoren"""

    SENSOR_CONFIGS = {
        "CNC": [
            SensorConfig("temperature", "°C", 15, 90, 45, 8, 0.03),
            SensorConfig("vibration", "mm/s", 0, 5, 0.5, 0.3, 0.08),
            SensorConfig("spindle_speed", "RPM", 0, 12000, 6000, 1500, 0.02),
            SensorConfig("power_consumption", "kW", 0, 50, 25, 8, 0.04),
        ],
        "Press": [
            SensorConfig("pressure", "bar", 0, 500, 200, 40, 0.06),
            SensorConfig("temperature", "°C", 20, 100, 60, 12, 0.04),
            SensorConfig("cycle_time", "s", 5, 30, 15, 3, 0.03),
            SensorConfig("force", "kN", 0, 1000, 400, 80, 0.05),
        ],
        "Conveyor": [
            SensorConfig("speed", "m/min", 0, 100, 50, 10, 0.02),
            SensorConfig("temperature", "°C", 20, 80, 40, 8, 0.03),
            SensorConfig("load", "kg", 0, 500, 200, 50, 0.04),
        ],
    }

    def __init__(self, machine_id: int, machine_type: str):
        self.machine_id = machine_id
        self.machine_type = machine_type
        self.sensors = self.SENSOR_CONFIGS.get(machine_type, [])
        self._anomaly_active = False
        self._anomaly_counter = 0

    def generate_reading(self, sensor: SensorConfig) -> float:
        """Generiert einen Messwert (normal oder Anomalie)"""
        # Gelegentlich Anomalie erzeugen
        if random.random() < sensor.anomaly_probability:
            self._trigger_anomaly()

        if self._anomaly_active:
            # Anomalie: Wert außerhalb Normalbereich
            if random.random() < 0.5:
                value = sensor.normal_mean + random.uniform(3, 5) * sensor.normal_std
            else:
                value = sensor.normal_mean - random.uniform(3, 5) * sensor.normal_std

            value = max(sensor.min_value, min(sensor.max_value, value))
        else:
            # Normal: Gauss-Verteilung um Mittelwert
            value = random.gauss(sensor.normal_mean, sensor.normal_std)
            value = max(sensor.min_value, min(sensor.max_value, value))

        return round(value, 2)

    def _trigger_anomaly(self) -> None:
        """Startet Anomalie-Sequenz"""
        self._anomaly_active = True
        self._anomaly_counter = random.randint(3, 10)  # Anomalie für 3-10 Messungen

    def step(self) -> dict[str, float]:
        """Führt einen Simulations-Schritt aus (alle Sensoren)"""
        readings = {}
        for sensor in self.sensors:
            readings[sensor.name] = self.generate_reading(sensor)

        # Anomalie-Counter verringern
        if self._anomaly_active:
            self._anomaly_counter -= 1
            if self._anomaly_counter <= 0:
                self._anomaly_active = False

        return readings


class DataSimulator:
    """Hauptsimulator für mehrere Maschinen"""

    def __init__(self, db_handler: DatabaseHandler, interval: float = 1.0):
        self.db = db_handler
        self.interval = interval
        self.machines: list[MachineSimulator] = []
        self._running = False

    def add_machine(self, machine_id: int, machine_type: str) -> None:
        """Fügt Maschine zur Simulation hinzu"""
        simulator = MachineSimulator(machine_id, machine_type)
        self.machines.append(simulator)
        logger.info(f"Added machine {machine_id} ({machine_type}) to simulator")

    def _check_anomaly(self, machine: MachineSimulator, readings: dict[str, float]) -> None:
        """Prüft auf Anomalien und loggt Events"""
        for sensor in machine.sensors:
            value = readings.get(sensor.name, 0)

            # Schwellwert-basierte Event-Generierung
            deviation = abs(value - sensor.normal_mean) / sensor.normal_std

            if deviation > 3.0:
                self.db.add_event(
                    machine.machine_id,
                    "CRITICAL",
                    f"{sensor.name} critical: {value} {sensor.unit} (deviation: {deviation:.1f}σ)",
                    json.dumps({"sensor": sensor.name, "value": value, "deviation": deviation}),
                )
            elif deviation > 2.0:
                self.db.add_event(
                    machine.machine_id,
                    "WARNING",
                    f"{sensor.name} warning: {value} {sensor.unit} (deviation: {deviation:.1f}σ)",
                    json.dumps({"sensor": sensor.name, "value": value, "deviation": deviation}),
                )

    def run_once(self) -> None:
        """Führt einen Simulations-Zyklus aus"""
        for machine in self.machines:
            readings = machine.step()

            # Messwerte in DB speichern
            for sensor in machine.sensors:
                if sensor.name in readings:
                    self.db.add_measurement(
                        machine.machine_id, sensor.name, readings[sensor.name], sensor.unit
                    )

            # Auf Anomalien prüfen
            self._check_anomaly(machine, readings)

    def run(self, duration: int = 0) -> None:
        """Startet kontinuierliche Simulation"""
        self._running = True
        logger.info(
            f"Starting simulation ({len(self.machines)} machines, interval={self.interval}s)"
        )

        start_time = time.time()
        cycles = 0

        try:
            while self._running:
                self.run_once()
                cycles += 1

                if duration > 0 and (time.time() - start_time) >= duration:
                    break

                time.sleep(self.interval)

        except KeyboardInterrupt:
            logger.info("Simulation stopped by user")
        finally:
            self._running = False
            logger.info(
                f"Simulation ended: {cycles} cycles, {time.time() - start_time:.1f}s elapsed"
            )

    def stop(self) -> None:
        """Stoppt Simulation"""
        self._running = False


# CLI
if __name__ == "__main__":
    import sys

    # Setup
    db = DatabaseHandler("MachinaData.db")
    simulator = DataSimulator(db, interval=2.0)

    # RAG-Indizierung beim Start (falls PDFs vorhanden)
    try:
        from pathlib import Path

        from rag_engine.rag_manager import RAGManager

        pdf_files = list(Path(__file__).parent.glob("*.pdf"))
        if pdf_files:
            logger.info(f"📄 Found {len(pdf_files)} PDF(s) for RAG...")

            # Prüfe ob bereits indiziert
            vector_store_path = Path(__file__).parent / "vector_store"
            index_exists = (vector_store_path / "faiss.index").exists()

            if index_exists and "--reindex" not in sys.argv:
                logger.info("ℹ️  Vector store exists. Use --reindex to rebuild.")
                logger.info("✅ RAG ready (use existing index)")
            else:
                # Neu indizieren (überschreibt alten Index)
                logger.info("🔄 Indexing PDFs for RAG...")
                rag = RAGManager(vector_store_path="vector_store")

                # Alten Index löschen
                if index_exists:
                    import shutil

                    shutil.rmtree(vector_store_path)
                    logger.info("🗑️  Deleted old index")
                    rag = RAGManager(vector_store_path="vector_store")

                num_indexed = rag.index_directory(
                    directory=str(Path(__file__).parent), file_extensions=[".pdf"]
                )

                if num_indexed > 0:
                    logger.info(f"✅ RAG indexed {num_indexed} document chunks")
                else:
                    logger.warning("⚠️  No documents indexed - RAG may not work")
        else:
            logger.info("ℹ️  No PDFs found, skipping RAG indexing")
    except Exception as e:
        logger.warning(f"⚠️  RAG indexing failed: {e}")
        logger.info("Continuing without RAG support...")

    # Demo-Maschinen erstellen (falls nicht vorhanden)
    machines = db.get_all_machines()
    if not machines:
        logger.info("Creating demo machines...")
        m1 = db.add_machine("CNC-Mill-01", "CNC", "Hall A")
        m2 = db.add_machine("Hydraulic-Press-02", "Press", "Hall B")
        m3 = db.add_machine("Conveyor-Belt-03", "Conveyor", "Hall A")
        machines = [{"id": m1}, {"id": m2}, {"id": m3}]

    # Maschinen zur Simulation hinzufügen
    for m in machines:
        machine_data = db.get_machine(m["id"])
        if machine_data:
            simulator.add_machine(machine_data["id"], machine_data["type"])

    # Simulation starten
    if "--once" in sys.argv:
        simulator.run_once()
        logger.info(f"Single cycle complete. Stats: {db.get_stats()}")
    elif "--demo" in sys.argv:
        # Demo mode: run for 60 seconds with live data
        logger.info("Starting demo mode (60 seconds)...")
        simulator.run(60)
    else:
        # Parse duration from command line
        duration = 0
        for arg in sys.argv[1:]:
            if arg.isdigit():
                duration = int(arg)
                break
        simulator.run(duration)
