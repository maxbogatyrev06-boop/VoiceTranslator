"""
Отправка кода подтверждения на email.
Использует Mailtrap для тестов (письма видны в личном кабинете mailtrap.io).
"""
import smtplib
from email.mime.text import MIMEText

# === НАСТРОЙКИ SMTP ===
# Замените на свои данные от mailtrap.io
SMTP_HOST = "sandbox.smtp.mailtrap.io"
SMTP_PORT = 2525
SMTP_USER = "your_mailtrap_user"
SMTP_PASSWORD = "your_mailtrap_password"
SENDER_EMAIL = "noreply@voicetranslator.app"


def send_verification_code(to_email, code):
    """Отправляет код подтверждения на email."""
    try:
        message = MIMEText(
            f"Ваш код подтверждения для VoiceTranslator: {code}\n\n"
            f"Если вы не регистрировались — проигнорируйте это письмо."
        )
        message["Subject"] = "VoiceTranslator — подтверждение регистрации"
        message["From"] = SENDER_EMAIL
        message["To"] = to_email

        with smtplib.SMTP(SMTP_HOST, SMTP_PORT) as server:
            server.starttls()
            server.login(SMTP_USER, SMTP_PASSWORD)
            server.send_message(message)

        print(f"[EMAIL] Код {code} отправлен на {to_email}")
        return True
    except Exception as e:
        print(f"[EMAIL] Ошибка отправки: {e}")
        return False