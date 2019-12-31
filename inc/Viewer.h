/**
 * @file Viewer.h
 * @author guoqing (1337841346@qq.com)
 * @brief 可视化查看器
 * @version 0.1
 * @date 2019-12-30
 * 
 * @copyright Copyright (c) 2019
 * 
 */

#ifndef __VIEWER_H__
#define __VIEWER_H__

#include <vector>
#include <string>
#include <thread>
#include <iostream>

#include <pangolin/pangolin.h>
#include <opencv2/opencv.hpp>

class Viewer
{
public:

    //构造函数
    Viewer();


    /** @brief 避免触发全屏操作 */
    static inline void NonFullScreen(void)
    {   
        return ;
    }
    
    /** @brief 析构函数 */
    ~Viewer();
   
public:
    /** @brief 线程的主进程函数 */
    void run(void);

    /** @brief 等待查看器进程结束,堵塞函数 */
    void waitForStopped(void);

public:

    // 和数据更新相关的函数
    void UpdateLeftImage(
        const cv::Mat&  imgLeft,
        const uint16_t& nExposeTime,
        const uint32_t& nLeftTimestamp);


public:

    // 和绘图相关的函数

    // 绘制彩色图像的纹理
    void DrawLeftImageTexture(void);

private:

    // 和绘图有关的变量, 只被绘图线程使用到, 不需要线程锁保护
    std::unique_ptr<pangolin::GlTexture> mupLeftImageTexture;

public:

    // 和控制相关的内联函数
    /** @brief 请求终止Viewer线程 */
    inline void requestStop(void)
    {
        std::lock_guard<std::mutex> lock(mMutexRequestStop);
        mbRequestStop=true;
    }

    /** @brief 是否有了请求终止Viewer线程的操作 */
    inline bool isRequestStop(void)
    {
        std::lock_guard<std::mutex> lock(mMutexRequestStop);
        return mbRequestStop;
    }

    /** @brief 查看Viewer进程是否已经终止 */
    inline bool isStateStop(void)
    {
        std::lock_guard<std::mutex> lock(mMutexStateStop);
        return mbStateStop;
    }

    /** @brief 查看Viewer用户是否已经按下了ESC键 */
    inline bool isESCPressed(void)
    {
        std::lock_guard<std::mutex> lock(mMutexESCPressed);
        return mbESCPressed;
    }

private:
    //私有内联函数

    /** @brief 设置当前线程已经是停止的状态 */
    // TODO 好像是没有被用到?
    inline void setStateStop(void)
    {
        std::lock_guard<std::mutex> lock(mMutexStateStop);
        mbStateStop=true;
    }

    /** @brief 回调函数, 设置ESC键已经被按下 */
    inline void OnESC(void)
    {
        std::lock_guard<std::mutex> lock(mMutexESCPressed);
        mbESCPressed=true;
    }

private:

    //绘制地图查看器
    void drawMapViewer(void);

    // 下面是比较像工具的函数======================================================

    // 将OpenCV的图像格式转换成为OpenGL的图像格式
    bool cv2glIamge(pangolin::GlTexture& imageTexture, const cv::Mat img);
    std::vector<GLfloat> eigen2glfloat(Eigen::Isometry3d T);

    // 绘制底部状态栏的图像
    void drawStatusBarImg(pangolin::GlTexture& imageTexture);

    // 绘制相机
    // 参数化
    void drawCamera(Eigen::Isometry3d Twc,double size, double r,double g,double b);

    void drawCamera(Eigen::Isometry3d Twc,double r=1.0f,double g=1.0f,double b=1.0f);

private:

    // ======================= 缓存相关 ======================
    // 线程锁保护: 此处的百年来那个会被可视化线程使用, 也可能被主线程使用

