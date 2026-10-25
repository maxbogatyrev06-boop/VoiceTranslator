"""Обёртка над Wav2Vec 2.0 с конвертацией аудио."""
import os
import sys
import torch
import soundfile as sf
from transformers import Wav2Vec2ForCTC, Wav2Vec2Processor

sys.path.insert(0, os.path.dirname(__file__))
from audio_utils import convert_to_16k_mono

_model = None
_processor = None

def _get_model():
    global _model, _processor
    if _model is None:
        model_name = "jonatasgrosman/wav2vec2-large-xlsr-53-russian"
        print("Загружаю Wav2Vec (первый раз ~30 сек)...")
        _processor = Wav2Vec2Processor.from_pretrained(model_name)
        _model = Wav2Vec2ForCTC.from_pretrained(model_name)
    return _model, _processor

def transcribe(audio_path):
    converted = convert_to_16k_mono(audio_path)
    model, processor = _get_model()

    speech, sr = sf.read(converted)

    if len(speech.shape) > 1:
        speech = speech.mean(axis=1)

    inputs = processor(speech, sampling_rate=16000, return_tensors="pt", padding=True)

    with torch.no_grad():
        logits = model(inputs.input_values).logits

    predicted_ids = torch.argmax(logits, dim=-1)
    text = processor.batch_decode(predicted_ids)[0]

    try:
        os.remove(converted)
    except:
        pass

    return text, "ru"