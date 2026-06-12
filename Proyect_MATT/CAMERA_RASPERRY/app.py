"""
MATT vision service.

Runs on the Raspberry Pi, captures camera frames, detects writing on the
board, and publishes both the frame and detections to Firebase for the web UI.
There is no local Flask/debug server in this version.
"""

import base64
import json
import os
import threading
import time
import urllib.request

import cv2
import numpy as np


# ============================================================================
# CONFIG
# ============================================================================
FRAME_W, FRAME_H = 1280, 720
PUSH_W, PUSH_H = 640, 360
CORNERS_FILE = "corners.json"
REFERENCE_FILE = "reference.jpg"

FIREBASE_DB = "https://AQUI_VA_TU_PROYECTO.firebaseio.com"

BOARD_W_MM_DEFAULT = 1200.0
BOARD_H_MM_DEFAULT = 900.0

THRESHOLD = 12
MIN_AREA = 25
MAX_AREA_RATIO = 0.35
CLOSE_KERNEL = 13
OPEN_KERNEL = 2
DILATE_KERNEL = 3
BOARD_EDGE_IGNORE_PX = 45
STABLE_OVERLAP_RATIO = 0.25

FRAME_PUSH_INTERVAL = 2.5
DETECTION_PUSH_INTERVAL = 1.5
CONFIG_PULL_INTERVAL = 5.0


# ============================================================================
# CAMERA / STATE
# ============================================================================
cap = cv2.VideoCapture(0)
cap.set(cv2.CAP_PROP_FRAME_WIDTH, FRAME_W)
cap.set(cv2.CAP_PROP_FRAME_HEIGHT, FRAME_H)

frame_lock = threading.Lock()
latest_frame = None
reference_gray = None

board_corners = None
board_mask = None
board_width_mm = BOARD_W_MM_DEFAULT
board_height_mm = BOARD_H_MM_DEFAULT
homography = None
vision_mode = "automatico"

last_frame_push = 0.0
last_detection_push = 0.0
last_reference_request = 0
previous_regions = []


# ============================================================================
# FIREBASE
# ============================================================================
def fb_request(path, data=None, method="GET"):
    url = f"{FIREBASE_DB}{path}.json"
    body = json.dumps(data).encode("utf-8") if data is not None else None
    headers = {"Content-Type": "application/json"} if body else {}
    req = urllib.request.Request(url, data=body, method=method, headers=headers)

    try:
        with urllib.request.urlopen(req, timeout=6) as response:
            raw = response.read().decode("utf-8")
            return json.loads(raw) if raw and raw != "null" else None
    except Exception as exc:
        print(f"Firebase {method} {path} error: {exc}")
        return None


def push_frame_firebase(frame):
    small = cv2.resize(frame, (PUSH_W, PUSH_H))
    ok, jpg = cv2.imencode(".jpg", small, [cv2.IMWRITE_JPEG_QUALITY, 60])
    if not ok:
        return

    fb_request("/robot/vision/frame", data={
        "timestamp": int(time.time() * 1000),
        "image": base64.b64encode(jpg.tobytes()).decode("utf-8"),
        "width": PUSH_W,
        "height": PUSH_H,
    }, method="PUT")


def push_detections_firebase(regions):
    detecciones = []

    for x, y, w, h, area in regions:
        cx = x + w / 2
        cy = y + h / 2
        x_mm, y_mm = pixel_to_mm(cx, cy)

        detecciones.append({
            "x": int(x),
            "y": int(y),
            "w": int(w),
            "h": int(h),
            "area": int(area),
            "cx": float(cx),
            "cy": float(cy),
            "x_mm": x_mm,
            "y_mm": y_mm,
            "radius_mm": estimate_radius_mm(x, y, w, h),
        })

    fb_request("/robot/vision/detecciones", data={
        "timestamp": int(time.time() * 1000),
        "count": len(detecciones),
        "detecciones": detecciones,
        "ready": homography is not None and board_mask is not None,
    }, method="PUT")


