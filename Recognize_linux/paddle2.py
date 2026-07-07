"""
调用方式：
  python3 paddle2.py
  退出码 0 = 正常完成
  退出码 1 = 文件读取失败
"""
import sys
import os
os.environ["RKNN_LOG_LEVEL"] = "0"
os.environ["RKNN_DISABLE_WARN"] = "1"
import time
import numpy as np
import cv2
import re
import socket
import struct
import json

from rknnpool_ld import rknnPoolExecutor
from db_postprocess import DBPostProcess, DetPostProcess
from operators import DetResizeForTest

DET_MODEL_PATH = "/home/elf/paddle/paddle_det.rknn"
REC_MODEL_PATH = "/home/elf/paddle/paddle_rec.rknn"
GRAY_PATH      = "/home/elf/HCimage.gray"
GRAY_H         = 1080
GRAY_W         = 1920

SERVER_HOST = "127.0.0.1"
SERVER_PORT = 9000

BUZZER_GPIO_NUM      = 89
BUZZER_GPIO_PATH     = f"/sys/class/gpio/gpio{BUZZER_GPIO_NUM}"
BUZZER_ON_VALUE      = "1"
BUZZER_OFF_VALUE     = "0"
BUZZER_BEEP_DURATION = 0.5      #蜂鸣器时长
DEBUG_NO_BUZZER      = True     # 没有蜂鸣器时 True改 False

CHAR_CONF_THRESHOLD  = 0.60    #置信度多少

fifo_path = "/home/elf/my_fifo"

det_resize     = DetResizeForTest(image_shape=[640, 640])
db_postprocess = DBPostProcess(thresh=0.3, box_thresh=0.6,
                               max_candidates=1000, unclip_ratio=1.6)
det_postprocess = DetPostProcess()

CHAR_TABLE = (
    None,
    '0','1','2','3','4','5','6','7','8','9',
    ':',';','<','=','>','?','@',
    'A','B','C','D','E','F','G','H','I','J','K','L','M',
    'N','O','P','Q','R','S','T','U','V','W','X','Y','Z',
    '[','\\',']','^','_','`',
    'a','b','c','d','e','f','g','h','i','j','k','l','m',
    'n','o','p','q','r','s','t','u','v','w','x','y','z',
    '{','|','}','~','!','"','#','$','%','&',"'",'(',')','*','+',',','-','.','/',
    ' ',
)
CHAR_TABLE_LEN = len(CHAR_TABLE)
_RE_VALID = re.compile(r'[A-Za-z0-9\-\s]{2,}')
REC_W, REC_H = 320, 48

def fifos():
    with open(fifo_path, "r") as fifo:
        print("等待写入端连接并发送数据...")
        while True:
            data = fifo.read(1024)   
            if data == "":           
                break
            print(f"收到: {data.strip()}")
    print("所有数据已读完，写入端已关闭。")

def buzzer_init():
    if DEBUG_NO_BUZZER:
        print("[蜂鸣器] 调试模式：不操作蜂鸣器")
        return True

    if not os.path.exists(BUZZER_GPIO_PATH):
        try:
            with open("/sys/class/gpio/export", "w") as f:
                f.write(str(BUZZER_GPIO_NUM))
            time.sleep(0.05)
        except OSError as e:
            print(f"[蜂鸣器] GPIO export 失败: {e}")
            return False

    try:
        with open(f"{BUZZER_GPIO_PATH}/direction", "w") as f:
            f.write("out")
        with open(f"{BUZZER_GPIO_PATH}/value", "w") as f:
            f.write(BUZZER_OFF_VALUE)
    except OSError as e:
        print(f"[蜂鸣器] 初始化写入失败: {e}")
        return False

    print(f"[蜂鸣器] 初始化完成，引脚 gpio{BUZZER_GPIO_NUM}")
    return True


def buzzer_beep():
    if DEBUG_NO_BUZZER:
        print("[蜂鸣器] 模拟报警")
        return
    try:
        with open(f"{BUZZER_GPIO_PATH}/value", "w") as f:
            f.write(BUZZER_ON_VALUE)
        time.sleep(BUZZER_BEEP_DURATION)
        with open(f"{BUZZER_GPIO_PATH}/value", "w") as f:
            f.write(BUZZER_OFF_VALUE)
        print("[蜂鸣器] 低置信度警告")
    except OSError as e:
        print(f"[蜂鸣器] 写入失败: {e}")


