/**
 * @file main.cc
 * @author guoqing (1337841346@qq.com)
 * @brief 主
 * @version 0.1
 * @date 2019-12-30
 * 
 * @copyright Copyright (c) 2019
 * 
 */

// ================================= 头文件 ==========================================

// C++ 标准
#include <iostream>
#include <vector>
#include <string>
#include <chrono>
#include <functional>
#include <thread>
#include <memory>

// Linux 信号处理
#include<signal.h>

// GLog
#include <glog/logging.h>

// GFLAGS 命令行参数解析支持
#include <gflags/gflags.h>

// 自己进一步封装的接口
#include "MYNT_CameraIMU.h"

// 小觅相机驱动
#include "mynteyed/camera.h"
// #include "mynteyed/utils.h"

#include "Viewer.h"

// ================================= 命名空间 ==========================================

using std::cout;
using std::cin;
using std::endl;

// ================================= 宏定义 ============================================
// 当前程序中要使用的参数名称
#define DEFAULT_LOG_PATH "./log/"

// =============================== 命令行参数 ===========================================
DEFINE_string(logPath,        DEFAULT_LOG_PATH, "path to log        (.log file)");

// =================================== 全局变量 ========================================
bool bLoop;
std::unique_ptr<Viewer> gcpViewer;

// ================================= 函数原型 ===========================================

/**
 * @brief 解析命令行参数
 * @param[in]  argc 
 * @param[in]  argv 
 * @param[out] strLogPath 日志文件路径
 * @return true 
 * @return false 
 */
bool ParseAndCheckArguments(
    int& argc, char* argv[], 
    std::string& strLogPath);

/**
 * @brief 初始化 GLog 库
 * @param[in] strLogRootPath    存放日志的目录
 */
void InitGLog(std::string& strLogRootPath);

/** @brief 反初始化 GLog */
void DeinitGLog(void);

/**
 * @brief 信号处理函数. 当终端按下 Ctrl+C的时候执行
 * @param[in] signo 信号量
 */
void SignalHandle(int nSignalID);

// 下面是回调函数
void OnImageInfo(const std::shared_ptr<mynteyed::ImgInfo>& cspImgInfo);
void OnLeftImage(const mynteyed::StreamData& leftImgData);
void OnRightImage(const mynteyed::StreamData& rightImgData);
void OnDepthImage(const mynteyed::StreamData& depthImgData);
void OnIMUData(const mynteyed::MotionData& imuData);


int main(int argc, char* argv[])
{
    // step 0 输出默认信息
    cout<<"MYNT EYE D stereo camera evalution tool."<<endl;
    cout<<"Complied at "<<__TIME__<<", "<<__DATE__<<endl;

    // step 1 解析命令行参数, 初始化GLOG, 设置信号响应函数
    std::string strLogPath;
    if(!ParseAndCheckArguments(argc, argv, strLogPath))
    {
        return 0;
    }
    InitGLog(strLogPath);
    signal(SIGINT,SignalHandle);
    bLoop = true;

    // step 2 创建可视化查看器
    gcpViewer = std::unique_ptr<Viewer>(new Viewer());
    LOG(DEBUG)<<"[main] Launching Viewer ...";
    std::thread*  pThreadViewer = new std::thread(&Viewer::run, gcpViewer.get());
    gcpViewer->UpdateStatusBar("Initializing camera ...");
    

    // step 3 初始化相机并打开设备
    CameraIMU camera;
    // 回调函数
    std::function<void(const std::shared_ptr<mynteyed::ImgInfo>&)> f1(OnImageInfo);
    std::function<void(const mynteyed::StreamData&)>               f2(OnLeftImage);
    std::function<void(const mynteyed::StreamData&)>               f3(OnRightImage);
    std::function<void(const mynteyed::StreamData&)>               f4(OnDepthImage);
    std::function<void(const mynteyed::MotionData&)>               f5(OnIMUData);

    gcpViewer->UpdateStatusBar("Opening camera ...");
    camera.OpenCameraIMU(
        f1, f2, f3, f4, f5);

    gcpViewer->UpdateStatusBar("Done.");
    // step 4 主循环
    while(bLoop)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));

        if(gcpViewer->isESCPressed())
        {
            bLoop =false;
            LOG(DEBUG)<<"[main] ESC Pressed detected.";
            gcpViewer->UpdateStatusBar("ESC pressed.");
        }
    };

    
    
    // step 5 善后工作
    gcpViewer->UpdateStatusBar("Stopping camera ...");
    LOG(DEBUG)<<"[main] Stopping camera ...";
    camera.CloseCameraIMU();

    gcpViewer->UpdateStatusBar("Stopping viewer ...");
    LOG(DEBUG)<<"[main] Stopping viewer ...";
    gcpViewer->requestStop();
    pThreadViewer->join();


    return 0;
}

