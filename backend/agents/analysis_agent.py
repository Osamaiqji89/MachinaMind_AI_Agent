"""
Analysis Agent
Anomalieerkennung mit statistischen Methoden (IsolationForest, Z-Score)
"""

from datetime import datetime, timedelta
from typing import Any, Dict, List, Optional

import numpy as np
from loguru import logger

# Lazy import to avoid Windows DLL issues
_IsolationForest = None


def _ensure_sklearn():
    """Lazy load sklearn"""
    global _IsolationForest
    if _IsolationForest is None:
        try:
            from sklearn.ensemble import IsolationForest as IF

            _IsolationForest = IF
            return True
        except Exception as e:
            logger.warning(f"scikit-learn not available: {e}")
            return False
    return _IsolationForest is not None


from database.db_handler import DatabaseHandler


class AnalysisAgent:
    """Agent für Datenanalyse und Anomalieerkennung"""

    def __init__(self, db_handler: DatabaseHandler):
        self.db = db_handler

    async def analyze(
        self,
        machine_id: int,
        sensor_type: Optional[str] = None,
        time_range_minutes: int = 60,
    ) -> Dict[str, Any]:
        """
        Hauptanalyse-Methode

        Returns:
            Dict mit anomalies_detected, summary, details
        """
        # Daten laden - zunächst ohne Zeitfilter versuchen
        measurements = self.db.get_measurements(
            machine_id,
            sensor_type=sensor_type,
            limit=1000,
        )

        # Falls keine Daten, gib hilfreiche Meldung zurück
        if len(measurements) == 0:
            return {
                "anomalies_detected": 0,
                "summary": "Keine Messdaten in der Datenbank. Starten Sie den Simulator: python data_simulator.py",
                "details": [],
            }

        # Falls genug Daten vorhanden, filtere nach Zeitraum (optional)
        if time_range_minutes > 0 and len(measurements) > 0:
            # Nimm die neuesten N Einträge basierend auf time_range
            # Als Approximation: Nehme die letzten X% der Daten
            from_index = max(0, len(measurements) - (time_range_minutes * 2))
            measurements = measurements[from_index:]

        if len(measurements) < 10:
            return {
                "anomalies_detected": 0,
                "summary": f"Nur {len(measurements)} Messwerte verfügbar. Mindestens 10 benötigt für Analyse.",
                "details": [],
            }

        # Anomalien erkennen
        anomalies = []

        # Gruppiere nach Sensor
        by_sensor = self._group_by_sensor(measurements)

        for sensor, data in by_sensor.items():
            sensor_anomalies = self._detect_anomalies(sensor, data)
            anomalies.extend(sensor_anomalies)

        # Zusammenfassung
        summary = self._create_summary(anomalies, measurements)

        return {"anomalies_detected": len(anomalies), "summary": summary, "details": anomalies}

    def _group_by_sensor(self, measurements: List[Dict]) -> Dict[str, List[Dict]]:
        """Gruppiert Messungen nach Sensor-Typ"""
        by_sensor = {}
        for m in measurements:
            sensor = m["sensor_type"]
            if sensor not in by_sensor:
                by_sensor[sensor] = []
            by_sensor[sensor].append(m)
        return by_sensor

    def _detect_anomalies(self, sensor_name: str, data: List[Dict]) -> List[Dict]:
        """
        Anomalieerkennung mit mehreren Methoden

        1. Z-Score (statistische Abweichung)
        2. IsolationForest (ML-basiert)
        """
        if len(data) < 5:
            return []

        values = np.array([d["value"] for d in data])
        timestamps = [d["timestamp"] for d in data]

        anomalies = []

        # Methode 1: Z-Score
        mean = np.mean(values)
        std = np.std(values)

        if std > 0:
            z_scores = np.abs((values - mean) / std)

            for i, (z, val, ts) in enumerate(zip(z_scores, values, timestamps)):
                if z > 3.0:  # 3-Sigma-Regel
                    anomalies.append(
                        {
                            "sensor": sensor_name,
                            "timestamp": ts,
                            "value": float(val),
                            "deviation": float(z),
                            "method": "z-score",
                            "severity": self._classify_severity(z),
                        }
                    )

        # Methode 2: IsolationForest (wenn genug Daten)
        if len(values) >= 20 and _ensure_sklearn():
            try:
                iso_forest = _IsolationForest(contamination=0.1, random_state=42)
                predictions = iso_forest.fit_predict(values.reshape(-1, 1))

                for i, (pred, val, ts) in enumerate(zip(predictions, values, timestamps)):
                    if pred == -1:  # Anomalie
                        # Nur hinzufügen wenn nicht schon von Z-Score erkannt
                        existing = any(a["timestamp"] == ts for a in anomalies)
                        if not existing:
                            anomalies.append(
                                {
                                    "sensor": sensor_name,
                                    "timestamp": ts,
                                    "value": float(val),
                                    "method": "isolation-forest",
                                    "severity": "MEDIUM",
                                }
                            )
            except Exception as e:
                logger.warning(f"IsolationForest failed for {sensor_name}: {e}")

        return anomalies

    def _classify_severity(self, z_score: float) -> str:
        """Klassifiziert Schweregrad basierend auf Z-Score"""
        if z_score > 5.0:
            return "CRITICAL"
        elif z_score > 4.0:
            return "HIGH"
        elif z_score > 3.0:
            return "MEDIUM"
        else:
            return "LOW"

    def _create_summary(self, anomalies: List[Dict], measurements: List[Dict]) -> str:
        """Erstellt Zusammenfassung der Analyse"""
        if not anomalies:
            return f"Keine Anomalien gefunden in {len(measurements)} Messungen"

        # Gruppiere nach Sensor
        by_sensor = {}
        for a in anomalies:
            sensor = a["sensor"]
            if sensor not in by_sensor:
                by_sensor[sensor] = []
            by_sensor[sensor].append(a)

        # Schweregrad-Counts
        severity_counts = {"CRITICAL": 0, "HIGH": 0, "MEDIUM": 0, "LOW": 0}
        for a in anomalies:
            severity = a.get("severity", "LOW")
            severity_counts[severity] += 1

        summary_parts = [f"🚨 {len(anomalies)} Anomalien gefunden:"]

        for sensor, sensor_anomalies in by_sensor.items():
            summary_parts.append(f"  - {sensor}: {len(sensor_anomalies)} Auffälligkeiten")

        if severity_counts["CRITICAL"] > 0:
            summary_parts.append(f"  ⛔ CRITICAL: {severity_counts['CRITICAL']}")
        if severity_counts["HIGH"] > 0:
            summary_parts.append(f"  🔴 HIGH: {severity_counts['HIGH']}")
        if severity_counts["MEDIUM"] > 0:
            summary_parts.append(f"  🟠 MEDIUM: {severity_counts['MEDIUM']}")

        return "\n".join(summary_parts)
