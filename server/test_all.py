"""
Проверка всех установленных моделей.
"""
import os
import sys

print("=" * 60)
print("  ПРОВЕРКА УСТАНОВЛЕННЫХ МОДЕЛЕЙ")
print("=" * 60)

# 1. Argos Translate
print("\n[1] Argos Translate (ru→en):")
try:
    import argostranslate.translate
    result = argostranslate.translate.translate("Привет, как дела?", "ru", "en")
    print(f"    ✅ Результат: {result}")
except Exception as e:
    print(f"    ❌ Ошибка: {e}")

# 2. Whisper
print("\n[2] Whisper base (CPU):")
try:
    from faster_whisper import WhisperModel
    model = WhisperModel("base", device="cpu", compute_type="int8")
    print(f"    ✅ Модель загружена")
except Exception as e:
    print(f"    ❌ Ошибка: {e}")

# 3. Vosk
print("\n[3] Vosk (ru):")
try:
    from vosk import Model
    if not os.path.exists("vosk_model/ru"):
        print("    ❌ Папка vosk_model/ru не найдена")
    else:
        model = Model("vosk_model/ru")
        print(f"    ✅ Модель загружена")
except Exception as e:
    print(f"    ❌ Ошибка: {e}")

# 4. Wav2Vec 2.0 (русский)
print("\n[4] Wav2Vec 2.0 (русский):")
try:
    from transformers import Wav2Vec2ForCTC, Wav2Vec2Processor
    model_name = "jonatasgrosman/wav2vec2-large-xlsr-53-russian"
    processor = Wav2Vec2Processor.from_pretrained(model_name)
    model = Wav2Vec2ForCTC.from_pretrained(model_name)
    print(f"    ✅ Модель загружена")
except Exception as e:
    print(f"    ❌ Ошибка: {e}")

# 5. NLLB-200
print("\n[5] NLLB-200:")
try:
    from transformers import AutoModelForSeq2SeqLM, AutoTokenizer
    model_name = "facebook/nllb-200-distilled-600M"
    tokenizer = AutoTokenizer.from_pretrained(model_name)
    model = AutoModelForSeq2SeqLM.from_pretrained(model_name)
    print(f"    ✅ Модель загружена")
except Exception as e:
    print(f"    ❌ Ошибка: {e}")

print("\n" + "=" * 60)
print("  ПРОВЕРКА ЗАВЕРШЕНА")
print("=" * 60)