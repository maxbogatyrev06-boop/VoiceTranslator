"""Обёртка над Argos Translate (локально)."""
import argostranslate.translate

def translate(text, from_lang, to_lang):
    """Перевод через Argos."""
    return argostranslate.translate.translate(text, from_lang, to_lang)