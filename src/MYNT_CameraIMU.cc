/**
 * @file MYNT_CameraIMU.cc
 * @author guoqing (1337841346@qq.com)
 * @brief 自己对小觅相机SDK常用接口的封装
 * @version 0.1
 * @date 2019-12-30
 * 
 * @copyright Copyright (c) 2019
 * 
 */

#include "MYNT_CameraIMU.h"

// GLog
#include <glog/logging.h>

CameraIMU::CameraIMU()
    :mpCamera(nullptr), mpDevInfo(nullptr), mbOpened(false)
{
    // step 0 创建对象
    mpCamera  = std::shared_ptr<mynteyed::Camera>(new mynteyed::Camera());
    mpDevInfo = std::shared_ptr<mynteyed::DeviceInfo>(new mynteyed::DeviceInfo());
}

CameraIMU::~CameraIMU()
{
    // step 1 如果当前相机已经处于打开状态, 那么就要关闭相机
    if(mpCamera->IsOpened())
    {
        LOG(INFO)<<"[CameraIMU::~CameraIMU()] Closing Camera ...";
        mpCamera->Close();
    }
}

// 初始化相机
bool CameraIMU::OpenCameraIMU(
    std::function<void(const std::shared_ptr<mynteyed::ImgInfo>&)> funcOnImageInfo, 
    std::function<void(const mynteyed::StreamData&)>               funcOnLeftImage,
    std::function<void(const mynteyed::StreamData&)>               funcOnRightImage,
    std::function<void(const mynteyed::StreamData&)>               funcOnDepthImage,
    std::function<void(const mynteyed::MotionData&)>               funcOnIMUStream)
{
    // step 1 枚举设备
    LOG(INFO)<<"[CameraIMU::OpenCameraIMU] Enumerating device(s) ...";

    if(!mynteyed::util::select(*mpCamera, static_cast<mynteyed::DeviceInfo*>(mpDevInfo.get())))
    {
        LOG(ERROR)<<"[CameraIMU::OpenCameraIMU] No device found.";
        return false;
    }

    LOG(INFO)<<"[CameraIMU::OpenCameraIMU] Found device: " << mpDevInfo->name;

    // step 2 设置参数.
    mynteyed::OpenParams params(mpDevInfo->index);
    params.framerate            = 30;
    params.dev_mode             = mynteyed::DeviceMode  ::DEVICE_ALL;
    params.color_mode           = mynteyed::ColorMode   ::COLOR_RECTIFIED;
    params.color_stream_format  = mynteyed::StreamFormat::STREAM_YUYV;
    params.depth_mode           = mynteyed::DepthMode   ::DEPTH_COLORFUL;
    params.stream_mode          = mynteyed::StreamMode  ::STREAM_1280x480;
    params.state_ae             = true;
    params.state_awb            = true;
    params.colour_depth_value   = 5000;
    // 我的相机不支持IR
    params.ir_intensity         = 0;

    // step 3 设置相机工作方式以及回调函数
    mpCamera->EnableProcessMode(mynteyed::ProcessMode::PROC_IMU_ALL);

    // 如果需要统计曝光时间等数据, 就需要使能这个选项
    mpCamera->EnableImageInfo(true);
    // mpCamera->EnableMotionDatas(0);


    // 现在先不启用这两个
    // mpCamera->SetImgInfoCallback(funcOnImageInfo);
    // mpCamera->SetMotionCallback (funcOnIMUStream);
    mpCamera->SetStreamCallback (mynteyed::ImageType::IMAGE_LEFT_COLOR,  funcOnLeftImage);
    mpCamera->SetStreamCallback (mynteyed::ImageType::IMAGE_RIGHT_COLOR, funcOnRightImage);
    mpCamera->SetStreamCallback (mynteyed::ImageType::IMAGE_DEPTH,       funcOnDepthImage);

    LOG(DEBUG)<<"[CameraIMU::OpenCameraIMU] Ready to open camera ...";

    // step 4 打开相机
    mpCamera->Open(params);
    if(!mpCamera->IsOpened())
    {
        LOG(ERROR)<<"[CameraIMU::OpenCameraIMU] Open device \""<<mpDevInfo->name<<"\" failed.";
        return false;
    }

    LOG(INFO)<<"[CameraIMU::OpenCameraIMU] Open device \""<<mpDevInfo->name<<"\" success.";

    mbOpened = true;

    return true;
}

// 关闭相机
bool CameraIMU::CloseCameraIMU(void)
{

    mpCamera->DisableMotionDatas();
    // 这个应该在图像流之后被 disable 不然在图像的回调函数中将会尝试读取imageinfo从而发生段错误
    mpCamera->DisableImageInfo();

    
    mpCamera->Close();

    if(mpCamera->IsOpened())
    {
        // 关闭失败
        LOG(ERROR)<<"[CameraIMU::CloseCameraIMU] Close device \""<<mpDevInfo->name<<"\" failed.";
        return false;
    }
    else
    {
        LOG(INFO)<<"[CameraIMU::CloseCameraIMU] Close device \""<<mpDevInfo->name<<"\" success.";
        return true;
    }
}