def push_vision_status(status, message):
    fb_request("/robot/vision/status", data={
        "timestamp": int(time.time() * 1000),
        "status": status,
        "message": message,
    }, method="PUT")


def acknowledge_reference_request(request_id):
    fb_request("/robot/vision/control/referenceStatus", data={
        "timestamp": int(time.time() * 1000),
        "requestId": request_id,
        "status": "recibido",
        "message": "Solicitud recibida. Capturando nueva imagen base..."
    }, method="PUT")


def complete_reference_request(request_id, ok, message):
    fb_request("/robot/vision/control/referenceStatus", data={
        "timestamp": int(time.time() * 1000),
        "requestId": request_id,
        "status": "ok" if ok else "error",
        "message": message,
    }, method="PUT")


def capture_reference_from_latest():
    global reference_gray

    if board_mask is None:
        return False, "Primero marca las esquinas/margen del pizarron."

    with frame_lock:
        frame = latest_frame.copy() if latest_frame is not None else None

    if frame is None:
        return False, "Todavia no hay frame de camara para capturar referencia."

    gray = cv2.cvtColor(frame, cv2.COLOR_BGR2GRAY)
    gray = cv2.GaussianBlur(gray, (5, 5), 0)
    reference_gray = cv2.bitwise_and(gray, board_mask)
    cv2.imwrite(REFERENCE_FILE, frame)
    return True, "Imagen base recapturada. Ya puedes escribir o mover objetos."


def load_config_from_firebase():
    global board_corners, board_mask, board_width_mm, board_height_mm
    global reference_gray
    global last_reference_request
    global vision_mode

    config = fb_request("/robot/vision/config")
    if not config:
        return

    board_data = config.get("board")
    if board_data:
        board_width_mm = float(board_data.get("width_mm", board_width_mm))
        board_height_mm = float(board_data.get("height_mm", board_height_mm))

    corners_data = config.get("corners")
    if corners_data and "points" in corners_data:
        raw_points = corners_data["points"]
        img_w = float(corners_data.get("image_width", PUSH_W))
        img_h = float(corners_data.get("image_height", PUSH_H))
        sx = FRAME_W / img_w
        sy = FRAME_H / img_h

        scaled_points = [
            [int(round(float(p[0]) * sx)), int(round(float(p[1]) * sy))]
            for p in raw_points
        ]

        if len(scaled_points) == 4 and scaled_points != board_corners:
            board_corners = scaled_points
            board_mask = np.zeros((FRAME_H, FRAME_W), dtype=np.uint8)
            cv2.fillPoly(board_mask, [np.array(board_corners, dtype=np.int32)], 255)

            with open(CORNERS_FILE, "w", encoding="utf-8") as file:
                json.dump(board_corners, file)

            print(f"Corners from Firebase: {board_corners}")
            reference_gray = None
            if os.path.exists(REFERENCE_FILE):
                os.remove(REFERENCE_FILE)
            push_vision_status(
                "esperando_referencia",
                "Margen guardado. Limpia el pizarron y presiona Recapturar imagen base."
            )
    else:
        if board_corners is not None or board_mask is not None:
            print("Waiting for board margin/corners from Firebase")
        board_corners = None
        board_mask = None
        reference_gray = None

    update_homography()

    control = fb_request("/robot/vision/control")
    if control:
        mode = control.get("mode")
        if mode in ("automatico", "manual"):
            vision_mode = mode

        request_id = int(control.get("recaptureReference") or 0)
        if request_id and request_id != last_reference_request:
            last_reference_request = request_id
            acknowledge_reference_request(request_id)
            ok, message = capture_reference_from_latest()
            complete_reference_request(request_id, ok, message)
            push_vision_status("referencia" if ok else "error_referencia", message)


