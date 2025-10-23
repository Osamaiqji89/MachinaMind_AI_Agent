"""
Integration Tests für API Endpoints
"""

import pytest
from fastapi.testclient import TestClient
from backend.api.main import app


@pytest.fixture
def client():
    """Test Client"""
    return TestClient(app)


def test_health_check(client):
    """Test: Health Endpoint"""
    response = client.get("/health")
    
    assert response.status_code == 200
    data = response.json()
    assert "status" in data
    assert data["status"] == "healthy"


def test_get_machines(client):
    """Test: Maschinen abrufen"""
    response = client.get("/machines")
    
    assert response.status_code == 200
    data = response.json()
    assert isinstance(data, list)


def test_chat_endpoint(client):
    """Test: Chat Endpoint"""
    payload = {
        "message": "Test question",
        "context_limit": 10
    }
    
    response = client.post("/chat", json=payload)
    
    assert response.status_code == 200
    data = response.json()
    assert "answer" in data
    assert "timestamp" in data


def test_analyze_endpoint_no_machine(client):
    """Test: Analyse ohne Maschine"""
    payload = {
        "machine_id": 999,
        "time_range_minutes": 60
    }
    
    response = client.post("/analyze", json=payload)
    
    # Sollte 404 zurückgeben (Maschine nicht gefunden)
    assert response.status_code == 404


def test_get_events_with_filters(client):
    """Test: Events mit Filtern"""
    response = client.get("/events?level=ERROR&limit=10")
    
    assert response.status_code == 200
    data = response.json()
    assert isinstance(data, list)