def send_ocr_results(ocr_results: list):
    items = []
    low_conf_chars = []
    need_beep = False

    for item in ocr_results:
        box, text, char_confs = item     
        if text.startswith("空信号"):
            continue

        chars_payload = []
        for ch, conf in char_confs:
            chars_payload.append({"c": ch, "conf": round(conf, 5)})
            if conf < CHAR_CONF_THRESHOLD:
                low_conf_chars.append({"c": ch, "conf": round(conf, 5)})
                need_beep = True
                print(f"[置信度] 字符 '{ch}' 置信度 {conf:.5f} 低于阈值 {CHAR_CONF_THRESHOLD}")

        items.append({
            "text": text,
            "chars": chars_payload,
        })

    if need_beep:
        buzzer_beep()

    if not items:
        print(" 无有效识别结果，跳过发送")
        return

    payload = json.dumps({"type":"ocr","items": items, "low_conf": low_conf_chars},
                         ensure_ascii=False).encode("utf-8")
    header = struct.pack("!I", len(payload))

    try:
        s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        s.settimeout(5)
        s.connect((SERVER_HOST, SERVER_PORT))
        s.sendall(header + payload)
        s.close()
        print(f" 发送成功，{len(payload)} 字节 → {SERVER_HOST}:{SERVER_PORT}")
        print(f" 内容: {json.dumps({'items': items, 'low_conf': low_conf_chars}, ensure_ascii=False)}")
        if need_beep:
            fifos()
    except ConnectionRefusedError:
        print(f" 连接被拒绝，请确认 C 程序已启动（{SERVER_HOST}:{SERVER_PORT}）")
    except socket.timeout:
        print(" 连接超时")
    except Exception as e:
        print(f" 发送失败: {e}")


def sorted_boxes(dt_boxes):
    _boxes = sorted(dt_boxes, key=lambda x: (x[0][1], x[0][0]))
    n = len(_boxes)
    for i in range(n - 1):
        for j in range(i, -1, -1):
            if (abs(_boxes[j + 1][0][1] - _boxes[j][0][1]) < 10 and
                    _boxes[j + 1][0][0] < _boxes[j][0][0]):
                _boxes[j], _boxes[j + 1] = _boxes[j + 1], _boxes[j]
            else:
                break
    return _boxes


def get_rotate_crop_image(img, points):
    w = int(max(np.linalg.norm(points[0] - points[1]),
                np.linalg.norm(points[2] - points[3])))
    h = int(max(np.linalg.norm(points[0] - points[3]),
                np.linalg.norm(points[1] - points[2])))
    pts_std = np.float32([[0, 0], [w, 0], [w, h], [0, h]])
    M = cv2.getPerspectiveTransform(points, pts_std)
    dst = cv2.warpPerspective(img, M, (w, h),
                              borderMode=cv2.BORDER_REPLICATE,
                              flags=cv2.INTER_CUBIC)
    if dst.shape[0] * 1.0 / dst.shape[1] >= 1.5:
        dst = np.rot90(dst)
    return dst


def det_inference_func(rknn_instance, frame):
    pre_res    = det_resize({'image': frame})
    img_input  = pre_res['image']
    shape_info = pre_res['shape']
    if img_input.ndim == 3:
        img_input = img_input[np.newaxis]
    outputs = rknn_instance.inference(inputs=[img_input])
    if not outputs:
        return np.array([]), frame
    preds    = {'maps': outputs[0].astype(np.float32)}
    result   = db_postprocess(preds, shape_info)
    dt_boxes = det_postprocess.filter_tag_det_res(result[0]['points'], frame.shape)
    if len(dt_boxes) > 0:
        dt_boxes = sorted_boxes(np.array(dt_boxes))
    return dt_boxes, frame


