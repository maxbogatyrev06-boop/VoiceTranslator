"""
whisper_stt.py
Распознавание речи с помощью Whisper.
Вызов: python whisper_stt.py <аудиофайл> [язык]
Результат: JSON с полями text, language.
"""

import sys
import json

sys.stdout.reconfigure(encoding='utf-8')

from faster_whisper import WhisperModel

model = WhisperModel("base", device="cpu", compute_type="int8")


def transcribe_audio(file_path: str, language: str = None) -> dict:
    try:
        segments, info = model.transcribe(
            file_path,
            beam_size=2,
            language=language,      # None = автоопределение
            vad_filter=True,        # отсекаем тишину
        )
        text = " ".join(seg.text.strip() for seg in segments)
        return {"text": text, "language": info.language}
    except Exception as e:
        return {"text": "", "language": "", "error": str(e)}


if __name__ == "__main__":
    if len(sys.argv) < 2:
        print(json.dumps({"error": "No file specified"}))
        sys.exit(1)

    audio_file = sys.argv[1]
    lang = sys.argv[2] if len(sys.argv) > 2 else None

    result = transcribe_audio(audio_file, lang)
    print(json.dumps(result, ensure_ascii=False))