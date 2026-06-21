import os
import threading
import time
import eventlet
# 必须在导入其他库之前进行 monkey_patch，否则后台线程无法发送 Socket 消息
eventlet.monkey_patch()

from flask import Flask, render_template, send_from_directory
from flask_socketio import SocketIO, emit
from locator import SoundLocator, process_audio_stream

app = Flask(__name__, static_folder='frontend', static_url_path='')
app.config['SECRET_KEY'] = 'secret!'
# 显式设置 async_mode 并允许跨域和旧版本协议
socketio = SocketIO(app, cors_allowed_origins="*", async_mode='eventlet', logger=True, engineio_logger=True)

@app.route('/')
def index():
    return send_from_directory('frontend', 'index.html')

def background_locator():
    """后台定位任务"""
    def on_direction_detected(angle):
        # 强制转换为标准 float，防止 numpy 类型导致序列化失败
        angle_val = float(angle)
        print(f"发送到前端: {angle_val}")
        socketio.emit('direction_update', {'angle': angle_val})
    
    # 启动音频流处理
    process_audio_stream(on_direction_detected)

if __name__ == '__main__':
    # 在后台线程运行定位逻辑
    locator_thread = threading.Thread(target=background_locator, daemon=True)
    locator_thread.start()
    
    print("Web服务器启动在 http://0.0.0.0:5000")
    socketio.run(app, host='0.0.0.0', port=5000)
