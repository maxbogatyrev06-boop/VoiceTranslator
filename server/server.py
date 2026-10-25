"""
Сервер VoiceTranslator.
Flask + flask-sock + SQLite.
"""
import random
import hashlib
import json
import os
import uuid
from flask import Flask, request, jsonify, send_from_directory
from flask_cors import CORS
from flask_sock import Sock
from simple_websocket import ConnectionClosed

import database as db
from email_sender import send_verification_code

app = Flask(__name__)
app.config['SECRET_KEY'] = 'voice-translator-secret-key'
CORS(app)
sock = Sock(app)


# ============================================================
# ВСПОМОГАТЕЛЬНЫЕ ФУНКЦИИ
# ============================================================
def hash_password(password):
    """SHA-256 хеш пароля."""
    return hashlib.sha256(password.encode('utf-8')).hexdigest()


def generate_code():
    """Генерирует 4-значный код подтверждения."""
    return str(random.randint(1000, 9999))


# ============================================================
# HTTP API
# ============================================================
@app.route('/api/health', methods=['GET'])
def health():
    return jsonify({"status": "ok"})


@app.route('/api/register', methods=['POST'])
def register():
    """Регистрация нового пользователя."""
    data = request.json
    email = data.get('email', '').strip().lower() or None
    phone = data.get('phone', '').strip() or None
    username = data.get('username', '').strip()
    password = data.get('password', '')
    birthdate = data.get('birthdate', '')
    country = data.get('country', '')
    native_language = data.get('native_language', '')

    if not username or not password:
        return jsonify({"error": "Никнейм и пароль обязательны"}), 400

    if not email and not phone:
        return jsonify({"error": "Укажите email или телефон"}), 400

    if db.user_exists(email=email, phone=phone, username=username):
        return jsonify({"error": "Пользователь уже существует"}), 409

    code = generate_code()
    password_hash = hash_password(password)

    user_id = db.create_user(
        email=email, phone=phone, username=username,
        password_hash=password_hash, birthdate=birthdate,
        country=country, native_language=native_language,
        verification_code=code
    )

    if email:
        send_verification_code(email, code)
    else:
        print(f"[SMS] Код {code} для {phone}")

    return jsonify({
        "user_id": user_id,
        "message": "Код отправлен",
        "debug_code": code
    }), 201


@app.route('/api/verify', methods=['POST'])
def verify():
    """Проверка кода подтверждения."""
    data = request.json
    user_id = data.get('user_id')
    code = data.get('code', '').strip()

    user = db.get_user_by_id(user_id)
    if not user:
        return jsonify({"error": "Пользователь не найден"}), 404

    if user['verification_code'] != code:
        return jsonify({"error": "Неверный код"}), 400

    db.verify_user(user_id)
    return jsonify({"message": "Аккаунт подтверждён", "user_id": user_id})


@app.route('/api/login', methods=['POST'])
def login():
    """Вход в аккаунт."""
    data = request.json
    login_field = data.get('login', '').strip().lower()
    password = data.get('password', '')

    if '@' in login_field:
        user = db.get_user_by_email(login_field)
    else:
        user = db.get_user_by_phone(login_field)

    if not user:
        return jsonify({"error": "Пользователь не найден"}), 404

    if user['password_hash'] != hash_password(password):
        return jsonify({"error": "Неверный пароль"}), 401

    if not user['is_verified']:
        return jsonify({"error": "Аккаунт не подтверждён"}), 403

    return jsonify({
        "user_id": user['id'],
        "username": user['username'],
        "native_language": user['native_language'],
        "country": user['country']
    })


@app.route('/api/users', methods=['GET'])
def users():
    """Список онлайн-пользователей."""
    country = request.args.get('country', '')
    users_list = db.get_online_users()
    if country:
        users_list = [u for u in users_list if u['country'] == country]
    return jsonify(users_list)


@app.route('/api/countries', methods=['GET'])
def countries():
    """Список стран из БД."""
    return jsonify(db.get_all_countries())


@app.route('/api/heartbeat', methods=['POST'])
def heartbeat():
    """Обновление онлайн-статуса."""
    data = request.json
    user_id = data.get('user_id')
    if not user_id:
        return jsonify({"error": "user_id обязателен"}), 400
    db.set_user_online(user_id)
    return jsonify({"status": "ok"})


@app.route('/api/avatar/<int:user_id>', methods=['GET'])
def get_avatar(user_id):
    """Отдаёт аватар пользователя."""
    user = db.get_user_by_id(user_id)
    if not user or not user['avatar_path']:
        return jsonify({"error": "Аватар не найден"}), 404
    return send_from_directory('avatars', user['avatar_path'])


