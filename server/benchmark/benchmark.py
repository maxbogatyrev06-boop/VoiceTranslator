"""
Бенчмарк ASR × Переводчик.
Поддерживает два направления:
  ru-en (по умолчанию) — русская речь → английский перевод
  en-ru — английская речь → русский перевод
"""
import os
import sys
import time
import json
import csv

sys.path.insert(0, os.path.dirname(__file__))

from jiwer import cer
from nltk.translate.bleu_score import sentence_bleu, SmoothingFunction
import nltk

try:
    nltk.data.find('tokenizers/punkt')
except LookupError:
    nltk.download('punkt', quiet=True)
    nltk.download('punkt_tab', quiet=True)

import asr_whisper
import asr_vosk
import asr_wav2vec
import trans_mymemory
import trans_argos
import trans_nllb


# ============================================================
# РЕЖИМ (из аргументов командной строки)
# ============================================================
DIRECTION = sys.argv[1] if len(sys.argv) > 1 else "ru-en"

if DIRECTION == "ru-en":
    AUDIO_PATH = "audio/test_ru.wav"
    REF_ASR = open("reference/ru.txt", encoding="utf-8").read().strip()
    REF_TRANS = open("reference/en.txt", encoding="utf-8").read().strip()
    FROM_LANG = "ru"
    TO_LANG = "en"

    ASR_MODELS = [
        ("Whisper base", asr_whisper),
        ("Vosk", asr_vosk),
        ("Wav2Vec 2.0", asr_wav2vec),
    ]

elif DIRECTION == "en-ru":
    AUDIO_PATH = "audio/test_en.wav"
    REF_ASR = open("reference/en_audio.txt", encoding="utf-8").read().strip()
    REF_TRANS = open("reference/ru_translation.txt", encoding="utf-8").read().strip()
    FROM_LANG = "en"
    TO_LANG = "ru"

    import asr_vosk_en
    import asr_wav2vec_en

    ASR_MODELS = [
        ("Whisper base", asr_whisper),
        ("Vosk EN", asr_vosk_en),
        ("Wav2Vec EN", asr_wav2vec_en),
    ]
else:
    print(f"❌ Неверное направление: {DIRECTION}")
    print("Используй: python benchmark.py ru-en  или  python benchmark.py en-ru")
    sys.exit(1)


TRANS_MODELS = [
    ("MyMemory", trans_mymemory),
    ("Argos", trans_argos),
    ("NLLB", trans_nllb),
]


# ============================================================
# УТИЛИТЫ
# ============================================================
def normalize(text):
    import re
    text = text.lower()
    text = re.sub(r"[^\w\s]", "", text, flags=re.UNICODE)
    text = re.sub(r"\s+", " ", text).strip()
    return text


def accuracy_cer(reference, hypothesis):
    ref = normalize(reference)
    hyp = normalize(hypothesis)
    if not ref or not hyp:
        return 0.0
    try:
        return max(0.0, (1 - cer(ref, hyp)) * 100)
    except Exception:
        return 0.0


def accuracy_bleu(reference, hypothesis):
    ref_tokens = normalize(reference).split()
    hyp_tokens = normalize(hypothesis).split()
    if not ref_tokens or not hyp_tokens:
        return 0.0
    try:
        smoothie = SmoothingFunction().method1
        return sentence_bleu([ref_tokens], hyp_tokens, smoothing_function=smoothie) * 100
    except Exception:
        return 0.0


# ============================================================
# ГЛАВНАЯ ФУНКЦИЯ
# ============================================================
def run_benchmark():
    print("=" * 70)
    print(f"  БЕНЧМАРК: направление {DIRECTION.upper()}")
    print("=" * 70)
    print(f"\n📝 Эталон ASR ({FROM_LANG}): {REF_ASR}")
    print(f"📝 Эталон перевода ({TO_LANG}): {REF_TRANS}")
    print(f"🎵 Аудио: {AUDIO_PATH}")

    if not os.path.exists(AUDIO_PATH):
        print(f"\n❌ Файл {AUDIO_PATH} не найден!")
        return

    print("\n" + "=" * 70)
    print("  ЗАМЕР (холодный старт)")
    print("=" * 70)

    results = []
    counter = 0
    total = len(ASR_MODELS) * len(TRANS_MODELS)

    for asr_name, asr_module in ASR_MODELS:
        print(f"\n{'=' * 70}")
        print(f"  🎤 ASR: {asr_name}")
        print(f"{'=' * 70}")

        try:
            t0 = time.perf_counter()
            recognized_text, detected_lang = asr_module.transcribe(AUDIO_PATH)
            asr_time = time.perf_counter() - t0
            asr_acc = accuracy_cer(REF_ASR, recognized_text)

            print(f"\n  Распознано: \"{recognized_text}\"")
            print(f"  Язык:       {detected_lang}")
            print(f"  Точность:   {asr_acc:.1f}%")
            print(f"  Время ASR:  {asr_time:.3f} сек")
        except Exception as e:
            print(f"\n  ❌ Ошибка ASR: {e}")
            continue

        for trans_name, trans_module in TRANS_MODELS:
            counter += 1
            print(f"\n  [{counter}/{total}] Переводчик: {trans_name}")

            try:
                t0 = time.perf_counter()
                translated_text = trans_module.translate(
                    recognized_text, FROM_LANG, TO_LANG
                )
                trans_time = time.perf_counter() - t0
                trans_acc = accuracy_bleu(REF_TRANS, translated_text)

                print(f"    Переведено: \"{translated_text}\"")
                print(f"    BLEU:        {trans_acc:.1f}%")
                print(f"    Время:       {trans_time:.3f} сек")

                results.append({
                    "direction": DIRECTION,
                    "asr": asr_name,
                    "translator": trans_name,
                    "asr_text": recognized_text,
                    "asr_accuracy": round(asr_acc, 1),
                    "asr_time": round(asr_time, 3),
                    "trans_text": translated_text,
                    "trans_accuracy": round(trans_acc, 1),
                    "trans_time": round(trans_time, 3),
                    "total_time": round(asr_time + trans_time, 3),
                })
            except Exception as e:
                print(f"    ❌ Ошибка: {e}")

    # ИТОГ
    print("\n\n" + "=" * 70)
    print(f"  ИТОГОВАЯ ТАБЛИЦА ({DIRECTION})")
    print("=" * 70)

    header = (f"{'ASR':<15} | {'Перевод':<10} | {'ASR %':>6} | "
              f"{'BLEU':>6} | {'ASR с':>7} | {'Пер с':>7} | {'Итого':>7}")
    print(header)
    print("-" * len(header))
    for r in results:
        print(f"{r['asr']:<15} | {r['translator']:<10} | "
              f"{r['asr_accuracy']:>6} | {r['trans_accuracy']:>6} | "
              f"{r['asr_time']:>7} | {r['trans_time']:>7} | "
              f"{r['total_time']:>7}")

    # Сохранение
    fname = f"results_{DIRECTION.replace('-', '_')}.json"
    with open(fname, "w", encoding="utf-8") as f:
        json.dump(results, f, ensure_ascii=False, indent=2)

    if results:
        fname_csv = f"results_{DIRECTION.replace('-', '_')}.csv"
        with open(fname_csv, "w", encoding="utf-8-sig", newline="") as f:
            writer = csv.DictWriter(f, fieldnames=results[0].keys())
            writer.writeheader()
            writer.writerows(results)

    print(f"\n💾 Сохранено: {fname}, {fname_csv}")
    print("=" * 70)


if __name__ == "__main__":
    run_benchmark()