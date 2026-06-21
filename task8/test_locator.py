import numpy as np
from scipy import signal
from locator import SoundLocator

def test_localization():
    # 模拟参数
    fs = 16000
    d = 0.036
    v = 340
    locator = SoundLocator(fs, d, v)
    
    # 模拟测试多个角度
    test_angles = [0, 45, 90, 135, 180, 225, 270, 315]
    
    print(f"{'目标角度':<10} | {'计算角度':<10} | {'误差':<10}")
    print("-" * 35)
    
    for target in test_angles:
        rad = np.deg2rad(target)
        
        # 理论时延
        # 注意: 这里的坐标系定义需与 locator.py 一致
        # x轴是左右 (sin), y轴是前后 (cos)
        tau_x = d * np.sin(rad) / v
        tau_y = d * np.cos(rad) / v
        
        # 构造模拟信号 (简单正弦波 + 延迟)
        t = np.arange(0, 0.05, 1/fs) # 50ms 信号
        freq = 1000 # 1kHz
        
        sig_ref = np.sin(2 * np.pi * freq * t)
        
        # 使用高精度模拟延迟
        def delay_sig_high_res(s, tau, fs, res_factor=100):
            # 通过过采样模拟亚采样延迟
            fs_high = fs * res_factor
            s_high = signal.resample(s, len(s) * res_factor)
            n_delay = int(tau * fs_high)
            if n_delay >= 0:
                s_delayed_high = np.pad(s_high, (n_delay, 0))[:len(s_high)]
            else:
                s_delayed_high = np.pad(s_high, (0, -n_delay))[-len(s_high):]
            return signal.resample(s_delayed_high, len(s))

        ch0 = sig_ref
        ch2 = delay_sig_high_res(sig_ref, -tau_x, fs)
        
        ch1 = sig_ref
        ch3 = delay_sig_high_res(sig_ref, -tau_y, fs)
        
        calc_angle = locator.get_azimuth([ch0, ch1, ch2, ch3])
        error = abs(calc_angle - target)
        if error > 180: error = 360 - error
        
        print(f"{target:<10}° | {calc_angle:<10}° | {error:<10.2f}°")

if __name__ == "__main__":
    test_localization()
