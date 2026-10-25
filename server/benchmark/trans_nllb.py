"""Обёртка над NLLB-200 (исправленная)."""
from transformers import AutoModelForSeq2SeqLM, AutoTokenizer
import torch

_model = None
_tokenizer = None

NLLB_LANG_CODES = {
    "ru": "rus_Cyrl",
    "en": "eng_Latn",
    "de": "deu_Latn",
    "fr": "fra_Latn",
    "es": "spa_Latn",
}

def _get_model():
    global _model, _tokenizer
    if _model is None:
        model_name = "facebook/nllb-200-distilled-600M"
        print("Загружаю NLLB (первый раз ~1 мин)...")
        _tokenizer = AutoTokenizer.from_pretrained(model_name)
        _model = AutoModelForSeq2SeqLM.from_pretrained(model_name)
    return _model, _tokenizer

def translate(text, from_lang, to_lang):
    model, tokenizer = _get_model()

    src = NLLB_LANG_CODES.get(from_lang, "rus_Cyrl")
    tgt = NLLB_LANG_CODES.get(to_lang, "eng_Latn")

    tokenizer.src_lang = src
    inputs = tokenizer(text, return_tensors="pt", padding=True,
                       truncation=True, max_length=512)

    with torch.no_grad():
        generated = model.generate(
            **inputs,
            forced_bos_token_id=tokenizer.convert_tokens_to_ids(tgt),
            max_new_tokens=256,
            num_beams=4,
            early_stopping=True
        )

    result = tokenizer.batch_decode(generated, skip_special_tokens=True)[0]
    return result