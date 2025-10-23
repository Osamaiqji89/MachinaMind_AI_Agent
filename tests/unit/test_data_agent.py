"""
Unit Tests für DataAgent
"""

import pytest
from datetime import datetime, timedelta
from backend.database.db_handler import DatabaseHandler
from backend.agents.data_agent import DataAgent


@pytest.fixture
def db():
    """Test-Datenbank"""
    db = DatabaseHandler(":memory:")  # In-Memory DB für Tests
    
    # Demo-Maschine erstellen
    db.add_machine("Test-CNC", "CNC", "Test Hall")
    
    yield db


@pytest.fixture
def agent(db):
    """DataAgent mit Test-DB"""
    return DataAgent(db)


def test_get_machine_context(agent, db):
    """Test: Maschinen-Kontext sammeln"""
    # Arrange
    machine_id = 1
    db.add_measurement(machine_id, "temperature", 42.5, "°C")
    db.add_event(machine_id, "INFO", "Test event")
    
    # Act
    context = agent.get_machine_context(machine_id)
    
    # Assert
    assert context is not None
    assert context["machine"]["id"] == machine_id
    assert len(context["measurements"]) > 0
    assert len(context["events"]) > 0


def test_get_machine_context_not_found(agent):
    """Test: Nicht existierende Maschine"""
    context = agent.get_machine_context(999)
    assert context is None


def test_validate_data_quality_insufficient_data(agent, db):
    """Test: Datenqualität bei zu wenig Daten"""
    result = agent.validate_data_quality(1)
    
    assert result["valid"] is False
    assert "Zu wenig Daten" in result["issues"][0]


def test_get_all_machines_summary(agent, db):
    """Test: Zusammenfassung aller Maschinen"""
    db.add_machine("Test-Press", "Press", "Test Hall")
    
    summaries = agent.get_all_machines_summary()
    
    assert len(summaries) >= 2
    assert all("machine" in s for s in summaries)