# ============================================================
# АВАТАРКИ
# ============================================================
AVATAR_DIR = os.path.join(os.path.dirname(__file__), 'avatars')
os.makedirs(AVATAR_DIR, exist_ok=True)


@app.route('/api/upload_avatar', methods=['POST'])
def upload_avatar():
    """Загружает аватарку пользователя."""
    if 'avatar' not in request.files:
        return jsonify({"error": "Файл не передан"}), 400

    user_id = request.form.get('user_id', type=int)
    if not user_id:
        return jsonify({"error": "user_id обязателен"}), 400

    file = request.files['avatar']
    if file.filename == '':
        return jsonify({"error": "Пустое имя файла"}), 400

    ext = os.path.splitext(file.filename)[1].lower()
    if ext not in ['.png', '.jpg', '.jpeg', '.gif', '.bmp']:
        return jsonify({"error": "Неверный формат файла"}), 400

    filename = f"user_{user_id}_{uuid.uuid4().hex[:8]}{ext}"
    filepath = os.path.join(AVATAR_DIR, filename)

    user = db.get_user_by_id(user_id)
    if user and user['avatar_path']:
        old_path = os.path.join(AVATAR_DIR, user['avatar_path'])
        if os.path.exists(old_path):
            try:
                os.remove(old_path)
            except Exception as e:
                print(f"[AVATAR] Не удалось удалить старую: {e}")

    file.save(filepath)

    conn = db.get_connection()
    cursor = conn.cursor()
    cursor.execute("UPDATE users SET avatar_path = ? WHERE id = ?",
                   (filename, user_id))
    conn.commit()
    conn.close()

    print(f"[AVATAR] Обновлена для user_id={user_id}: {filename}")
    return jsonify({"status": "ok", "avatar_path": filename})


@app.route('/api/update_profile', methods=['POST'])
def update_profile():
    """Обновляет данные профиля пользователя."""
    data = request.json
    user_id = data.get('user_id')
    username = data.get('username', '').strip()
    country = data.get('country', '').strip()
    native_language = data.get('native_language', '').strip()
    birthdate = data.get('birthdate', '').strip()

    if not user_id or not username:
        return jsonify({"error": "user_id и username обязательны"}), 400

    conn = db.get_connection()
    cursor = conn.cursor()
    cursor.execute("SELECT id FROM users WHERE username = ? AND id != ?",
                   (username, user_id))
    if cursor.fetchone():
        conn.close()
        return jsonify({"error": "Никнейм уже занят"}), 409

    cursor.execute('''
        UPDATE users
        SET username = ?, country = ?, native_language = ?, birthdate = ?
        WHERE id = ?
    ''', (username, country, native_language, birthdate, user_id))
    conn.commit()
    conn.close()

    print(f"[PROFILE] Обновлён user_id={user_id}")
    return jsonify({"status": "ok"})


# ============================================================
# WEBSOCKET
# ============================================================

connections = {}       # user_id -> websocket
active_pairs = {}      # user_id -> partner_user_id
waiting_queue = []     # очередь ожидающих случайного собеседника


def send_event(ws, event, data=None):
    """Отправить событие клиенту."""
    payload = {"event": event}
    if data:
        payload.update(data)
    try:
        ws.send(json.dumps(payload, ensure_ascii=False))
    except Exception as e:
        print(f"[WS] Ошибка отправки: {e}")


def send_to_user(user_id, event, data=None):
    """Отправить событие конкретному пользователю."""
    if user_id in connections:
        send_event(connections[user_id], event, data)
        return True
    return False


