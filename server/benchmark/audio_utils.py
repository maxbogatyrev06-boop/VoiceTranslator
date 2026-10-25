"""
Утилиты для работы с аудио.
"""
import os
import soundfile as sf
import librosa

def convert_to_16k_mono(input_path, output_path=None):
    """
    Конвертирует любой WAV в моно 16 кГц.
    Возвращает путь к новому файлу.
    """
    if output_path is None:
        base, ext = os.path.splitext(input_path)
        output_path = f"{base}_16k_mono.wav"

    speech, sr = librosa.load(input_path, sr=16000, mono=True)
    sf.write(output_path, speech, 16000)
    return output_path