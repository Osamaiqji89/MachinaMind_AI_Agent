"""
Data Agent
Verantwortlich für Datenabruf, Validation und Präprozessierung
"""

from datetime import datetime, timedelta
from typing import Any

from database.db_handler import DatabaseHandler
from loguru import logger


class DataAgent:
    """Agent für Daten-Operationen"""

    def __init__(self, db_handler: DatabaseHandler):
        self.db = db_handler

    def get_machine_context(
        self, machine_id: int, lookback_minutes: int = 60
    ) -> dict[str, Any] | None:
        """
        Sammelt vollständigen Kontext für eine Maschine

        Returns:
            Dict mit machine, measurements, events, stats
        """
        machine = self.db.get_machine(machine_id)
        if not machine:
            logger.warning(f"Machine {machine_id} not found")
            return None

        # Zeitfenster
        end_time = datetime.now()
        start_time = end_time - timedelta(minutes=lookback_minutes)

        # Messungen
        measurements = self.db.get_measurements(
            machine_id, start_time=start_time, end_time=end_time, limit=1000
        )

        # Events
        events = self.db.get_events(machine_id=machine_id, limit=50)

        # Statistiken berechnen
        stats = self._compute_stats(measurements)

        return {
            "machine": machine,
            "measurements": measurements,
            "events": events,
            "stats": stats,
            "time_range": {"start": start_time.isoformat(), "end": end_time.isoformat()},
        }

    def _compute_stats(self, measurements: list[dict]) -> dict[str, Any]:
        """Berechnet Basis-Statistiken für Messungen"""
        if not measurements:
            return {}

        # Gruppiere nach Sensor-Typ
        by_sensor = {}
        for m in measurements:
            sensor = m["sensor_type"]
            if sensor not in by_sensor:
                by_sensor[sensor] = []
            by_sensor[sensor].append(m["value"])

        # Berechne Stats
        stats = {}
        for sensor, values in by_sensor.items():
            if values:
                stats[sensor] = {
                    "count": len(values),
                    "min": min(values),
                    "max": max(values),
                    "mean": sum(values) / len(values),
                    "latest": values[0] if values else None,
                }

        return stats

    def get_all_machines_summary(self) -> list[dict[str, Any]]:
        """Holt Zusammenfassung aller Maschinen"""
        machines = self.db.get_all_machines()
        summaries = []

        for machine in machines:
            # Letzte Messung
            latest_measurement = self.db.get_measurements(machine["id"], limit=1)

            # Event-Count
            recent_events = self.db.get_events(machine_id=machine["id"], limit=10)
            critical_count = sum(1 for e in recent_events if e["level"] in ["ERROR", "CRITICAL"])

            summaries.append(
                {
                    "machine": machine,
                    "last_measurement": latest_measurement[0] if latest_measurement else None,
                    "critical_events": critical_count,
                    "total_events": len(recent_events),
                }
            )

        return summaries

    def validate_data_quality(self, machine_id: int) -> dict[str, Any]:
        """
        Prüft Datenqualität (fehlende Werte, Ausreißer, etc.)
        """
        context = self.get_machine_context(machine_id, lookback_minutes=120)
        if not context:
            return {"valid": False, "reason": "Machine not found"}

        measurements = context["measurements"]

        # Prüfungen
        issues = []

        # 1. Genug Daten?
        if len(measurements) < 10:
            issues.append(f"Zu wenig Daten: nur {len(measurements)} Messungen")

        # 2. Lücken in Zeitreihe?
        if len(measurements) >= 2:
            timestamps = [datetime.fromisoformat(m["timestamp"]) for m in measurements]
            timestamps.sort()
            gaps = []
            for i in range(len(timestamps) - 1):
                gap = (timestamps[i + 1] - timestamps[i]).total_seconds()
                if gap > 300:  # > 5 Minuten
                    gaps.append(gap)

            if gaps:
                issues.append(f"Zeitlücken gefunden: {len(gaps)} Lücken")

        # 3. Sensor-Coverage
        sensors = {m["sensor_type"] for m in measurements}
        expected_sensors = {"temperature", "vibration", "power_consumption"}  # Beispiel
        missing = expected_sensors - sensors
        if missing:
            issues.append(f"Fehlende Sensoren: {', '.join(missing)}")

        return {
            "valid": len(issues) == 0,
            "issues": issues,
            "measurement_count": len(measurements),
            "sensors_active": list(sensors),
        }