@sock.route('/ws')
def websocket_handler(ws):
    """Обработчик WebSocket-подключения."""
    print(f"[WS] Клиент подключился")
    user_id = None

    try:
        while True:
            raw = ws.receive()
            if raw is None:
                break

            try:
                msg = json.loads(raw)
            except json.JSONDecodeError:
                print(f"[WS] Некорректный JSON: {raw}")
                continue

            event = msg.get('event')
            print(f"[WS] Получено: {event} от user_id={user_id}")

            # --- Регистрация ---
            if event == 'register':
                user_id = msg.get('user_id')
                connections[user_id] = ws
                print(f"[WS] Зарегистрирован user_id={user_id}")
                send_event(ws, 'registered', {'user_id': user_id})

            # --- Приглашение ---
            elif event == 'invite_user':
                from_id = msg.get('from_id')
                to_id = msg.get('to_id')

                if to_id not in connections:
                    send_event(ws, 'invite_failed',
                               {'reason': 'Пользователь не в сети'})
                    continue

                from_user = db.get_user_by_id(from_id)
                if not from_user:
                    send_event(ws, 'invite_failed',
                               {'reason': 'Отправитель не найден'})
                    continue

                send_to_user(to_id, 'incoming_invite', {
                    'from_id': from_id,
                    'from_username': from_user['username'],
                    'from_country': from_user['country'],
                    'from_language': from_user['native_language'],
                })

            # --- Ответ на приглашение ---
            elif event == 'invite_response':
                from_id = msg.get('from_id')
                to_id = msg.get('to_id')
                accepted = msg.get('accepted', False)

                if not accepted:
                    send_to_user(from_id, 'invite_rejected',
                                 {'by_user_id': to_id})
                    continue

                active_pairs[from_id] = to_id
                active_pairs[to_id] = from_id

                a_user = db.get_user_by_id(from_id)
                b_user = db.get_user_by_id(to_id)

                send_to_user(from_id, 'call_started', {
                    'partner_id': to_id,
                    'partner_username': b_user['username'],
                    'partner_country': b_user['country'],
                    'partner_language': b_user['native_language'],
                })

                send_to_user(to_id, 'call_started', {
                    'partner_id': from_id,
                    'partner_username': a_user['username'],
                    'partner_country': a_user['country'],
                    'partner_language': a_user['native_language'],
                })

            # --- Поиск случайного собеседника ---
            elif event == 'find_random':
                uid = msg.get('user_id')
                if not uid:
                    continue

                if uid in waiting_queue:
                    waiting_queue.remove(uid)

                partner_id = None
                for candidate in waiting_queue:
                    if candidate in connections and candidate != uid:
                        partner_id = candidate
                        break

                if partner_id:
                    waiting_queue.remove(partner_id)
                    active_pairs[uid] = partner_id
                    active_pairs[partner_id] = uid

                    a_user = db.get_user_by_id(uid)
                    b_user = db.get_user_by_id(partner_id)

                    print(f"[WS] Случайная пара: {uid} <-> {partner_id}")

                    send_to_user(uid, 'call_started', {
                        'partner_id': partner_id,
                        'partner_username': b_user['username'],
                        'partner_country': b_user['country'],
                        'partner_language': b_user['native_language'],
                    })

                    send_to_user(partner_id, 'call_started', {
                        'partner_id': uid,
                        'partner_username': a_user['username'],
                        'partner_country': a_user['country'],
                        'partner_language': a_user['native_language'],
                    })
                else:
                    waiting_queue.append(uid)
                    print(f"[WS] {uid} добавлен в очередь ({len(waiting_queue)} чел.)")
                    send_event(ws, 'waiting', {'position': len(waiting_queue)})

            # --- Отмена поиска ---
            elif event == 'cancel_search':
                uid = msg.get('user_id')
                if uid in waiting_queue:
                    waiting_queue.remove(uid)
                    print(f"[WS] {uid} покинул очередь")
                send_event(ws, 'search_cancelled', {})

            # --- Перевод ---
            elif event == 'translation':
                from_id = msg.get('from_id')
                text = msg.get('text')
                language = msg.get('language')

                partner_id = active_pairs.get(from_id)

                if partner_id:
                    print(f"[WS] Пересылаю перевод {from_id} -> {partner_id}: {text[:60]}")
                    send_to_user(partner_id, 'incoming_translation', {
                        'from_id': from_id,
                        'text': text,
                        'language': language,
                    })
                else:
                    print(f"[WS] Нет партнёра для user_id={from_id}")

            # --- Завершение разговора ---
            elif event == 'end_call':
                uid = msg.get('user_id')
                partner_id = active_pairs.get(uid)

                if partner_id:
                    send_to_user(partner_id, 'partner_disconnected',
                                 {'user_id': uid})
                    active_pairs.pop(uid, None)
                    active_pairs.pop(partner_id, None)

    except ConnectionClosed:
        print(f"[WS] Соединение закрыто (user_id={user_id})")
    except Exception as e:
        print(f"[WS] Ошибка: {e}")
    finally:
        # Убираем из очереди
        if user_id and user_id in waiting_queue:
            waiting_queue.remove(user_id)
            print(f"[WS] {user_id} удалён из очереди (отключение)")

        if user_id and user_id in connections:
            del connections[user_id]
            print(f"[WS] Отключён user_id={user_id}")

            partner_id = active_pairs.get(user_id)
            if partner_id:
                send_to_user(partner_id, 'partner_disconnected',
                             {'user_id': user_id})
                active_pairs.pop(user_id, None)
                active_pairs.pop(partner_id, None)


# ============================================================
# ЗАПУСК
# ============================================================
if __name__ == '__main__':
    db.init_db()
    print("[SERVER] Запуск на http://0.0.0.0:5000")
    app.run(host='0.0.0.0', port=5000, debug=True, use_reloader=False)