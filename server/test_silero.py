"""
Диагностика Silero STT.
"""
import traceback
import torch

print("PyTorch:", torch.__version__)
print("CUDA доступна:", torch.cuda.is_available())

try:
    print("\nЗагружаю Silero...")
    model, decoder, utils, _ = torch.hub.load(
        repo_or_dir='snakers4/silero-models',
        model='silero_stt',
        language='ru',
        device=torch.device('cpu'),
        trust_repo=True
    )
    print("✅ Silero загружен успешно")

    # Проверим, что модель умеет
    print("Тип модели:", type(model))
    print("Decoder:", type(decoder))

except Exception as e:
    print("\n❌ ПОЛНАЯ ОШИБКА:")
    print("=" * 60)
    traceback.print_exc()
    print("=" * 60)