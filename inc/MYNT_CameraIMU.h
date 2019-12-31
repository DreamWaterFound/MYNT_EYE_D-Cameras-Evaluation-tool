/**
 * @file MYNT_CameraIMU.h
 * @author guoqing (1337841346@qq.com)
 * @brief 自己在小觅相机SDK基础上为了当前程序方便进行的封装
 * @version 0.1
 * @date 2019-12-30
 * 
 * @copyright Copyright (c) 2019
 * 
 */

#ifndef __MYNT_CAMERA_IMU_H__
#define __MYNT_CAMERA_IMU_H__


// ================================= 头文件 ==========================================

// C++ 标准
#include <iostream>
#include <vector>
#include <string>
#include <memory>
#include <functional>


// 小觅相机驱动
#include "mynteyed/camera.h"
#include "mynteyed/utils.h"

// ================================= 命名空间 ==========================================

/** @brief 对小觅相机操作的再一次封装 */
class CameraIMU
{
public:
    CameraIMU();
    ~CameraIMU();

public:
    // 打开相机, 回调函数方式
    bool OpenCameraIMU(
        std::function<void(const std::shared_ptr<mynteyed::ImgInfo>&)> funcOnImageInfo, 
        std::function<void(const mynteyed::StreamData&)>               funcOnLeftImage,
        std::function<void(const mynteyed::StreamData&)>               funcOnRightImage,
        std::function<void(const mynteyed::StreamData&)>               funcOnDepthImage,
        std::function<void(const mynteyed::MotionData&)>               funcOnIMUStream);

    bool CloseCameraIMU(void);

private:
    
    // 小觅相机操作使用的对象接口
    std::shared_ptr<mynteyed::Camera>       mpCamera;
    std::shared_ptr<mynteyed::DeviceInfo>   mpDevInfo;

    bool                                    mbOpened;           // 当前相机是否打开

   
};


#endif      // macro __MYNT_CAMERA_IMU_H__

/*



mynteyed::Camera        myntCamera;
    mynteyed::DeviceInfo    myntDevInfo;

    

    */