# ============================================================================
# GEOMETRY
# ============================================================================
def update_homography():
    global homography

    if board_corners is None or len(board_corners) != 4:
        homography = None
        return

    src = np.array(board_corners, dtype=np.float32)
    dst = np.array([
        [0, 0],
        [board_width_mm, 0],
        [board_width_mm, board_height_mm],
        [0, board_height_mm],
    ], dtype=np.float32)

    homography = cv2.getPerspectiveTransform(src, dst)


def pixel_to_mm(cx, cy):
    if homography is None:
        return None, None

    point = np.array([[[float(cx), float(cy)]]], dtype=np.float32)
    converted = cv2.perspectiveTransform(point, homography)
    return float(converted[0][0][0]), float(converted[0][0][1])


def estimate_radius_mm(x, y, w, h):
    if homography is None:
        return 30

    cx = x + w / 2
    cy = y + h / 2
    left_mm, _ = pixel_to_mm(cx - w / 2, cy)
    right_mm, _ = pixel_to_mm(cx + w / 2, cy)

    if left_mm is None or right_mm is None:
        return 30

    return max(10, min(200, int(abs(right_mm - left_mm) / 2)))


# ============================================================================
# PERSISTENCE
# ============================================================================
def load_local_state():
    global board_corners, board_mask, reference_gray

    if os.path.exists(CORNERS_FILE):
        try:
            with open(CORNERS_FILE, encoding="utf-8") as file:
                board_corners = json.load(file)

            board_mask = np.zeros((FRAME_H, FRAME_W), dtype=np.uint8)
            cv2.fillPoly(board_mask, [np.array(board_corners, dtype=np.int32)], 255)
            print(f"Loaded saved corners: {board_corners}")
        except Exception as exc:
            print(f"Could not load {CORNERS_FILE}: {exc}")

    if os.path.exists(REFERENCE_FILE):
        ref = cv2.imread(REFERENCE_FILE)
        if ref is not None and ref.shape[:2] == (FRAME_H, FRAME_W):
            gray = cv2.cvtColor(ref, cv2.COLOR_BGR2GRAY)
            reference_gray = cv2.GaussianBlur(gray, (5, 5), 0)
            if board_mask is not None:
                reference_gray = cv2.bitwise_and(reference_gray, board_mask)
            print(f"Loaded existing {REFERENCE_FILE}")

    update_homography()


# ============================================================================
# VISION
# ============================================================================
def capture_loop():
    global latest_frame

    while True:
        ret, frame = cap.read()

        if not ret:
            time.sleep(0.05)
            continue

        with frame_lock:
            latest_frame = frame


def detect_writing(frame):
    if reference_gray is None or board_mask is None or homography is None:
        return []

    detection_mask = board_mask.copy()
    if BOARD_EDGE_IGNORE_PX > 0:
        erode_kernel = np.ones((BOARD_EDGE_IGNORE_PX, BOARD_EDGE_IGNORE_PX), np.uint8)
        detection_mask = cv2.erode(detection_mask, erode_kernel, iterations=1)

    gray = cv2.cvtColor(frame, cv2.COLOR_BGR2GRAY)
    gray = cv2.GaussianBlur(gray, (5, 5), 0)
    gray = cv2.bitwise_and(gray, detection_mask)

    diff = cv2.absdiff(reference_gray, gray)
    _, mask = cv2.threshold(diff, THRESHOLD, 255, cv2.THRESH_BINARY)
    mask = cv2.bitwise_and(mask, detection_mask)

    k_close = np.ones((CLOSE_KERNEL, CLOSE_KERNEL), np.uint8)
    k_open = np.ones((OPEN_KERNEL, OPEN_KERNEL), np.uint8)
    k_dilate = np.ones((DILATE_KERNEL, DILATE_KERNEL), np.uint8)

    mask = cv2.morphologyEx(mask, cv2.MORPH_CLOSE, k_close)
    mask = cv2.morphologyEx(mask, cv2.MORPH_OPEN, k_open)
    mask = cv2.dilate(mask, k_dilate, iterations=1)

    contours, _ = cv2.findContours(mask, cv2.RETR_EXTERNAL, cv2.CHAIN_APPROX_SIMPLE)
    regions = []
    max_area = FRAME_W * FRAME_H * MAX_AREA_RATIO

    for contour in contours:
        area = cv2.contourArea(contour)
        if MIN_AREA <= area <= max_area:
            x, y, w, h = cv2.boundingRect(contour)
            regions.append((x, y, w, h, int(area)))

    regions.sort(key=lambda region: region[4], reverse=True)
    return regions


