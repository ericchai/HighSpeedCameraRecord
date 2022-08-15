# HighSpeedCameraRecord
高速相机采集
使用的相机为usb30的高速工业相机，型号为MiniSVision的，分辨率为30万像素，640*480，最高帧率为775frames/s。
程序采用使用直接写内存的方式存储数据，而后根据数据采用不同的过滤方法生成图片，最后使用ffmpeg生成视频。
