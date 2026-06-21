import numpy as np
import scipy.signal as signal
import struct
import subprocess
import os

class SoundLocator:
    def __init__(self, fs=16000, d=0.036, vsound=340):
        self.fs = fs
        self.d = d
        self.vsound = vsound
        self.interp = 8 # 降低插值倍数以提高实时性，平衡精度
        self.max_tau = d / vsound # 物理最大时延

    def gcc_phat(self, sig, refsig):
        """GCC-PHAT 算法实现"""
        n = sig.shape[0] + refsig.shape[0]
        SIG = np.fft.rfft(sig, n=n)
        REFSIG = np.fft.rfft(refsig, n=n)
        R = SIG * np.conj(REFSIG)
        R /= (np.abs(R) + 1e-10)
        cc = np.fft.irfft(R, n=n * self.interp)
        
        max_shift = int(self.interp * n / 2)
        # 限制在物理可能的最大时延范围内
        search_range = int(self.interp * self.max_tau * self.fs) + 1
        
        # 提取中心附近的互相关结果
        cc_center = np.concatenate((cc[-search_range:], cc[:search_range+1]))
        shift = np.argmax(np.abs(cc_center)) - search_range
        
        tau = shift / float(self.interp * self.fs)
        return tau

    def get_azimuth(self, channels):
        """
        从4通道数据计算方位角
        channels: [mic1, mic2, mic3, mic4]
        假设: 0-2 为左右对, 1-3 为前后对
        """
        tau_x = self.gcc_phat(channels[0], channels[2])
        tau_y = self.gcc_phat(channels[1], channels[3])
        
        arg_x = np.clip(self.vsound * tau_x / self.d, -1.0, 1.0)
        arg_y = np.clip(self.vsound * tau_y / self.d, -1.0, 1.0)
        
        angle_rad = np.arctan2(arg_x, arg_y)
        angle_deg = np.rad2deg(angle_rad)
        
        if angle_deg < 0:
            angle_deg += 360
            
        return round(angle_deg, 2)

    def is_clap(self, data, threshold=3000):
        """简单的能量检测，判断是否为拍手声"""
        max_val = np.max(np.abs(data))
        if max_val > threshold:
            print(f"检测到声音信号, 峰值: {max_val}")
            return True
        return False

def process_audio_stream(callback):
    """从 arecord 读取音频流并处理"""
    # U2P 麦克风阵列通常是 hw:0,0, 4通道
    cmd = [
        'arecord', '-D', 'hw:0,0', 
        '-f', 'S16_LE', '-r', '16000', '-c', '4', 
        '-t', 'raw', '-q'
    ]
    
    locator = SoundLocator()
    chunk_size = 1024 # 约 64ms
    try:
        proc = subprocess.Popen(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        print(f"音频采集进程已启动 (PID: {proc.pid})")
    except Exception as e:
        print(f"无法启动 arecord: {e}")
        return
    
    print("开始监听音频数据...")
    
    try:
        while True:
            # 读取 4 通道数据
            raw_data = proc.stdout.read(chunk_size * 4 * 2)
            if not raw_data:
                # 检查是否有错误输出
                err = proc.stderr.read()
                if err:
                    print(f"arecord 错误: {err.decode()}")
                break
            
            # 转换为 numpy 数组并分离通道
            audio_data = np.frombuffer(raw_data, dtype=np.int16)
            channels = [audio_data[i::4] for i in range(4)]
            
            # 检测拍手 (使用通道0作为参考)
            if locator.is_clap(channels[0]):
                angle = locator.get_azimuth(channels)
                callback(angle)
                
    except KeyboardInterrupt:
        proc.terminate()
    finally:
        proc.terminate()

if __name__ == "__main__":
    def my_callback(angle):
        print(f"检测到声源方位: {angle}°")

    process_audio_stream(my_callback)
