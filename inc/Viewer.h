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

    /** @brief 更新当前帧的信息,由外部线程调用 */
    // void updateCurrentFrame(Frame::Ptr currFrame);

    /** @brief 更新全局地图,由外部线程调用 */
    // void updateGlobalMap(Map::Ptr map);

    /** @brief 等待查看器进程结束,堵塞函数 */
    void waitForStopped(void);

public:
    //内联函数
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
    inline void setStateStop(void)
    {
        std::lock_guard<std::mutex> lock(mMutexStateStop);
        mbStateStop=true;
    }

    /** @brief 设置ESC键已经被按下 */
    inline void setESCPressed(void)
    {
        std::lock_guard<std::mutex> lock(mMutexESCPressed);
        mbESCPressed=true;
    }

private:


    //绘制地图查看器
    void drawMapViewer(void);

    //绘制RGB图像
    bool drawRGBView(void);

    //绘制深度图像,同时把图像纹理也绘制了，因为深度图像的格式有些复杂
    bool drawDepthView(pangolin::GlTexture& imageTexture,bool isColored, bool& isOk);

    /*

    //绘制另外两种图像，这里就暂时命名为view3 和 view4
    void drawView3(void);

    //同上
    void drawView4(void);std::mutex mMutexFrameData;

    

    */

    //下面是比较像工具的函数======================================================

    //将OpenCV的图像格式转换成为OpenGL的图像格式
    bool cv2glIamge(pangolin::GlTexture& imageTexture, const cv::Mat img);
    std::vector<GLfloat> eigen2glfloat(Eigen::Isometry3d T);

    //绘制底部状态栏的图像
    void drawStatusBarImg(pangolin::GlTexture& imageTexture);

    //绘制相机
    //参数化
    void drawCamera(Eigen::Isometry3d Twc,double size, double r,double g,double b);

    void drawCamera(Eigen::Isometry3d Twc,double r=1.0f,double g=1.0f,double b=1.0f);

    //绘制地图
    void drawMap(void);


private:

    ///缓存的当前帧的彩色图像
    cv::Mat miCacheCurrentRGB;
    ///缓存的当前帧的深度图像
    cv::Mat miCacheCurrentDepth;
    ///缓存的当前帧的特征点
    std::vector<cv::KeyPoint> mvCacheKeyPoints;
    ///缓存的地图点数据
    // std::vector<MapPoint::Ptr> mvCacheMap;
    ///缓存的帧的位姿
    Eigen::Isometry3d mmCacheFrameTwc;


    ///深度图像可视化使用的颜色环
    // std::vector<Color> mvHueCircle;

    ///深度测量值的最小值
    unsigned int mnDepthMin;
    ///深度测量值的最大值
    unsigned int mnDepthMax;
    ///深度图彩色化时的映射参数
    double mfDepthColoredFactor;
    
    

    ///准备显示的当前帧彩色图像
    cv::Mat miCurrentRGB;
    ///准备显示的当前帧深度图像
    cv::Mat miCurrentDepth;

    ///是否显示浮动的RGB图像窗口
    bool mbFloatRGBView;
    ///是否显示浮动的深度图像窗口
    bool mbFloatDepthView;
    ///深度图像是否以伪彩色显示
    bool mbDepthColored;

    ///浮动RGB图像窗口标题
    const std::string msCurRGBWinName;
    ///浮动深度图像窗口标题
    const std::string msCurDepthWinName;
    





    ///外部请求终止当前进程的标志
    bool mbRequestStop;
    ///当前进程是否已经停止的标志
    bool mbStateStop;
    ///用户是否在查看器中按下ESC键的标志
    bool mbESCPressed;
    ///退出模式.为True的时候,当系统进程结束时,当前查看器进程也将会结束;反之则当前进程会等待,直到用户关闭窗口
    bool mbQuitAfterEnding;

    //不安全的类型
    unsigned char* mpImageCache;
    unsigned char* mpStatusImageCache;

    ///配置器指针
    // DEPRECATED
    // Config::Ptr mpConfig;

    ///运行查看器的线程
    std::shared_ptr<std::thread> mthreadViewer;


    
    ///线程互斥量
    std::mutex mMutexRequestStop;
    std::mutex mMutexStateStop;
    std::mutex mMutexESCPressed;

    ///读写有关当前帧的数据的时候的互斥量
    std::mutex mMutexFrameData;
    ///读写地图缓存的时候的互斥量
    std::mutex mMutexMapData;

private:

    // 一些用于控制绘图的变量. 一次写入,全程只读

    // 窗口的大小
    size_t mnWindowWitdh = 1024;
    size_t mnWindowHigh  = 768;
    
    // 真的是原始图像的大小, 我们先默认是 640 x 480吧
    size_t  mnImageHigh = 480;
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
    float mfCameraSize = 0.5f;

    // 窗口标题
    std::string mstrWindowTitle = std::string("MYNT EYE D Evalution Tool");

    
};  //class Viewer



#endif //__VIEWER_H__