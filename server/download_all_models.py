"""
Скачивание всех ML-моделей для проекта VoiceTranslator.
"""
import os
import sys

# ============================================================
# 1. Wav2Vec 2.0 (русский)
# ============================================================
def download_wav2vec():
    print("\n" + "=" * 60)
    print("  [1/2] Wav2Vec 2.0 (русский)")
    print("=" * 60)

    try:
        from transformers import Wav2Vec2ForCTC, Wav2Vec2Processor

        model_name = "jonatasgrosman/wav2vec2-large-xlsr-53-russian"
        print(f"Скачиваю {model_name}...")
        print("(Это ~1.2 ГБ, может занять 10-15 минут)")

        processor = Wav2Vec2Processor.from_pretrained(model_name)
        model = Wav2Vec2ForCTC.from_pretrained(model_name)

        print("✅ Wav2Vec 2.0 загружен и сохранён в кэш")
        return True
    except Exception as e:
        print(f"❌ Ошибка: {e}")
        return False


# ============================================================
# 2. NLLB-200 (distilled 600M)
# ============================================================
def download_nllb():
    print("\n" + "=" * 60)
    print("  [2/2] NLLB-200 (distilled 600M)")
    print("=" * 60)

    try:
        from transformers import AutoModelForSeq2SeqLM, AutoTokenizer

        model_name = "facebook/nllb-200-distilled-600M"
        print(f"Скачиваю {model_name}...")
        print("(Это ~2.5 ГБ, может занять 20-40 минут)")

        tokenizer = AutoTokenizer.from_pretrained(model_name)
        model = AutoModelForSeq2SeqLM.from_pretrained(model_name)

        print("✅ NLLB-200 загружен и сохранён в кэш")
        return True
    except Exception as e:
        print(f"❌ Ошибка: {e}")
        return False


# ============================================================
# ТОЧКА ВХОДА
# ============================================================
if __name__ == "__main__":
    print("=" * 60)
    print("  СКАЧИВАНИЕ ВСЕХ МОДЕЛЕЙ")
    print("=" * 60)
    print(f"Свободное место: проверь вручную (нужно ~4 ГБ)")

    results = []

    # Wav2Vec
    ok1 = download_wav2vec()
    results.append(("Wav2Vec 2.0", ok1))

    # NLLB
    ok2 = download_nllb()
    results.append(("NLLB-200", ok2))

    # Итог
    print("\n" + "=" * 60)
    print("  ИТОГ")
    print("=" * 60)
    for name, ok in results:
        status = "✅" if ok else "❌"
        print(f"  {status} {name}")

    if all(ok for _, ok in results):
        print("\n🎉 Все модели успешно скачаны!")
    else:
        print("\n⚠️ Некоторые модели не удалось скачать. Смотри ошибки выше.")