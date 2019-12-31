/**
 * @file Viewer.cpp
 * @author guoqing (1337841346@qq.com)
 * @brief 可视化查看器的实现（后面的文件创建时间不准8）
 * @version 0.1
 * @date 2019-03-30
 * 
 * @copyright Copyright (c) 2019
 * 
 */

// TODO 注意, Viewer::Run() 中在绘图的时候如果发现要获取的数据被锁了,应当跳过去, 以保证绘图界面的流畅工作

#include "Viewer.h"
// 原则上不应该包含这个头文件的, 不过为了获取当前窗口的大小我只找到了这样的方式
#include <pangolin/display/display_internal.h>

// GLog
#include <glog/logging.h>


Viewer::Viewer()
    :mbRequestStop(false),mbStateStop(true),mbESCPressed(false),
     mpImageCache(nullptr), mpStatusImageCache(nullptr)
{
    ;
}


Viewer::~Viewer()
{
    // 删除缓存
    if(mpImageCache)
    {
        delete[] mpImageCache;
    }

    if(mpStatusImageCache)
    {
        delete[] mpStatusImageCache;
    }
}

//进程主函数
void Viewer::run(void)
{
    {
        std::lock_guard<std::mutex> lock(mMutexStateStop);
        mbStateStop=false;
    }

    //============================== 配置部分 ===================================
    //创建窗口
    // pangolin::PangolinGl&  __attribute__((__used__)) windowHandle = dynamic_cast<pangolin::PangolinGl&>(
        pangolin::CreateWindowAndBind(
            mstrWindowTitle,
            mnWindowWitdh,
            mnWindowHigh);
        // );

    //使能深度测试
    glEnable(GL_DEPTH_TEST);

    //创建3D场景渲染器, 这里视图的一些参数直接使用了ORB-SLAM2中使用的参数
    pangolin::OpenGlRenderState scenceRender(
        pangolin::ProjectionMatrix(
            mnWindowWitdh,mnWindowHigh,
            2000, 2000,
            512, 389, 
            0.1,1000),
        pangolin::ModelViewLookAt(0,-100,-0.1, 0,0,0,0.0,-1.0, 0.0));

    //地图查看器
    pangolin::View& mapViewer = pangolin::CreateDisplay()
        .SetBounds(
            pangolin::Attach::Pix(mnStatusBarHigh + mfLeftImageViewerHigh),
            pangolin::DisplayBase().top,
            pangolin::Attach::Pix(mnPanelWitdh),
            pangolin::DisplayBase().right);
        mapViewer.SetHandler(new pangolin::Handler3D(scenceRender));

    // 控制面板
    pangolin::CreatePanel("Panel")
        .SetBounds(
            pangolin::DisplayBase().bottom,
            pangolin::DisplayBase().top,
            pangolin::DisplayBase().left,
            pangolin::Attach::Pix(mnPanelWitdh));
    
    pangolin::Var<bool> widgetBtnQuitMode("Panel.Quit After Ending",true,true);
    pangolin::Var<bool> widgetBtnFloatRGBView("Panel.Float RGB View",false,true);
    pangolin::Var<bool> widgetBtnFloatDepthView("Panel.Float Depth View",false,true);
    pangolin::Var<bool> widgetBtnDepthColored("Panel.Depth colored",false,true);
    
    // Caches
    mpImageCache=new unsigned char[
        mnImageHigh * mnImageWidth *3];

    mpStatusImageCache=new unsigned char[
        mnStatusBarHigh * mnStatusBarWidth *3];

    // 图形窗口1
    pangolin::View& leftImgViewer = pangolin::Display("Left_Image")
      .SetBounds(
          pangolin::Attach::Pix(mfLeftImageViewerHigh + mnStatusBarHigh),
          pangolin::Attach::Pix(mnStatusBarHigh),
          pangolin::Attach::ReversePix( 
                mfLeftImageViewerWidth  + 
                mfRightImageViewerWidth +
                mfDepthImageViewerWidth),
          pangolin::Attach::ReversePix( 
                mfRightImageViewerWidth +
                mfDepthImageViewerWidth),
          (1.0f)*mnImageWidth/mnImageHigh);

    mupLeftImageTexture.reset(new pangolin::GlTexture(
        mnImageWidth, mnImageHigh,
        GL_RGB,
        false,
        0,
        GL_RGB,
        GL_UNSIGNED_BYTE));

    // 图形窗口2
    pangolin::View& rightImgViewer = pangolin::Display("Right_Image")
      .SetBounds(
          pangolin::Attach::Pix(mfRightImageViewerHigh + mnStatusBarHigh),
          pangolin::Attach::Pix(mnStatusBarHigh),
          pangolin::Attach::ReversePix(
                mfRightImageViewerWidth +
                mfDepthImageViewerWidth),
          pangolin::Attach::ReversePix(mfDepthImageViewerWidth),
          (1.0f)*mnImageWidth/mnImageHigh);

    pangolin::GlTexture rightImageTexture(
        mnImageWidth, mnImageHigh,
        GL_RGB,
        false,
        0,
        GL_RGB,
        GL_UNSIGNED_BYTE);

    // 图形窗口3 
    pangolin::View& depthImgViewer = pangolin::Display("Depth_image")
      .SetBounds(
          pangolin::Attach::Pix(mfDepthImageViewerHigh + mnStatusBarHigh),
          pangolin::Attach::Pix(mnStatusBarHigh),
          pangolin::Attach::ReversePix(mfDepthImageViewerWidth),
          1.0f,
          (1.0f)*mnImageWidth/mnImageHigh);

    pangolin::GlTexture depthImageTexture(
        mnImageWidth, mnImageHigh,
        GL_RGB,
        false,
        0,
        GL_RGB,
        GL_UNSIGNED_BYTE);

    // 状态条 
    pangolin::View& statusViewer = pangolin::Display("Status_bar")
      .SetBounds(
          pangolin::DisplayBase().bottom,
          pangolin::Attach::Pix(mnStatusBarHigh),
          pangolin::Attach::ReversePix( 
                mfLeftImageViewerWidth  + 
                mfRightImageViewerWidth +
                mfDepthImageViewerWidth),
          pangolin::DisplayBase().right,
          (1.0f)*mnStatusBarWidth/mnStatusBarHigh);

    pangolin::GlTexture statusImageTexture(
        mnStatusBarWidth, mnStatusBarHigh,
        GL_RGB,
        false,
        0,
        GL_RGB,
        GL_UNSIGNED_BYTE);

    // DEBUG 
    cv::Mat image1 = cv::imread("./img/1.png");
    if(image1.empty())
    {
        std::cout<<"\e[1;31mCan not open image1!\e[0m"<<std::endl;
        exit(0);
    }

    cv::Mat image2 = cv::imread("./img/no_image.png");
    if(image2.empty())
    {
        std::cout<<"\e[1;31mCan not open image2!\e[0m"<<std::endl;
        exit(0);
    }

    cv::Mat image3 = cv::imread("./img/3.png");
    if(image3.empty())
    {
        std::cout<<"\e[1;31mCan not open image3!\e[0m"<<std::endl;
        exit(0);
    }

    // TODO 快捷键处理
    // pangolin::RegisterKeyPressCallback('\t', Viewer::NonFullScreen);
    pangolin::RegisterKeyPressCallback('\e', 
        std::bind(&Viewer::OnESC, this));

    // 初始化各个纹理
    cv2glIamge(*mupLeftImageTexture,image2);
    cv2glIamge(rightImageTexture,image2);
    cv2glIamge(depthImageTexture,image2);

    //=========================== 显示刷新部分 ================================

    while( !pangolin::ShouldQuit() && !isRequestStop() )
    {
        //清空信息准备绘制
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // TODO 处理按钮信息
        mbQuitAfterEnding   = (bool)widgetBtnQuitMode;

        // MapViewer刷新
        mapViewer.Activate(scenceRender);
        drawMapViewer();
        glColor3f(0.3,0.3,0.3);
        glFlush();
        
        // 左目图像更新
        leftImgViewer.Activate();
        DrawLeftImageTexture();
        

        // 右目图像更新
        rightImgViewer.Activate();
        glColor3f(1.0,1.0,1.0);
        if(cv2glIamge(rightImageTexture,image3))
            rightImageTexture.RenderToViewport();

        // 深度图像更新
        depthImgViewer.Activate();
        glColor3f(1.0,1.0,1.0);
        if(cv2glIamge(depthImageTexture,image2))
            depthImageTexture.RenderToViewport();

        //底部状态栏更新
        statusViewer.Activate();
        glColor3f(1.0,1.0,1.0);
        drawStatusBarImg(statusImageTexture);
        statusImageTexture.RenderToViewport();

        pangolin::FinishFrame();
    }

    std::cout<<"\e[1;33mStop request get.\e[0m"<<std::endl;

    // 销毁所有窗口
    pangolin::DestroyWindow(mstrWindowTitle);

    // // 这里笼统地认为已经按下了ESC键
    // setESCPressed();
    setStateStop();
}