// 命令行解析
bool ParseAndCheckArguments(
    int& argc, char* argv[], 
    std::string& strLogPath)
{
    // step 0 解析输入参数
    // 设置当前程序的使用说明
    std::string strUsage("MYNT EYE D stereo camera evalution tool.");
    gflags::SetUsageMessage(strUsage);
    // 解析参数
    gflags::ParseCommandLineFlags(&argc, &argv, true);

    // step 1 参数合法性判断
    if(FLAGS_logPath.empty())
    {
        // 如果没有设置日志文件的保存路径
        cout << "\033[33m[main] Warning: logPath is empty. Default set to \"" << DEFAULT_LOG_PATH << "\".\033[0m" << endl;
    }

    // step 2 将解析出来的参数进行保存
    strLogPath        = FLAGS_logPath;

    // step 3 析构 gflags 并且返回
    gflags::ShutDownCommandLineFlags();
    return true;
}

// 初始化GLog库
void InitGLog(std::string& strLogRootPath)
{
    // step 0 初始化GLog写入的根地址
    google::InitGoogleLogging(strLogRootPath.c_str());

    // step 1 设置不同重要等级的日志的文件名格式
    google::SetLogDestination(google::GLOG_FATAL,   (strLogRootPath+"/fatal_").c_str());        // 设置 google::FATAL 级别的日志存储路径和文件名前缀
    google::SetLogDestination(google::GLOG_ERROR,   (strLogRootPath+"/error_").c_str());        // 设置 google::ERROR 级别的日志存储路径和文件名前缀
    google::SetLogDestination(google::GLOG_WARNING, (strLogRootPath+"/warning_").c_str());      // 设置 google::WARNING 级别的日志存储路径和文件名前缀
    google::SetLogDestination(google::GLOG_INFO,    (strLogRootPath+"/info_").c_str());         // 设置 google::INFO 级别的日志存储路径和文件名前缀
    google::SetLogDestination(google::GLOG_DEBUG,   (strLogRootPath+"/debug_").c_str());        // 设置 google::INFO 级别的日志存储路径和文件名前缀

    // step 2 其他选项
    // 磁盘满时终止写入
    FLAGS_stop_logging_if_full_disk = true;
    // 输出终端时使能颜色标记
    FLAGS_colorlogtostderr = true;
    // 大于该等级的日志信息均输出到屏幕上
    google::SetStderrLogging(google::GLOG_DEBUG);

    LOG(INFO)<<"[InitGLog] GLog enabled. System is going to bring up.";
    return ;
}

// 反初始化GLog库
void DeinitGLog(void)
{
    google::ShutdownGoogleLogging();
}


// 下面是用于进行测试的回调函数
// c = const   s = shared   p = pointer
// TODO 考虑去掉
void OnImageInfo(const std::shared_ptr<mynteyed::ImgInfo>& cspImgInfo)
{
    LOG(DEBUG)<<"[OnImageInfo] frame id = " << cspImgInfo->frame_id
              <<", stamp = "<<cspImgInfo->timestamp
              <<", expos time = "<<cspImgInfo->exposure_time;
}

void OnLeftImage(const mynteyed::StreamData& leftImgData)
{
    gcpViewer->UpdateLeftImage(
        leftImgData.img->To(mynteyed::ImageFormat::COLOR_BGR)->ToMat(),
        leftImgData.img_info->exposure_time,
        leftImgData.img_info->timestamp);
}

void OnRightImage(const mynteyed::StreamData& rightImgData)
{
    gcpViewer->UpdateRightImage(
        rightImgData.img->To(mynteyed::ImageFormat::COLOR_BGR)->ToMat(),
        rightImgData.img_info->exposure_time,
        rightImgData.img_info->timestamp);
}

void OnDepthImage(const mynteyed::StreamData& depthImgData)
{
    gcpViewer->UpdateDepthImage(
        depthImgData.img->To(mynteyed::ImageFormat::COLOR_BGR)->ToMat(),
        // depthImgData.img_info->exposure_time,
        0);
        // depthImgData.img_info->timestamp);
}

void OnIMUData(const mynteyed::MotionData& imuData)
{
    if(imuData.imu->flag == MYNTEYE_IMU_ACCEL)
    {
        LOG(DEBUG)<<"[OnIMUData] accel stamp"<< imuData.imu->timestamp;
    }
    else if(imuData.imu->flag == MYNTEYE_IMU_GYRO)
    {
        LOG(DEBUG)<<"[OnIMUData] gyro  stamp"<< imuData.imu->timestamp;
    }
}


void SignalHandle(int nSignalID)
{
    // TODO 现在这里并没有添加什么功能
    LOG(ERROR)<<"nSignalID = "<<nSignalID<<endl;
    if(bLoop)
    {
        bLoop =false;
    }
    else
    {
        exit(0);
    }
}