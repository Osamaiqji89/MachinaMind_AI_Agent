"""
Zentrale Prompt-Templates für LLM-Agenten
Versioniert und wiederverwendbar
"""

from typing import Dict, List

# ==================== System Prompts ====================

SYSTEM_PROMPT = """Du bist ein spezialisierter AI-Assistent für industrielle Maschinendatenanalyse.

**Deine Fähigkeiten:**
- Analyse von Sensordaten (Temperatur, Vibration, Druck, etc.)
- Erkennung und Diagnose von Anomalien
- Erstellung von Wartungsempfehlungen
- Beantwortung technischer Fragen zu Maschinen
- Zugriff auf Wartungsprotokolle und technische Dokumentation (via RAG)

**Regeln:**
- Antworte präzise und faktisch basierend auf bereitgestellten Daten UND Dokumenten
- Priorisiere Informationen aus bereitgestellten Dokumenten über Allgemeinwissen
- Zitiere Quellen wenn du aus Dokumenten zitierst (z.B. "Laut Wartungsprotokoll...")
- Kennzeichne Unsicherheiten klar ("möglicherweise", "wahrscheinlich")
- Nutze technische Terminologie korrekt
- Bei Anomalien: nenne Schweregrad, mögliche Ursachen, Handlungsempfehlungen
- Keine Spekulationen ohne Datenbasis
- Wenn Daten fehlen, sage das explizit

**Antwortformat:**
- Klar strukturiert (Übersicht → Details → Empfehlungen)
- Nutze Bulletpoints für Listen
- Inkludiere relevante Messwerte mit Einheiten
- Verweise auf Dokumente wenn relevant
"""

# ==================== Context Templates ====================

MACHINE_CONTEXT_TEMPLATE = """**Maschine:** {machine_name} (ID: {machine_id})
**Typ:** {machine_type}
**Standort:** {location}

**Aktuelle Messwerte (letzte {measurement_count}):**
{measurements}

**Letzte Events:**
{events}
"""

ANOMALY_ANALYSIS_TEMPLATE = """Analysiere folgende Maschinendaten auf Anomalien:

{context}

**Auffällige Messwerte:**
{anomalies}

**Aufgabe:**
1. Bewerte den Schweregrad (LOW/MEDIUM/HIGH/CRITICAL)
2. Identifiziere mögliche Ursachen
3. Gib konkrete Handlungsempfehlungen
4. Schätze Dringlichkeit ein (Sofort/Heute/Diese Woche/Monitoring)
"""

CHAT_WITH_CONTEXT_TEMPLATE = """**Verfügbare Daten:**
{context}

**Benutzerfrage:**
{user_question}

**Antwort:**
"""

CHAT_WITH_RAG_TEMPLATE = """**Verfügbare Maschinendaten:**
{context}

**Relevante Dokumentation (aus Wissensdatenbank):**
{rag_documents}

**Benutzerfrage:**
{user_question}

**Antwort:**
Beantworte die Frage basierend auf den bereitgestellten Dokumenten und Maschinendaten. 
Zitiere relevante Passagen aus den Dokumenten wenn möglich.
"""

# ==================== Few-Shot Examples ====================

FEW_SHOT_EXAMPLES = [
    {
        "user": "Was ist der aktuelle Status von CNC-Mill-01?",
        "assistant": """**Status CNC-Mill-01:**

✅ **Betriebszustand:** Normal
- Temperatur: 45°C (Normal: 40-50°C)
- Vibration: 0.5 mm/s (Normal: <1.0 mm/s)
- Spindeldrehzahl: 6000 RPM (Normal)

📊 **Letzte 24h:** Keine kritischen Events

**Empfehlung:** Keine Maßnahmen erforderlich, Routine-Monitoring fortsetzen.""",
    },
    {
        "user": "Warum ist die Temperatur bei Press-02 so hoch?",
        "assistant": """⚠️ **Temperaturanalyse Press-02:**

**Messwert:** 85°C (Normal: 55-65°C)
**Abweichung:** +28°C über Normalbereich

**Mögliche Ursachen:**
1. Kühlsystem-Defekt (häufigste Ursache)
2. Erhöhte Umgebungstemperatur
3. Überlastung (Prüfe Kraftmesswerte)
4. Verschleißte Dichtungen → Reibung

**Handlungsempfehlungen:**
🔴 **SOFORT:** Kühlflüssigkeitsstand prüfen
🔴 **HEUTE:** Kühlsystem-Inspektion durch Techniker
📋 Lastprofil der letzten Stunden analysieren

**Dringlichkeit:** HIGH - binnen 4 Stunden prüfen""",
    },
]