def overlap_ratio(a, b):
    ax, ay, aw, ah, _ = a
    bx, by, bw, bh, _ = b

    x1 = max(ax, bx)
    y1 = max(ay, by)
    x2 = min(ax + aw, bx + bw)
    y2 = min(ay + ah, by + bh)

    if x2 <= x1 or y2 <= y1:
        return 0.0

    intersection = (x2 - x1) * (y2 - y1)
    smaller_area = min(aw * ah, bw * bh)
    if smaller_area <= 0:
        return 0.0

    return intersection / smaller_area


def stable_regions(current_regions):
    if not previous_regions:
        return []

    stable = []
    for region in current_regions:
        if any(overlap_ratio(region, prev) >= STABLE_OVERLAP_RATIO for prev in previous_regions):
            stable.append(region)

    return stable


# ============================================================================
# BACKGROUND LOOPS
# ============================================================================
def config_loop():
    while True:
        load_config_from_firebase()
        time.sleep(CONFIG_PULL_INTERVAL)


def firebase_push_loop():
    global last_frame_push, last_detection_push, previous_regions

    while True:
        now = time.time()

        with frame_lock:
            frame = latest_frame.copy() if latest_frame is not None else None

        if frame is None:
            time.sleep(0.2)
            continue

        if now - last_frame_push >= FRAME_PUSH_INTERVAL:
            push_frame_firebase(frame)
            last_frame_push = now

        if now - last_detection_push >= DETECTION_PUSH_INTERVAL:
            if vision_mode == "manual":
                push_detections_firebase([])
                push_vision_status(
                    "manual",
                    "Modo manual: arrastra un área sobre la imagen para borrar un punto."
                )
                last_detection_push = now
                time.sleep(0.2)
                continue

            if board_mask is None or homography is None:
                push_detections_firebase([])
                push_vision_status(
                    "esperando_margen",
                    "Marca primero las esquinas/margen del pizarron en la pagina."
                )
                last_detection_push = now
                time.sleep(0.2)
                continue

            if reference_gray is None:
                push_detections_firebase([])
                push_vision_status(
                    "esperando_referencia",
                    "Referencia no capturada todavia. Deja el pizarron limpio."
                )
                last_detection_push = now
                time.sleep(0.2)
                continue

            regions = detect_writing(frame)
            stable = stable_regions(regions)
            previous_regions = regions
            push_detections_firebase(stable)
            push_vision_status("detectando", f"{len(stable)} region(es) detectada(s).")
            last_detection_push = now

        time.sleep(0.2)


def main():
    print("Iniciando MATT Vision Firebase...")

    if not cap.isOpened():
        print("ERROR: No se pudo abrir la camara.")
        raise SystemExit(1)

    load_local_state()
    load_config_from_firebase()

    threading.Thread(target=capture_loop, daemon=True).start()
    threading.Thread(target=config_loop, daemon=True).start()
    threading.Thread(target=firebase_push_loop, daemon=True).start()

    print("MATT Vision corriendo. Subiendo camara y detecciones a Firebase.")
    print("No hay servidor local. Todo se comunica por Firebase.")
    print("Inicia con el pizarron limpio si quieres referencia automatica nueva.")

    try:
        while True:
            time.sleep(1)
    except KeyboardInterrupt:
        print("\nCerrando MATT Vision...")
    finally:
        cap.release()


if __name__ == "__main__":
    main()
