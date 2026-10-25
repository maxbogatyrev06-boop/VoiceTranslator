"""
Работа с базой данных SQLite.
"""
import sqlite3
import os

DB_PATH = os.path.join(os.path.dirname(__file__), 'users.db')


def get_connection():
    """Возвращает соединение с БД."""
    conn = sqlite3.connect(DB_PATH)
    conn.row_factory = sqlite3.Row
    return conn


def init_db():
    """Создаёт таблицы, если их нет."""
    conn = get_connection()
    cursor = conn.cursor()

    # Таблица пользователей
    cursor.execute('''
        CREATE TABLE IF NOT EXISTS users (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            email TEXT UNIQUE,
            phone TEXT UNIQUE,
            username TEXT UNIQUE NOT NULL,
            password_hash TEXT NOT NULL,
            birthdate TEXT,
            country TEXT,ы
            native_language TEXT,
            avatar_path TEXT,
            is_verified INTEGER DEFAULT 0,
            verification_code TEXT,
            created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
        )
    ''')

    # Таблица онлайн-статуса (heartbeat)
    cursor.execute('''
        CREATE TABLE IF NOT EXISTS online_users (
            user_id INTEGER PRIMARY KEY,
            last_seen TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
            FOREIGN KEY (user_id) REFERENCES users(id)
        )
    ''')

    conn.commit()
    conn.close()
    print("[DB] База данных инициализирована")


# === Функции работы с пользователями ===

def user_exists(email=None, phone=None, username=None):
    """Проверяет, существует ли пользователь по email/телефону/никнейму."""
    conn = get_connection()
    cursor = conn.cursor()
    if email:
        cursor.execute("SELECT id FROM users WHERE email = ?", (email,))
        if cursor.fetchone():
            conn.close()
            return True
    if phone:
        cursor.execute("SELECT id FROM users WHERE phone = ?", (phone,))
        if cursor.fetchone():
            conn.close()
            return True
    if username:
        cursor.execute("SELECT id FROM users WHERE username = ?", (username,))
        if cursor.fetchone():
            conn.close()
            return True
    conn.close()
    return False


def create_user(email, phone, username, password_hash,
                birthdate, country, native_language, verification_code):
    """Создаёт нового пользователя. Возвращает id."""
    conn = get_connection()
    cursor = conn.cursor()
    cursor.execute('''
        INSERT INTO users (email, phone, username, password_hash,
                           birthdate, country, native_language,
                           verification_code, is_verified)
        VALUES (?, ?, ?, ?, ?, ?, ?, ?, 0)
    ''', (email, phone, username, password_hash,
          birthdate, country, native_language, verification_code))
    user_id = cursor.lastrowid
    conn.commit()
    conn.close()
    return user_id


def get_user_by_email(email):
    """Возвращает пользователя по email."""
    conn = get_connection()
    cursor = conn.cursor()
    cursor.execute("SELECT * FROM users WHERE email = ?", (email,))
    row = cursor.fetchone()
    conn.close()
    return dict(row) if row else None


def get_user_by_phone(phone):
    """Возвращает пользователя по телефону."""
    conn = get_connection()
    cursor = conn.cursor()
    cursor.execute("SELECT * FROM users WHERE phone = ?", (phone,))
    row = cursor.fetchone()
    conn.close()
    return dict(row) if row else None


def verify_user(user_id):
    """Помечает пользователя как подтверждённого."""
    conn = get_connection()
    cursor = conn.cursor()
    cursor.execute("UPDATE users SET is_verified = 1 WHERE id = ?", (user_id,))
    conn.commit()
    conn.close()


def get_user_by_id(user_id):
    """Возвращает пользователя по id."""
    conn = get_connection()
    cursor = conn.cursor()
    cursor.execute("SELECT * FROM users WHERE id = ?", (user_id,))
    row = cursor.fetchone()
    conn.close()
    return dict(row) if row else None


def get_online_users():
    """Возвращает список онлайн-пользователей (кто недавно был активен)."""
    conn = get_connection()
    cursor = conn.cursor()
    cursor.execute('''
        SELECT u.id, u.username, u.country, u.native_language, u.avatar_path
        FROM users u
        JOIN online_users o ON u.id = o.user_id
        WHERE u.is_verified = 1
          AND o.last_seen > datetime('now', '-30 seconds')
    ''')
    rows = cursor.fetchall()
    conn.close()
    return [dict(r) for r in rows]


def set_user_online(user_id):
    """Обновляет heartbeat пользователя."""
    conn = get_connection()
    cursor = conn.cursor()
    cursor.execute('''
        INSERT INTO online_users (user_id, last_seen)
        VALUES (?, CURRENT_TIMESTAMP)
        ON CONFLICT(user_id) DO UPDATE SET last_seen = CURRENT_TIMESTAMP
    ''', (user_id,))
    conn.commit()
    conn.close()


def set_user_offline(user_id):
    """Помечает пользователя офлайн."""
    conn = get_connection()
    cursor = conn.cursor()
    cursor.execute("DELETE FROM online_users WHERE user_id = ?", (user_id,))
    conn.commit()
    conn.close()


def get_all_countries():
    """Список уникальных стран из БД."""
    conn = get_connection()
    cursor = conn.cursor()
    cursor.execute("SELECT DISTINCT country FROM users WHERE country IS NOT NULL")
    rows = cursor.fetchall()
    conn.close()
    return [r['country'] for r in rows]