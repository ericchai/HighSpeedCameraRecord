# HighSpeedCameraRecord

## 高速相机采集
1. 使用场景：捕捉3.5m/s的运动速度的小物体，

2. 使用的相机为usb30的高速工业相机，型号为MiniSVision（Mshiwi）的ht-sua33gc-t， 对应于迈德威视的mv-sua33gc-t，分辨率为30万像素，640*480，最高帧率为775frames/s,对于3m/s的运动速度的捕 捉绰绰有余。

3. 程序采用使用直接写内存的方式存储数据，采用这种方法电脑即使没有高速硬盘也可以来高速采集图片，而后根据数据采用不同的过滤方法生成图片，最后使用ffmpeg生成视频。

4. 相机驱动：MiniSVision Camera Platform Setup(2.1.10.129).zip
### 开发
5. 编译环境：MSVC2019
