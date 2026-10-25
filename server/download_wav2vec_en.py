from transformers import Wav2Vec2ForCTC, Wav2Vec2Processor

print("Скачиваю английскую модель Wav2Vec 2.0...")
model_name = "facebook/wav2vec2-base-960h"

processor = Wav2Vec2Processor.from_pretrained(model_name)
model = Wav2Vec2ForCTC.from_pretrained(model_name)

print("✅ Английская Wav2Vec 2.0 загружена")