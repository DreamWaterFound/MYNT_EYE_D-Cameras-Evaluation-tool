# Evaluation tool for MYNT_EYE_D Cameras

[English](./ReadMe.md) | Chinese

为小觅相机 MYNT D1000-50 相机编写的一个简易评测工具。于 Ubuntu 16.04 上进行了测试

近处场景：

![near](../MYNT_EYE_D_EVA/doc/near.gif)

远处场景：

![distance](../MYNT_EYE_D_EVA/doc/far.gif)

> GIF 图像有点大 (5.9MB, 5.8MB) ， 可能需要花点时间去加载

## 功能

- [x] 显示双目图像, 并且提供点云显示(软件计算, 非相机本身输出)
- [ ] 基于 IMU 数据直观展示相机的位姿
- [x] 绘制 IMU 原始数据(包括温度)随着时间的变化
- [ ] 绘制三个传感器(左目相机, 右目相机和IMU)的时间戳
- [x] 绘制曝光时间随时间的变化

![distance](../MYNT_EYE_D_EVA/doc/GUI_cn.png)

## 依赖

- CMake
- Pangolin (已包含，基于0.5修改)
- Gflags (已包含)
- GLog (已包含， 修改过)
- Eigen 3.3 或更高
- OpenCV 3.0 或更高
- Indicators (已包含, 但是目前版本尚未使用)
- 小觅相机深度版 SDK (4.12.9)

## 构建

### 1. 编译已包含的第三方库

```
# 编译 gflags
cd ./ThirdParty/gflags 
mkdir build && cd build 
cmake .. && make 
```

```
# 编译 GLog
cd ./ThirdParty/glog 
mkdir build && cd build 
cmake .. && make 
```

```
# 编译 Pangolin
cd ./ThirdParty/Pangolin 
mkdir build && cd build 
cmake .. && make 
```

### 2. 设置 Eigen 和 OpenCV 的路径

如果你的 Eigen 库和 OpenCV库都安装到了默认路径，仅仅需要将[CMakeLists.txt](./CMakeLists.txt)中的：

```
include_directories("/home/guoqing/libraries/eigen_333/include/eigen3")
```

和

```
set(OpenCV_DIR "/home/guoqing/libraries/opencv_331/share/OpenCV")
```

注释掉即可。

如果你想使用的 Eigen 库或者 OpenCV 库并不在默认的安装路径，请自行修改 Eigen 库的头文件所在路径，或者是修改CMake变量`OpenCV_DIR`为文件`OpenCVConfig.cmake`所在的路径。

### 3. 编译工程主体

执行：

```
mkdir build && cd build
cmake ..
make
```

生成的可执行文件在文件夹`bin`中。

## 使用

插入相机（注意当前模式可能需要将相机接入**USB3.0**接口），然后直接运行即可：

```
./bin/eva
```

界面介绍：

![GUI_ch](./doc/GUI_ch.png)

功能尚不完善，凑合用吧（┑(￣Д ￣)┍）

另日志文件默认保存在工程的 log 文件夹下。工程根目录下的[cleanLogs.sh](./cleanLogs.sh)可以帮助你清空最近生成的日志文件：

```
./cleanLogs.sh
```

## 设备

理论上支持 D1000-100 和 D1000-50 两款相机. 测试设备的信息如下:

项目|值
:-:|:-:
型号|MYNT-EYE-D1000-50
固件版本|1.4
硬件版本|2.0
SDK版本|4.12.9

## 已知问题

- 如果相机还没有初始化完成就在终端按下 `Ctrl+C`，会触发段错误