def rec_inference_func(rknn_instance, frame, boxes):
    results  = []
    crop_buf = np.empty((1, REC_H, REC_W, 3), dtype=np.float32)

    for box in boxes:
        pts  = np.array(box, dtype=np.float32).reshape(4, 2)
        crop = get_rotate_crop_image(frame, pts)
        if crop is None or crop.size == 0:
            continue

        crop = cv2.resize(crop, (REC_W, REC_H))
        crop = cv2.cvtColor(crop, cv2.COLOR_BGR2RGB)
        crop_buf[0] = crop.astype(np.float32) * (2.0 / 255.0) - 1.0

        outputs = rknn_instance.inference(inputs=[crop_buf])
        if not outputs:
            continue

        prob = outputs[0].astype(np.float32)[0]   # shape: (T, num_classes)

        idx   = prob.argmax(axis=1)
        max_p = prob.max(axis=1)

        mask     = np.ones(len(idx), dtype=bool)
        mask[1:] = idx[1:] != idx[:-1]
        mask    &= idx != 0
        valid_idx = idx[mask]
        valid_p   = max_p[mask]

        char_confs = []
        for i, p in zip(valid_idx, valid_p):
            if i < CHAR_TABLE_LEN and CHAR_TABLE[i] is not None:
                char_confs.append((CHAR_TABLE[i], float(p)))

        raw_text = "".join(ch for ch, _ in char_confs).strip()

        if raw_text:
            m = _RE_VALID.search(raw_text)
            detected = m.group(0).strip() if m else raw_text
            if detected:
                start_idx = raw_text.find(detected)
                end_idx = start_idx + len(detected)
                char_confs_trimmed = char_confs[start_idx:end_idx]
                results.append((box, detected, char_confs_trimmed))
        else:
            results.append((box, f"空信号: {idx[:5].tolist()}", []))

    return results, frame


def main():
    buzzer_init()

    if not os.path.exists(GRAY_PATH):
        print(f" 找不到文件 {GRAY_PATH}")
        sys.exit(1)

    with open(GRAY_PATH, 'rb') as f:
        gray_flat = np.frombuffer(f.read(), dtype=np.uint8)

    expected = GRAY_H * GRAY_W
    if gray_flat.size != expected:
        print(f" 文件大小异常：期望 {expected} 字节，实际 {gray_flat.size} 字节")
        sys.exit(1)

    gray      = gray_flat.reshape((GRAY_H, GRAY_W))
    src_image = cv2.cvtColor(gray, cv2.COLOR_GRAY2BGR)
    print(f" 读取灰度图: {GRAY_W}×{GRAY_H}")

    print(" 加载 NPU 模型...")
    det_pipeline = rknnPoolExecutor(DET_MODEL_PATH, TPEs=4, func=det_inference_func)
    rec_pipeline = rknnPoolExecutor(REC_MODEL_PATH, TPEs=4, func=rec_inference_func)
    print(" 模型加载完成")

    start = time.perf_counter()

    det_pipeline.put(src_image)
    det_res = det_pipeline.get()

    if not det_res[1] or len(det_res[0][0]) == 0:
        print("未检测到任何文字")
    else:
        boxes, det_frame = det_res[0]
        rec_pipeline.put(det_frame, boxes)
        rec_res = rec_pipeline.get()

        if rec_res[1]:
            ocr_results, _ = rec_res[0]
            elapsed_ms = (time.perf_counter() - start) * 1000

            print("\n================ OCR 识别结果 ================")
            for item in ocr_results:
                box, text, char_confs = item
                if text.startswith("空信号"):
                    print(f"文本: [ {text} ]")
                    continue
                box_list = box.tolist() if hasattr(box, 'tolist') else box
                print(f"坐标: {box_list}")
                print(f"文本: [ {text} ]")
                for ch, conf in char_confs:
                    flag = " ← 低置信度" if conf < CHAR_CONF_THRESHOLD else ""
                    print(f"  '{ch}': {conf:.5f}{flag}")
            print("==============================================")
            print(f"推理耗时: {elapsed_ms:.2f} ms\n")

            send_ocr_results(ocr_results)



    det_pipeline.release()
    rec_pipeline.release()
    print("资源释放完毕")


if __name__ == "__main__":
    main()