//等待当前查看器结束
void Viewer::waitForStopped(void)
{
    if(mbQuitAfterEnding)
        requestStop(); 
}

// 将OpenCV格式的图像转换成为OpenGL的纹理
bool Viewer::cv2glIamge(pangolin::GlTexture& imageTexture, const cv::Mat img)
{
    if(img.empty()) return false;

    cv::Mat rgbFormatImg=img.clone();

    if(img.type()==CV_8UC1)
        cv::cvtColor(img,rgbFormatImg,CV_GRAY2RGB);
    
    //OpenGL中进行图像更新

    for(int i=0;i<rgbFormatImg.rows;i++) {
      for(int j=0;j<rgbFormatImg.cols;j++)
      {
        mpImageCache[i*3*rgbFormatImg.cols+j*3]=rgbFormatImg.at<unsigned char>(rgbFormatImg.rows-1-i,3*j+2);
        mpImageCache[i*3*rgbFormatImg.cols+j*3+1]=rgbFormatImg.at<unsigned char>(rgbFormatImg.rows-1-i,3*j+1);
        mpImageCache[i*3*rgbFormatImg.cols+j*3+2]=rgbFormatImg.at<unsigned char>(rgbFormatImg.rows-1-i,3*j);;
      }
    }

    imageTexture.Upload(mpImageCache,GL_RGB,GL_UNSIGNED_BYTE);

    return true;
}

