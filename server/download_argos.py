"""
Скачивание моделей Argos Translate для пар ru↔en.
"""
import argostranslate.package

def main():
    print("Обновляю индекс пакетов Argos...")
    argostranslate.package.update_package_index()
    available = argostranslate.package.get_available_packages()

    pairs = [("ru", "en"), ("en", "ru")]

    for from_code, to_code in pairs:
        print(f"\nИщу пакет {from_code} -> {to_code}...")
        package = next(
            filter(
                lambda x: x.from_code == from_code and x.to_code == to_code,
                available
            ),
            None
        )
        if not package:
            print(f"Пакет {from_code}->{to_code} не найден в индексе")
            continue

        print(f"Скачиваю...")
        path = package.download()
        print(f"Устанавливаю...")
        argostranslate.package.install_from_path(path)
        print(f"✅ {from_code}->{to_code} установлен")

    print("\nВсе модели Argos загружены!")

if __name__ == "__main__":
    main()