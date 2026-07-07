import sys
import os
os.environ["RKNN_LOG_LEVEL"] = "0"
os.environ["RKNN_DISABLE_WARN"] = "1"
import time
import numpy as np
import cv2
import socket
import struct
import json

from rknnpool_ld import rknnPoolExecutor
from db_postprocess import DBPostProcess, DetPostProcess
from operators import DetResizeForTest

DET_MODEL_PATH = "/home/elf/paddle/paddle_det.rknn"

SERVER_HOST = "127.0.0.1"
SERVER_PORT = 9000
_last_state = None

GPIO_NUM = 8
GPIO_PATH = f"/sys/class/gpio/gpio{GPIO_NUM}"
GPIO_STOP_VALUE = "1"
GPIO_START_VALUE = "0"
DEBUG_NO_CONVEYOR = True        # 实际接传送带时改 False

CAMERA_DEV = "/dev/video31"
PREVIEW_W = 1920
PREVIEW_H = 1080

CENTER_MIN_RATIO = 0.40
CENTER_MAX_RATIO = 0.60
EDGE_MARGIN_RATIO = 0.06        # 左右边缘至少留 6%
CONFIRM_FRAMES = 3
DETECT_INTERVAL = 0.08
MIN_TEXT_BOXES = 1              

GRAB_FRAME_BIN = "/home/elf/paddle/grab_frame2"
PREVIEW_GRAY_PATH = "/home/elf/paddle/preview.gray"

det_resize = DetResizeForTest(image_shape=[640, 640])
db_postprocess = DBPostProcess(thresh=0.3, box_thresh=0.6, max_candidates=1000, unclip_ratio=1.6)
det_postprocess = DetPostProcess()

def send_state(state: int):
    global _last_state

    if _last_state == state:
        return

    _last_state = state

    send_to_uvgl({
        "type": "detector",
        "state": state
    })


def send_to_uvgl(data: dict):
    try:
        payload = json.dumps(data, ensure_ascii=False).encode("utf-8")
        header  = struct.pack("!I", len(payload))

        s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        s.settimeout(5)
        s.connect((SERVER_HOST, SERVER_PORT))
        s.sendall(header + payload)
        s.close()

        print(f"[uvgl] 发送成功: {data}")

    except ConnectionRefusedError:
        print("[uvgl] 连接被拒绝")
    except socket.timeout:
        print("[uvgl] 超时")
    except Exception as e:
        print(f"[uvgl] 错误: {e}")


def gpio_init():
    if DEBUG_NO_CONVEYOR:
        print("[GPIO] 调试模式：不操作 GPIO")
        return True

    try:
        if not os.path.exists(GPIO_PATH):
            with open("/sys/class/gpio/export", "w") as f:
                f.write(str(GPIO_NUM))
            time.sleep(0.05)
        with open(f"{GPIO_PATH}/direction", "w") as f:
            f.write("out")
        print(f"[GPIO] gpio{GPIO_NUM} 初始化完成")
        return True
    except OSError as e:
        print(f"[GPIO] 初始化失败: {e}")
        return False


def gpio_write(value):
    if DEBUG_NO_CONVEYOR:
        return
    try:
        with open(f"{GPIO_PATH}/value", "w") as f:
            f.write(value)
    except OSError as e:
        print(f"[GPIO] 写入失败: {e}")


def conveyor_start():
    print("[传送带] 启动" if not DEBUG_NO_CONVEYOR else "[传送带] 模拟启动")
    gpio_write(GPIO_START_VALUE)


def conveyor_stop():
    print("[传送带] 停止" if not DEBUG_NO_CONVEYOR else "[传送带] 模拟停止")
    gpio_write(GPIO_STOP_VALUE)


def grab_preview_frame():
    os.system(f"fuser -k {CAMERA_DEV} 2>/dev/null")
    time.sleep(0.1)

    ret = os.system(GRAB_FRAME_BIN)
    if ret != 0:
        print(f"[camera] grab_frame2 执行失败，返回码: {ret}")
        return None

    if not os.path.exists(PREVIEW_GRAY_PATH):
        print(f"[camera] 找不到输出文件 {PREVIEW_GRAY_PATH}")
        return None

    data = open(PREVIEW_GRAY_PATH, "rb").read()
    expected = PREVIEW_H * PREVIEW_W
    if len(data) != expected:
        print(f"[camera] 预览图大小异常：期望 {expected}，实际 {len(data)}")
        return None

    gray = np.frombuffer(data, dtype=np.uint8).reshape((PREVIEW_H, PREVIEW_W))
    frame = cv2.cvtColor(gray, cv2.COLOR_GRAY2BGR)
    return frame


