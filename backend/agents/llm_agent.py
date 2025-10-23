"""
LLM Agent
Wrapper für LLM-Anfragen (OpenAI, Anthropic, Hugging Face)
Mit RAG-Unterstützung für Wartungsprotokolle
"""

import asyncio
from typing import Any

import aiohttp
from loguru import logger

_AsyncOpenAI = None
try:
    from openai import AsyncOpenAI as _OpenAIClass
    _AsyncOpenAI = _OpenAIClass
except ImportError:
    pass

from config import settings
from prompt_templates import (
    FEW_SHOT_EXAMPLES,
    SYSTEM_PROMPT,
    build_chat_prompt,
    build_chat_prompt_with_rag,
)

# RAG Integration
_RAGManagerClass = None
try:
    from rag_engine.rag_manager import RAGManager as _RAGClass
    _RAGManagerClass = _RAGClass
except ImportError:
    logger.warning("RAG Manager not available")


class LLMAgent:
    """Agent für LLM-Interaktionen mit RAG-Unterstützung"""

    def __init__(self):
        self.provider = settings.llm_provider
        self.model = settings.llm_model
        self.temperature = settings.llm_temperature
        self.max_tokens = settings.llm_max_tokens

        # Few-Shot Examples aktivieren (verbessert Antwort-Qualität, kostet ~350 Tokens)
        self.use_few_shot = getattr(settings, "llm_use_few_shot", True)  # Default: AN

        # RAG Manager initialisieren
        self.rag_manager = None
        if _RAGManagerClass:
            try:
                self.rag_manager = _RAGManagerClass(vector_store_path="vector_store")
                stats = self.rag_manager.get_stats()
                if stats["total_vectors"] > 0:
                    logger.info(f"✓ RAG enabled: {stats['total_vectors']} Dokumente indiziert")
                else:
                    logger.warning("RAG Manager loaded but no documents indexed")
            except Exception as e:
                logger.warning(f"RAG Manager initialization failed: {e}")
                self.rag_manager = None

        # Initialize client based on provider
        if self.provider == "huggingface":
            if (
                settings.huggingface_api_key
                and settings.huggingface_api_key != "hf_your_token_here"
            ):
                self.client = "huggingface"  # Marker
                self.api_key = settings.huggingface_api_key
                self.base_url = settings.huggingface_base_url
                logger.info(f"✓ Hugging Face configured (model: {self.model})")
            else:
                logger.warning("Hugging Face API key not set - LLM will use fallback mode")
                self.client = None
        elif self.provider == "openai" and _AsyncOpenAI:
            if settings.openai_api_key:
                self.client = _AsyncOpenAI(api_key=settings.openai_api_key)
                logger.info(f"✓ OpenAI configured (model: {self.model})")
            else:
                logger.warning("OpenAI API key not set - LLM will use fallback mode")
                self.client = None
        else:
            self.client = None
            logger.warning(f"Provider {self.provider} not configured - using fallback")

    async def query(
        self, user_message: str, context: dict[str, Any] | None = None
    ) -> tuple[str, list[str]]:
        """
        Sendet Query an LLM mit RAG-Unterstützung

        Args:
            user_message: Benutzerfrage
            context: Zusätzlicher Kontext (Maschine, Messungen, etc.)

        Returns:
            Tuple[answer, sources] - LLM-Antwort und RAG-Quellen
        """
        sources = []
        rag_documents = []

        # RAG: Relevante Dokumente abrufen
        if self.rag_manager:
            try:
                rag_results = self.rag_manager.retrieve(user_message, k=3, score_threshold=0.3)
                if rag_results:
                    logger.info(f"RAG: {len(rag_results)} relevante Dokumente gefunden")

                    # Sources für Response aufbereiten
                    sources = [
                        f"Wartungsprotokoll (Score: {score:.2f})" for _, score in rag_results
                    ]

                    # RAG-Dokumente für Prompt aufbereiten
                    rag_documents = [
                        {
                            "content": doc,
                            "score": score,
                            "source": "Wartungsprotokoll_industrielle_Maschinen.pdf",
                        }
                        for doc, score in rag_results
                    ]
            except Exception as e:
                logger.warning(f"RAG retrieval failed: {e}")

        if not self.client:
            return self._fallback_response(user_message, context), sources

        try:
            # Prompt aufbauen - MIT oder OHNE RAG
            messages = [{"role": "system", "content": SYSTEM_PROMPT}]

            # Few-Shot Examples hinzufügen (zeigt AI das gewünschte Antwort-Format)
            if self.use_few_shot:
                for example in FEW_SHOT_EXAMPLES:
                    messages.append({"role": "user", "content": example["user"]})
                    messages.append({"role": "assistant", "content": example["assistant"]})

                logger.info(f"✓ Added {len(FEW_SHOT_EXAMPLES)} few-shot examples")
            else:
                logger.info("Few-shot examples disabled (faster but lower quality)")

            # User-Prompt mit korrektem Template
            if rag_documents:
                # MIT RAG: Nutze build_chat_prompt_with_rag
                user_prompt = build_chat_prompt_with_rag(user_message, context or {}, rag_documents)
                logger.info(f"Using RAG prompt with {len(rag_documents)} documents")
            else:
                # OHNE RAG: Nutze build_chat_prompt
                user_prompt = build_chat_prompt(user_message, context or {})
                logger.info("Using standard prompt (no RAG documents)")

            messages.append({"role": "user", "content": user_prompt})

            # LLM Call mit Timeout
            logger.info(f"Sending query to {self.provider}: {user_message[:50]}...")

            try:
                if self.provider == "huggingface":
                    response_text = await asyncio.wait_for(
                        self._query_huggingface(messages), timeout=30.0
                    )
                else:
                    # OpenAI-compatible API
                    openai_response = await asyncio.wait_for(
                        self.client.chat.completions.create(
                            model=self.model,
                            messages=messages,
                            temperature=self.temperature,
                            max_tokens=self.max_tokens,
                        ),
                        timeout=30.0,
                    )
                    response_text = openai_response.choices[0].message.content

                logger.info("LLM response received successfully")
                return response_text, sources
            except asyncio.TimeoutError:
                logger.warning("LLM query timed out after 30s, using fallback")
                return self._fallback_response(user_message, context), sources

        except Exception as e:
            logger.error(f"LLM query failed: {e}")
            return self._fallback_response(user_message, context), sources

    async def _query_huggingface(self, messages: list[dict[str, str]]) -> str:
        """Sendet Anfrage an Hugging Face Inference API"""
        # Konvertiere Chat-Format zu Text-Prompt
        prompt = "\n\n".join([f"{msg['role']}: {msg['content']}" for msg in messages])
        prompt += "\n\nassistant:"

        # Korrekte URL-Konstruktion
        url = f"{self.base_url.rstrip('/')}/{self.model}"

        logger.debug(f"HuggingFace API URL: {url}")

        headers = {"Authorization": f"Bearer {self.api_key}", "Content-Type": "application/json"}
        payload = {
            "inputs": prompt,
            "parameters": {
                "max_new_tokens": self.max_tokens,
                "temperature": self.temperature,
                "return_full_text": False,
            },
        }

        async with aiohttp.ClientSession() as session:
            async with session.post(url, json=payload, headers=headers) as response:
                if response.status == 200:
                    result = await response.json()
                    # Hugging Face kann verschiedene Formate zurückgeben
                    if isinstance(result, list) and len(result) > 0:
                        return result[0].get("generated_text", "").strip()
                    elif isinstance(result, dict):
                        return result.get("generated_text", "").strip()
                    else:
                        raise Exception(f"Unexpected response format: {result}")
                elif response.status == 503:
                    # Modell wird geladen
                    error_json = await response.json()
                    estimated_time = error_json.get("estimated_time", 20)
                    raise Exception(
                        f"Modell wird geladen, bitte warten Sie ca. {estimated_time} Sekunden und versuchen Sie es erneut."
                    )
                else:
                    error_text = await response.text()
                    raise Exception(f"HuggingFace API error {response.status}: {error_text}")

    def _format_context(self, context: dict[str, Any]) -> str:
        """
        [DEPRECATED] Alte Kontext-Formatierung
        Nutze stattdessen: build_chat_prompt() oder build_chat_prompt_with_rag()
        """
        parts = ["**Verfügbarer Kontext:**\n"]

        # RAG-Dokumente zuerst (am wichtigsten!)
        if "rag_documents" in context:
            docs = context["rag_documents"]
            parts.append(f"\n**Wartungsprotokoll-Auszüge ({len(docs)}):**")
            for i, doc in enumerate(docs, 1):
                parts.append(f"\n[Dokument {i}]")
                parts.append(doc[:500] + "..." if len(doc) > 500 else doc)

        if "machine" in context:
            machine = context["machine"]
            parts.append(f"\nMaschine: {machine.get('name')} (Typ: {machine.get('type')})")

        if "recent_measurements" in context:
            measurements = context["recent_measurements"][:10]
            parts.append(f"\n**Letzte Messwerte ({len(measurements)}):**")
            for m in measurements:
                parts.append(
                    f"- {m['sensor_type']}: {m['value']} {m.get('unit', '')} " f"({m['timestamp']})"
                )

        if "recent_events" in context:
            events = context["recent_events"][:5]
            parts.append(f"\n**Letzte Events ({len(events)}):**")
            for e in events:
                parts.append(f"- [{e['level']}] {e['message']}")

        if "machines" in context:
            machines = context["machines"]
            parts.append(f"\n**Verfügbare Maschinen ({len(machines)}):**")
            for m in machines:
                parts.append(f"- {m['name']} ({m['type']})")

        return "\n".join(parts)

    def _fallback_response(self, user_message: str, context: dict | None) -> str:
        """Fallback wenn kein LLM verfügbar"""
        response = f"**Ihre Frage:** {user_message}\n\n"
        response += (
            "ℹ️ _LLM antwortet nicht oder ist nicht verfügbar. Hier sind die verfügbaren Daten:_\n\n"
        )

        if context:
            if "machine" in context:
                machine = context["machine"]
                response += f"**Maschine:** {machine.get('name')} (Typ: {machine.get('type')})\n"
                response += f"- Status: {machine.get('status', 'unknown')}\n"
                response += f"- Standort: {machine.get('location', 'unknown')}\n"

            if "recent_measurements" in context:
                measurements = context["recent_measurements"][:5]
                response += f"\n**Letzte {len(measurements)} Messwerte:**\n"
                for m in measurements:
                    response += f"- {m['sensor_type']}: {m['value']} {m.get('unit', '')} ({m['timestamp']})\n"

            if "recent_events" in context:
                events = context["recent_events"][:3]
                critical = sum(1 for e in events if e["level"] in ["ERROR", "CRITICAL"])
                response += f"\n**Events:** {len(events)} gesamt, davon {critical} kritisch\n"
                for e in events[:3]:
                    response += f"- [{e['level']}] {e['message']}\n"

            if "machines" in context:
                machines = context["machines"]
                response += f"\n**Verfügbare Maschinen ({len(machines)}):**\n"
                for m in machines[:5]:
                    response += f"- {m['name']} ({m['type']})\n"
        else:
            response += "_Kein Kontext verfügbar_\n"

        response += "\n\n💡 **Hinweis:** "
        if self.provider == "huggingface":
            response += "Setzen Sie Ihren Hugging Face API Key in der .env Datei: HUGGINGFACE_API_KEY=hf_..."
        else:
            response += (
                "Konfigurieren Sie einen LLM-Provider (OpenAI oder Hugging Face) in der .env Datei."
            )

        return response

    async def analyze_anomaly(self, machine_name: str, anomalies: list[dict], context: str) -> str:
        """
        Spezialisierte Anfrage für Anomalie-Analyse
        """
        prompt = f"""Analysiere folgende Anomalien für Maschine {machine_name}:

{context}

**Erkannte Anomalien:**
"""
        for a in anomalies:
            prompt += f"- {a['sensor']}: {a['value']} (Abweichung: {a.get('deviation', 'N/A')})\n"

        prompt += """
**Aufgabe:**
1. Bewerte den Schweregrad (CRITICAL/HIGH/MEDIUM/LOW)
2. Nenne mögliche Ursachen
3. Gib konkrete Handlungsempfehlungen
4. Schätze Dringlichkeit (Sofort/Heute/Diese Woche)
"""

        response_text, _sources = await self.query(prompt, None)
        return response_text
