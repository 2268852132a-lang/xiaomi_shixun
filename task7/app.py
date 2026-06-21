import os
import time
import threading
import wave
import json
import subprocess
import select
from flask import Flask, send_from_directory
from flask_socketio import SocketIO, emit
from vosk import Model, KaldiRecognizer

app = Flask(__name__, static_folder='frontend', static_url_path='')
app.config['SECRET_KEY'] = 'secret!'
socketio = SocketIO(app, cors_allowed_origins="*")

CHUNK = 1600
RATE = 16000
RECORD_SECONDS = 5

BASE_DIR = os.path.dirname(os.path.abspath(__file__))
RECORDINGS_DIR = os.path.join(BASE_DIR, 'recordings')
MODEL_PATH = "/home/sunrise/vosk_models/vosk-model-cn"

WAKE_WORD = ["你好小爱", "小爱", "你好小唉", "你好小艾", "你好 小艾", "你好 小爱", "着 小艾", "着 小爱", "你 好 小 爱", "你 好 小艾", "你 好 小 艾", "呢 着 小 艾" ]
is_recording = False

os.makedirs(RECORDINGS_DIR, exist_ok=True)

print(f"加载模型: {MODEL_PATH}")
model = Model(MODEL_PATH)
recognizer = KaldiRecognizer(model, RATE)
recognizer.SetWords(False)
print("模型加载完成")

monitor_proc = None

def record_audio(filename):
    global is_recording, monitor_proc
    
    # 1. 停止监控进程，释放硬件设备
    if monitor_proc:
        monitor_proc.terminate()
        monitor_proc.wait()
        monitor_proc = None
        print("监控进程已停止，准备录音...")

    # 2. 使用 arecord 录制 5 秒
    # 增加 -V mono 显示电平，方便调试
    cmd = ['arecord', '-D', 'hw:0,0', '-d', str(RECORD_SECONDS),
           '-f', 'S16_LE', '-r', str(RATE), '-c', '1', filename]
    
    result = subprocess.run(cmd, capture_output=True, text=True)
    if result.returncode != 0:
        print(f"录音失败: {result.stderr}")
    else:
        print(f"录音成功: {filename}")

    is_recording = False
    socketio.emit('status_update', {
        'status': 'idle',
        'msg': '录音完成',
        'file': '/recordings/latest.wav'
    })
    
    # 3. 录音结束后，会在 voice_monitor 循环中自动重新启动监控

def voice_monitor():
    global is_recording, monitor_proc
    
    print("语音监控已启动，等待唤醒词...")
    
    while True:
        if not is_recording:
            # 如果监控进程没启动，则启动它
            if monitor_proc is None:
                cmd = ['arecord', '-D', 'hw:0,0', '-f', 'S16_LE', '-r', str(RATE), '-c', '1', '-t', 'raw']
                monitor_proc = subprocess.Popen(cmd, stdout=subprocess.PIPE, stderr=subprocess.DEVNULL)
                print("监控进程已启动，正在监听...")

            # 读取 0.1 秒的音频数据
            try:
                data = monitor_proc.stdout.read(int(RATE * 0.1 * 2))  # 0.1秒，16bit=2字节
                if data and len(data) > 0:
                    if recognizer.AcceptWaveform(data):
                        result = json.loads(recognizer.Result())
                        text = result.get('text', '').replace(' ', '') # 去除空格
                        if text:
                            print(f"识别: {text}")
                            if WAKE_WORD in text:
                                print(f">>> 检测到唤醒词")
                                is_recording = True
                                socketio.emit('status_update', {
                                    'status': 'recording',
                                    'msg': '成功检测到关键词，正在录音 5 秒...'
                                })
                                filename = os.path.join(RECORDINGS_DIR, 'latest.wav')
                                # 在新线程录音，避免阻塞监控循环
                                threading.Thread(target=record_audio, args=(filename,)).start()
            except Exception as e:
                print(f"监控异常: {e}")
                if monitor_proc:
                    monitor_proc.terminate()
                    monitor_proc = None
        
        time.sleep(0.05)

@app.route('/')
def index():
    return send_from_directory('frontend', 'index.html')

@app.route('/recordings/<path:filename>')
def serve_recording(filename):
    return send_from_directory(RECORDINGS_DIR, filename)

if __name__ == '__main__':
    threading.Thread(target=voice_monitor, daemon=True).start()
    print("\n" + "="*50)
    print(f"服务启动: http://0.0.0.0:5001")
    print(f"唤醒词: {WAKE_WORD}")
    print("="*50 + "\n")
    socketio.run(app, host='0.0.0.0', port=5001, allow_unsafe_werkzeug=True)