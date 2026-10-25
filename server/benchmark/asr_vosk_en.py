"""Обёртка над Vosk EN."""
import json
import wave
import os
import sys
from vosk import Model, KaldiRecognizer

sys.path.insert(0, os.path.dirname(__file__))
from audio_utils import convert_to_16k_mono

_model = None

def _get_model():
    global _model
    if _model is None:
        _model = Model("../vosk_model/en")
    return _model

def transcribe(audio_path):
    converted = convert_to_16k_mono(audio_path)
    model = _get_model()

    wf = wave.open(converted, "rb")
    rec = KaldiRecognizer(model, wf.getframerate())
    rec.SetWords(True)

    parts = []
    while True:
        data = wf.readframes(4000)
        if len(data) == 0:
            break
        if rec.AcceptWaveform(data):
            result = json.loads(rec.Result())
            parts.append(result.get("text", ""))

    final = json.loads(rec.FinalResult())
    parts.append(final.get("text", ""))

    text = " ".join(p for p in parts if p).strip()
    wf.close()

    try:
        os.remove(converted)
    except:
        pass

    return text, "en"