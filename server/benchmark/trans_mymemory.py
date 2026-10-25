"""Обёртка над MyMemory API."""
import requests

def translate(text, from_lang, to_lang):
    """
    Принимает текст и языки.
    Возвращает переведённый текст.
    """
    url = "https://api.mymemory.translated.net/get"
    params = {
        "q": text,
        "langpair": f"{from_lang}|{to_lang}"
    }
    r = requests.get(url, params=params, timeout=15)
    data = r.json()
    return data["responseData"]["translatedText"]