# ==================== Utility Functions ====================


def build_machine_context(machine: Dict, measurements: List[Dict], events: List[Dict]) -> str:
    """Erstellt Maschinen-Kontext für Prompt"""
    measurements_str = "\n".join(
        [
            f"- {m['sensor_type']}: {m['value']} {m.get('unit', '')} ({m['timestamp']})"
            for m in measurements[:10]
        ]
    )

    events_str = "\n".join(
        [f"- [{e['level']}] {e['message']} ({e['timestamp']})" for e in events[:5]]
    )

    return MACHINE_CONTEXT_TEMPLATE.format(
        machine_name=machine.get("name", "Unknown"),
        machine_id=machine.get("id", "N/A"),
        machine_type=machine.get("type", "Unknown"),
        location=machine.get("location", "Unknown"),
        measurement_count=len(measurements),
        measurements=measurements_str or "Keine Daten",
        events=events_str or "Keine Events",
    )


def build_chat_prompt(user_question: str, context: Dict) -> str:
    """Erstellt vollständigen Chat-Prompt (ohne RAG)"""
    # Kontext aufbereiten
    context_str = ""

    if "machine" in context:
        context_str += f"Maschine: {context['machine']['name']}\n"

    if "recent_measurements" in context:
        context_str += f"Messwerte: {len(context['recent_measurements'])} Einträge\n"

    if "recent_events" in context:
        context_str += f"Events: {len(context['recent_events'])} Einträge\n"

    return CHAT_WITH_CONTEXT_TEMPLATE.format(context=context_str, user_question=user_question)


def build_chat_prompt_with_rag(
    user_question: str, context: Dict, rag_documents: List[Dict]
) -> str:
    """Erstellt vollständigen Chat-Prompt MIT RAG-Dokumenten
    
    Args:
        user_question: Die Benutzerfrage
        context: Maschinendaten-Kontext (DB)
        rag_documents: Liste von RAG-Dokumenten mit 'content' und 'score'
        
    Returns:
        Vollständiger Prompt mit RAG-Dokumenten
    """
    # Kontext aufbereiten (wie vorher)
    context_str = ""
    
    if "machine" in context:
        context_str += f"Maschine: {context['machine']['name']}\n"
    
    if "recent_measurements" in context:
        context_str += f"Messwerte: {len(context['recent_measurements'])} Einträge\n"
    
    if "recent_events" in context:
        context_str += f"Events: {len(context['recent_events'])} Einträge\n"
    
    # RAG-Dokumente formatieren
    if rag_documents:
        rag_str = ""
        for i, doc in enumerate(rag_documents, 1):
            score = doc.get('score', 0.0)
            content = doc.get('content', '').strip()
            source = doc.get('source', 'Unbekannt')
            
            rag_str += f"\n**Dokument {i}** (Relevanz: {score:.2f}, Quelle: {source}):\n"
            rag_str += f"{content}\n"
    else:
        rag_str = "Keine relevanten Dokumente gefunden."
    
    return CHAT_WITH_RAG_TEMPLATE.format(
        context=context_str or "Keine Maschinendaten",
        rag_documents=rag_str,
        user_question=user_question
    )



def build_anomaly_prompt(machine: Dict, anomalies: List[Dict], context: str) -> str:
    """Erstellt Anomalie-Analyse-Prompt"""
    anomalies_str = "\n".join(
        [
            f"- {a['sensor']}: {a['value']} {a.get('unit', '')} "
            f"(Abweichung: {a.get('deviation', 'N/A')}σ)"
            for a in anomalies
        ]
    )

    return ANOMALY_ANALYSIS_TEMPLATE.format(context=context, anomalies=anomalies_str)
