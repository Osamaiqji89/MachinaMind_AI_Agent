"""
Environment Configuration
Lädt API-Keys und Konfiguration aus .env
"""

from pydantic import ConfigDict
from pydantic_settings import BaseSettings


class Settings(BaseSettings):
    model_config = ConfigDict(
        env_file=".env", env_file_encoding="utf-8", extra="allow"  # Allow extra fields from .env
    )

    # API Keys
    openai_api_key: str = ""
    anthropic_api_key: str = ""
    huggingface_api_key: str = ""

    # Database
    database_url: str = "sqlite:///MachinaData.db"

    # LLM Settings
    llm_provider: str = "huggingface"  # openai | anthropic | huggingface
    llm_model: str = "meta-llama/Llama-3.2-3B-Instruct"
    llm_temperature: float = 0.7
    llm_max_tokens: int = 2000
    llm_use_few_shot: bool = True
    huggingface_base_url: str = "https://api-inference.huggingface.co/models"

    # RAG Settings
    embedding_model: str = "all-MiniLM-L6-v2"
    vector_store_type: str = "faiss"  # faiss | chroma
    chunk_size: int = 500
    chunk_overlap: int = 50

    # API Settings
    api_host: str = "0.0.0.0"
    api_port: int = 8000
    api_reload: bool = True

    # Security
    cors_origins: list = ["*"]
    api_key_required: bool = False


settings = Settings()
