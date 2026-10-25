"""Обёртка над Whisper для бенчмарка."""
from faster_whisper import WhisperModel

_model = None

def _get_model():
    global _model
    if _model is None:
        _model = WhisperModel("base", device="cpu", compute_type="int8")
    return _model

def transcribe(audio_path):
    """
    Принимает путь к WAV.
    Возвращает (текст, определённый_язык).
    """
    model = _get_model()
    segments, info = model.transcribe(audio_path, beam_size=5, vad_filter=True)
    text = " ".join(seg.text.strip() for seg in segments)
    return text, info.language