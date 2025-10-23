# Hugging Face API Setup

## 🚀 Schnellstart (5 Minuten)

### 1. Kostenlosen Account erstellen
- Gehen Sie zu: **https://huggingface.co/join**
- Registrieren Sie sich mit Email oder GitHub
- Bestätigen Sie Ihre Email-Adresse

### 2. API Token generieren
- Gehen Sie zu: **https://huggingface.co/settings/tokens**
- Click **"New token"**
- **Name**: `MachinaMindAIAgent`
- **Type**: `Read` (ausreichend für Inference API)
- Click **"Generate token"**
- **Kopieren Sie den Token** (beginnt mit `hf_...`)

### 3. Token in .env eintragen
Öffnen Sie `backend/.env` und ersetzen Sie:

```env
HUGGINGFACE_API_KEY=hf_your_token_here
```

mit Ihrem echten Token:

```env
HUGGINGFACE_API_KEY=hf_xxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
```

### 4. Backend neu starten
```powershell
cd backend
python api/main.py
```

Sie sollten sehen:
```
✓ Hugging Face configured (model: meta-llama/Llama-3.2-3B-Instruct)
```

## 🎯 Fertig!

Jetzt können Sie im Chat mit der KI sprechen!

## 📊 Verfügbare Modelle

Das Standard-Modell ist **Llama 3.2 3B Instruct** - schnell und kostenlos.

### Andere empfohlene Modelle:

**Schnell & Klein:**
- `meta-llama/Llama-3.2-1B-Instruct` (sehr schnell)
- `Qwen/Qwen2.5-3B-Instruct` (gute Balance)

**Besser, aber langsamer:**
- `meta-llama/Llama-3.1-8B-Instruct` (höhere Qualität)
- `mistralai/Mistral-7B-Instruct-v0.3` (sehr gut)

Um das Modell zu ändern, bearbeiten Sie in `backend/.env`:
```env
LLM_MODEL=meta-llama/Llama-3.2-1B-Instruct
```

## 💰 Kosten

Die **Hugging Face Inference API ist KOSTENLOS** für:
- Öffentliche Modelle
- Moderate Nutzung (Rate Limits gelten)

Perfekt für Entwicklung und Tests!

## 🔧 Troubleshooting

### "Model is currently loading"
Das Modell wird beim ersten Aufruf geladen. Warten Sie 30-60 Sekunden und versuchen Sie es erneut.

### "Rate limit exceeded"
Sie haben zu viele Anfragen gesendet. Warten Sie 1 Minute oder verwenden Sie ein anderes Modell.

### "Invalid API key"
- Überprüfen Sie, dass der Token mit `hf_` beginnt
- Stellen Sie sicher, dass keine Leerzeichen vor/nach dem Token sind
- Generieren Sie ggf. einen neuen Token

## 📚 Weitere Informationen

- Hugging Face Docs: https://huggingface.co/docs/api-inference
- Modell-Browser: https://huggingface.co/models?pipeline_tag=text-generation
- Rate Limits: https://huggingface.co/docs/api-inference/rate-limits