    std::mutex mMutexLeftImage;
    // 标志是否已经更新过了, 每次外部函数调用更新函数的时候这个标志将会被置位; 而当Viewer::Run()函数中使用完之后将复位
    bool mbLeftImagesUpdated = false;
    /// 缓存的左目图像
    cv::Mat mImgLeft;
    /// 曝光时间
    uint16_t mnLeftExposeTime;
    /// 时间戳
    uint32_t mnLeftTimestamp;





    
    /// 缓存的右目图像
    cv::Mat miCacheRight;
    /// 缓存的深度图像
    cv::Mat miCacheDepth;
    /// 缓存的点云
    std::vector<Eigen::Vector3f> mvevCacheClouds3f;
    
    uint16_t mnRightExposeTime;
    
    uint32_t mnRightTimestamp;
    uint32_t mnDepthTimestamp;

    // IMU 数据
    // /// 缓存的相机位姿
    // std::mutex mMutexAccelData;
    // Eigen::Vector2d mevAccel3d;
    // double mdAccelTemper;
    // uint64_t mnAccelTimestamp;

    // Eigen::Isometry3d memCacheCameraTwc;


    
    // ======================= 控制相关 ======================
    // 线程锁保护: 此处的变量会被可视化线程使用, 也可能被主线程使用

    /// 外部请求终止当前进程的标志
    std::mutex mMutexRequestStop;
    bool mbRequestStop;

    /// 当前进程是否已经停止的标志
    std::mutex mMutexStateStop;
    bool mbStateStop;

    /// 用户是否在查看器中按下ESC键的标志
    std::mutex mMutexESCPressed;
    bool mbESCPressed;


    /// 退出模式.为True的时候,当系统进程结束时,当前查看器进程也将会结束;反之则当前进程会等待,直到用户关闭窗口
    bool mbQuitAfterEnding;

    //不安全的类型, 用于缓冲图像
    unsigned char* mpImageCache;
    unsigned char* mpStatusImageCache;


private:

    // 一些用于控制绘图的变量. 一次写入,全程只读

    // 窗口的大小
    size_t mnWindowWitdh = 1024;
    size_t mnWindowHigh  = 768;
    
    // 真的是原始图像的大小, 我们先默认是 640 x 480吧
    size_t  mnImageHigh  = 480;
    size_t  mnImageWidth = 640;

    double mdLfx = 516.55377197265625000f;
    double mdLfy = 516.58856201171875000f;
    double mdLcx = 323.97888183593750000f;
    double mdLcy = 236.40682983398437500f;
    

    // Panel大小
    size_t  mnPanelWitdh = 180;
    size_t  mnPanelHigh  = mnWindowHigh;

    // Status Bar 大小
    size_t mnStatusBarHigh  = 32;
    size_t mnStatusBarWidth = mnWindowWitdh - mnPanelWitdh;

    // ImageViewer的大小
    size_t mfLeftImageViewerWidth  = round((mnWindowWitdh - 180)/3.0f);
    size_t mfRightImageViewerWidth = round((mnWindowWitdh - 180)/3.0f);
    size_t mfDepthImageViewerWidth = mnWindowWitdh - 180 - mfLeftImageViewerWidth - mfRightImageViewerWidth;

    size_t mfLeftImageViewerHigh   = round(mfLeftImageViewerWidth * 0.75);
    size_t mfRightImageViewerHigh  = mfLeftImageViewerHigh;
    size_t mfDepthImageViewerHigh  = mfLeftImageViewerHigh;

    // TODO 这里暂时没有考虑其他的Plotter

    // MapViewer
    size_t mnMapViewerHigh  = mnWindowHigh  - mfLeftImageViewerHigh - mnStatusBarHigh;
    size_t mnMapViewerWidth = mnWindowWitdh - mnPanelWitdh;
    // float  mfMapViewerRatio = 

    // 一些画图的时候使用到的其他属性
    float mfAxisSize = 1.0f;
    float mfCameraSize = 1.0f;

    // 窗口标题
    std::string mstrWindowTitle = std::string("MYNT EYE D Evalution Tool");

    
};  //class Viewer



#endif //__VIEWER_H__