// 绘制状态栏的图像
void Viewer::drawStatusBarImg(pangolin::GlTexture& imageTexture)
{
    
    std::stringstream ss;
    ss<<"Status Bar Demo.";

    cv::Mat statusImg(
        mnStatusBarHigh,
        mnStatusBarWidth,
        CV_8UC3,
        cv::Scalar(0.2*255,0.2*255,0.2*255));
    
    cv::putText(
        statusImg,
        ss.str(),
        //Point(5,statusImg.rows-5),
        cv::Point(10,statusImg.rows-10),
        cv::FONT_HERSHEY_PLAIN,
        1,
        cv::Scalar(255,255,255),
        1,8);
    
    for(int i=0;i<statusImg.rows;i++) {
      for(int j=0;j<statusImg.cols;j++)
      {
        mpStatusImageCache[i*3*statusImg.cols+j*3]=statusImg.at<unsigned char>(statusImg.rows-1-i,3*j+2);
        mpStatusImageCache[i*3*statusImg.cols+j*3+1]=statusImg.at<unsigned char>(statusImg.rows-1-i,3*j+1);
        mpStatusImageCache[i*3*statusImg.cols+j*3+2]=statusImg.at<unsigned char>(statusImg.rows-1-i,3*j);;
      }
    } 

    imageTexture.Upload(mpStatusImageCache,GL_RGB,GL_UNSIGNED_BYTE);
}