def det_inference_func(rknn_instance, frame):
    pre_res = det_resize({"image": frame})
    img = pre_res["image"]
    if img.ndim == 3:
        img = img[np.newaxis]

    outputs = rknn_instance.inference(inputs=[img])
    if not outputs:
        return np.array([]), frame

    preds = {"maps": outputs[0].astype(np.float32)}
    result = db_postprocess(preds, pre_res["shape"])
    boxes = det_postprocess.filter_tag_det_res(result[0]["points"], frame.shape)
    return boxes, frame


def get_text_position(frame, det_pipeline):
    det_pipeline.put(frame)
    det_res = det_pipeline.get()
    if not det_res[1]:
        return None

    boxes = det_res[0][0]
    if len(boxes) < MIN_TEXT_BOXES:
        return None

    pts = np.array(boxes, dtype=np.float32).reshape(-1, 2)
    w = frame.shape[1]
    x_min = float(pts[:, 0].min()) / w
    x_max = float(pts[:, 0].max()) / w
    center_x = (x_min + x_max) / 2.0
    return x_min, x_max, center_x, len(boxes)


def should_stop(pos):
    if pos is None:
        return False
    x_min, x_max, center_x, _ = pos
    center_ok = CENTER_MIN_RATIO <= center_x <= CENTER_MAX_RATIO
    fully_entered = x_min >= EDGE_MARGIN_RATIO and x_max <= 1.0 - EDGE_MARGIN_RATIO
    return center_ok and fully_entered


def wait_stamp_to_center(det_pipeline):
    ok_count = 0
    print("[detector] 等待角铁钢印进入画面中央...")

    while True:
        frame = grab_preview_frame()
        if frame is None:
            print("[camera] 取帧失败")
            return False

        pos = get_text_position(frame, det_pipeline)
        if should_stop(pos):
            ok_count += 1
            x_min, x_max, center_x, box_count = pos
            print(f"[detector] 居中 {ok_count}/{CONFIRM_FRAMES}: center={center_x:.2f}, x={x_min:.2f}~{x_max:.2f}, boxes={box_count}")
            send_state(3)
            if ok_count >= CONFIRM_FRAMES:
                return True
        else:
            ok_count = 0
            if pos is None:
                print("[detector] 未识别到钢印文字")
                send_state(1)
            else:
                x_min, x_max, center_x, box_count = pos
                print(f"[detector] 继续: center={center_x:.2f}, x={x_min:.2f}~{x_max:.2f}, boxes={box_count}")
                send_state(2)
        time.sleep(DETECT_INTERVAL)


def main():
    if not gpio_init():
        sys.exit(1)

    det_pipeline = None
    try:
        print("[detector] 加载模型...")
        det_pipeline = rknnPoolExecutor(DET_MODEL_PATH, TPEs=4, func=det_inference_func)

        conveyor_start()
        send_to_uvgl({"event": "conveyor_start", "msg": "传送带已启动"})

        if wait_stamp_to_center(det_pipeline):
            msg = "钢印已基本进入中央，停止传送带"
            print(f"[detector] {msg}")
            send_state(4)
            #send_to_uvgl({"event": "conveyor_stop", "msg": msg})
        else:
            msg = "异常退出，停止传送带"
            print(f"[detector] {msg}")
            send_to_uvgl({"event": "conveyor_stop", "msg": msg, "error": True})
        conveyor_stop()

    except KeyboardInterrupt:
        print("\n[detector] 中断，停止传送带")
        conveyor_stop()
        send_to_uvgl({"event": "interrupted", "msg": "用户中断"})
    finally:
        if det_pipeline is not None:
            det_pipeline.release()
        print("[detector] 模型资源已释放")


if __name__ == "__main__":
    main()
