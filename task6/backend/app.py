from flask import Flask, request, jsonify, send_from_directory
import os
import time

# 尝试导入 smbus2，如果没有则使用模拟模式
try:
    from smbus2 import SMBus
    HAS_SMBUS = True
except ImportError:
    HAS_SMBUS = False

app = Flask(__name__)

# 前端文件路径
FRONTEND_DIR = os.path.join(os.path.dirname(os.path.abspath(__file__)), 'frontend')

# ==================== 硬件控制基类 ====================
class I2CDevice:
    def __init__(self, address, bus_num=1):
        self.address = address
        self.bus_num = bus_num
        self.bus = None
        self.is_ready = False
        if HAS_SMBUS:
            try:
                self.bus = SMBus(self.bus_num)
                self.is_ready = True
            except Exception as e:
                print(f"I2C Init Error for 0x{address:02X}: {e}")

# ==================== PCA9685 控制类 (风扇 E2 & 灯光 E1) ====================
class PCA9685(I2CDevice):
    def __init__(self, address, bus_num=1):
        super().__init__(address >> 1, bus_num) # 8-bit to 7-bit
        if self.is_ready:
            self.init_chip()

    def init_chip(self):
        try:
            self.bus.write_byte_data(self.address, 0x00, 0x00) # MODE1 复位
            time.sleep(0.01)
            self.all_off()
        except:
            self.is_ready = False

    def set_pwm(self, channel, on, off):
        if self.is_ready:
            base_reg = 0x06 + 4 * channel
            self.bus.write_byte_data(self.address, base_reg, on & 0xFF)
            self.bus.write_byte_data(self.address, base_reg + 1, (on >> 8) & 0xFF)
            self.bus.write_byte_data(self.address, base_reg + 2, off & 0xFF)
            self.bus.write_byte_data(self.address, base_reg + 3, (off >> 8) & 0xFF)

    def all_off(self):
        if self.is_ready:
            self.bus.write_byte_data(self.address, 0xFA, 0x00)
            self.bus.write_byte_data(self.address, 0xFB, 0x00)
            self.bus.write_byte_data(self.address, 0xFC, 0x00)
            self.bus.write_byte_data(self.address, 0xFD, 0x10)

# ==================== E3 窗帘控制类 ====================
class E3Curtain(I2CDevice):
    def __init__(self, address=0xE0, bus_num=1):
        super().__init__(address >> 1, bus_num)

    def set_position(self, pos):
        if self.is_ready:
            # 参考 e3.c: i2c_delay_byte_write(..., 0x03, re_pos)
            self.bus.write_byte_data(self.address, 0x03, int(pos))

# 初始化硬件实例 (根据 i2cdetect -y 5 的结果)
BUS_NUM = 5
fan_hw = PCA9685(address=0xC8, bus_num=BUS_NUM)    # E2 风扇 (若扫描未出，请检查接线)
light_hw = PCA9685(address=0xC0, bus_num=BUS_NUM)  # E1 RGB灯
curtain_hw = E3Curtain(address=0xE0, bus_num=BUS_NUM) # E3 窗帘

# 模拟硬件状态
device_state = {
    "fan": {"switch": False, "speed": 50},
    "light": {"switch": False, "brightness": 80, "color": "#ffffff"},
    "curtain": {"position": 0}
}

@app.route('/')
def index():
    return send_from_directory(FRONTEND_DIR, 'index.html')

@app.route('/api/status', methods=['GET'])
def get_status():
    return jsonify(device_state)

@app.route('/api/control', methods=['POST'])
def control_device():
    data = request.json
    device = data.get('device')
    action = data.get('action')
    value = data.get('value')
    
    if device in device_state:
        device_state[device][action] = value
        
        # --- 实际硬件控制逻辑 ---
        if device == 'fan':
            speed = device_state['fan']['speed'] if device_state['fan']['switch'] else 0
            off_val = int(0xFFF * speed / 100)
            fan_hw.set_pwm(0, 0, off_val) # 通道 0
            
        elif device == 'light':
            if device_state['light']['switch']:
                bright = device_state['light']['brightness'] / 100.0
                color_hex = device_state['light']['color'].lstrip('#')
                r, g, b = tuple(int(color_hex[i:i+2], 16) for i in (0, 2, 4))
                # 参考 e1.c: off = on + color * 0x10 (即 color * 16)
                # 我们这里加入亮度调节
                light_hw.set_pwm(1, 0x0F, 0x0F + int(r * 16 * bright)) # Red: Channel 1
                light_hw.set_pwm(0, 0x0F, 0x0F + int(g * 16 * bright)) # Green: Channel 0
                light_hw.set_pwm(2, 0x0F, 0x0F + int(b * 16 * bright)) # Blue: Channel 2
            else:
                light_hw.all_off()
                
        elif device == 'curtain':
            # 参考 e3.c: re_pos (0-100)
            curtain_hw.set_position(value)
            
        print(f"Hardware Action: {device} -> {action}: {value}")
        return jsonify({"status": "success", "current_state": device_state[device]})
    
    return jsonify({"status": "error", "message": "Invalid device or action"}), 400

if __name__ == '__main__':
    app.run(host='0.0.0.0', port=5000, debug=True)