// 地图查看器更新
void Viewer::drawMapViewer(void)
{
    
    //改变背景颜色
    glClearColor(0.2,0.2,0.2,1.0);

    glColor3f(0.3f,0.3f,0.3f);
    glLineWidth(0.1);
    pangolin::glDraw_y0(1.0, 100);
    glLineWidth(2);
    pangolin::glDrawAxis(mfAxisSize);
    
    //绘制相机位姿. 这里只是绘制了一个最基本的处于坐标远点的相机位姿
    Eigen::Isometry3d T=Eigen::Isometry3d::Identity();
    {
        // NOTE 后续这里的操作需要添加线程锁. 这里就简单地
        // std::lock_guard<std::mutex> lock(mMutexFrameData);
        // T = mmCacheFrameTwc;
    }

    
    drawCamera(T);
   
    // TODO 点云的绘制可以放在这里
    
}

// 接口这里输入的位姿变换矩阵
void Viewer::drawCamera(Eigen::Isometry3d Twc,double size,double r,double g,double b)
{
    //可能需要根据情况对输入的位姿矩阵进行转置操作
    const float w = (size <= 0 ? 0.5f : size);
    const float h = w * 0.75;
    const float z = w * 0.6;

    std::vector<GLfloat> glTwc = eigen2glfloat(Twc);
    
    
    glPushMatrix();
    glMultMatrixf((GLfloat*)glTwc.data());
    
    glLineWidth(2);
    glColor3f(r,g,b);
    
    glBegin(GL_LINES);
    
    glVertex3f(0,0,0);
    glVertex3f(w,h,z);
    glVertex3f(0,0,0);
    glVertex3f(w,-h,z);
    glVertex3f(0,0,0);
    glVertex3f(-w,-h,z);
    glVertex3f(0,0,0);
    glVertex3f(-w,h,z);
    glVertex3f(w,h,z);
    glVertex3f(w,-h,z);
    glVertex3f(-w,h,z);
    glVertex3f(-w,-h,z);
    glVertex3f(-w,h,z);
    glVertex3f(w,h,z);
    glVertex3f(-w,-h,z);
    glVertex3f(w,-h,z);

    glEnd();
    glPopMatrix();
}

// 绘制相机模型
void Viewer::drawCamera(Eigen::Isometry3d Twc,double r,double g,double b)
{
    drawCamera(Twc,mfCameraSize,0.0f,1.0f,0.0f);
}

// 将 Eigen 类型的矩阵数据转换成为 OpenGL float 类型
std::vector<GLfloat> Viewer::eigen2glfloat(Eigen::Isometry3d T)
{
    //注意是列优先
    std::vector<GLfloat> res;
    
    for(int j=0;j<4;j++)
    {
        for(int i=0;i<4;i++)
        {
            res.push_back(T(i,j));
        }
    }
    
    return res;
}

void Viewer::UpdateLeftImage(
        const cv::Mat&  imgLeft,
        const uint16_t& nExposeTime,
        const uint32_t& nLeftTimestamp)
{
    bool bLastUpdateFlag;
    {
        std::lock_guard<std::mutex> lock(mMutexLeftImage);
        bLastUpdateFlag = mbLeftImagesUpdated;
        mbLeftImagesUpdated = true;
        
        mImgLeft = imgLeft.clone();
        mnLeftExposeTime = nExposeTime;
        mnLeftTimestamp = nLeftTimestamp;
    }

    // 写在这里是考虑到调用GLOG会耽误一些时间
    if(bLastUpdateFlag)
    {
        // 说明上一次更新后数据还没有来得及被使用
        LOG(WARN)<<"[Viewer::UpdateLeftImage] last left frame was not used!";
    }
}

void Viewer::DrawLeftImageTexture(void)
{
    std::unique_lock<std::mutex> lock(mMutexLeftImage);
    // if(lock.try_lock())
    {
        // 如果成功获取锁, 那么就绘制
        glColor3f(1.0,1.0,1.0);
        mbLeftImagesUpdated = false;
        if(cv2glIamge(*mupLeftImageTexture, mImgLeft))
            // 随机纹理
            mupLeftImageTexture->RenderToViewport();
    }
    // else
    // {
    //     // 锁定失败, 还是使用上次的结果绘图
    //     mupLeftImageTexture->RenderToViewport();
    // }
}
