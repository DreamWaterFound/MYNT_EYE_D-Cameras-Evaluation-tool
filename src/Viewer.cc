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
    // 状态栏初始图像生成
    mstrStatusBar = std::string("Viewer Laucnhed.");
    mbStatusBarUpdated = true;


    // 颜色环初始化
    mvHueCircle.reserve(1536);
    mvHueCircle.resize(1536);

    for (int i = 0;i < 255;i++)
	{
		mvHueCircle[i].r = 255;
		mvHueCircle[i].g = i;
		mvHueCircle[i].b = 0;

		mvHueCircle[i+255].r = 255-i;
		mvHueCircle[i+255].g = 255;
		mvHueCircle[i+255].b = 0;

		mvHueCircle[i+511].r = 0;
		mvHueCircle[i+511].g = 255;
		mvHueCircle[i+511].b = i;

		mvHueCircle[i+767].r = 0;
		mvHueCircle[i+767].g = 255-i;
		mvHueCircle[i+767].b = 255;

		mvHueCircle[i+1023].r = i;
		mvHueCircle[i+1023].g = 0;
		mvHueCircle[i+1023].b = 255;

		mvHueCircle[i+1279].r = 255;
		mvHueCircle[i+1279].g = 0;
		mvHueCircle[i+1279].b = 255-i;
	}

	mvHueCircle[1534].r = 0;
	mvHueCircle[1534].g = 0;
	mvHueCircle[1534].b = 0;

	mvHueCircle[1535].r = 255;
	mvHueCircle[1535].g = 255;
	mvHueCircle[1535].b = 255;

    // 目前的相机参数中是这样子设定的
    mnDepthMax = 10000;
    mnDepthMin = 100;
    mdDepthColoredFactor = 1.0 * 1536 / (mnDepthMax-mnDepthMin);
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
            // pangolin::Attach::Pix(mnStatusBarHigh + mfLeftImageViewerHigh),
            pangolin::DisplayBase().bottom,
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
    
    pangolin::Var<bool> widgetBtnQuitMode("Panel.D1000-50",true,true);
    // pangolin::Var<bool> widgetBtnFloatRGBView("Panel.Float RGB View",false,true);
    // pangolin::Var<bool> widgetBtnFloatDepthView("Panel.Float Depth View",false,true);
    // pangolin::Var<bool> widgetBtnDepthColored("Panel.Depth colored",false,true);
    
    // Caches
    mpImageCache=new unsigned char[
        mnImageHigh * mnImageWidth *3];

    mpStatusImageCache=new unsigned char[
        mnStatusBarHigh * mnStatusBarWidth *3];

    // 左目图像显示窗口
    pangolin::View& leftImgViewer = pangolin::Display("Left_Image")
      .SetBounds(
          pangolin::Attach::Pix(mfLeftImageViewerHigh + mnStatusBarHigh),
          pangolin::Attach::Pix(mnStatusBarHigh),
          pangolin::Attach::ReversePix( 
                mfLeftImageViewerWidth  + 
                mfRightImageViewerWidth +
                mfDepthImageViewerWidth +
                mfPloterWidth),
          pangolin::Attach::ReversePix( 
                mfRightImageViewerWidth +
                mfDepthImageViewerWidth +
                mfPloterWidth),
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
                mfDepthImageViewerWidth +
                mfPloterWidth),
          pangolin::Attach::ReversePix(
                mfDepthImageViewerWidth +
                mfPloterWidth),
          (1.0f)*mnImageWidth/mnImageHigh);

    mupRightImageTexture.reset(new pangolin::GlTexture(
        mnImageWidth, mnImageHigh,
        GL_RGB,
        false,
        0,
        GL_RGB,
        GL_UNSIGNED_BYTE));

    // 图形窗口3 
    pangolin::View& depthImgViewer = pangolin::Display("Depth_image")
      .SetBounds(
          pangolin::Attach::Pix(mfDepthImageViewerHigh + mnStatusBarHigh),
          pangolin::Attach::Pix(mnStatusBarHigh),
          pangolin::Attach::ReversePix(
                mfDepthImageViewerWidth + 
                mfPloterWidth),
          pangolin::Attach::ReversePix(mfPloterWidth),
          (1.0f)*mnImageWidth/mnImageHigh);

    mupDepthImageTexture.reset(new pangolin::GlTexture(
        mnImageWidth, mnImageHigh,
        GL_RGB,
        false,
        0,
        GL_RGB,
        GL_UNSIGNED_BYTE));

    // 状态条 
    pangolin::View& statusViewer = pangolin::Display("Status_bar")
      .SetBounds(
          pangolin::DisplayBase().bottom,
          pangolin::Attach::Pix(mnStatusBarHigh),
        //   pangolin::Attach::ReversePix(mnStatusBarWidth),
          pangolin::Attach::ReversePix(mnStatusBarWidth),
          pangolin::DisplayBase().right,
          (-1.0f)*mnStatusBarWidth / mnStatusBarHigh);


    LOG(DEBUG)<<"[Viewer::run] mnStatusBarHigh = "<<mnStatusBarHigh<<", mnStatusBarWidth = "<<mnStatusBarWidth;


    mupStatusImageTexture = std::unique_ptr<pangolin::GlTexture>(
        new pangolin::GlTexture(
            mnStatusBarWidth, mnStatusBarHigh,
            GL_RGB,
            false,
            0,
            GL_RGB,
            GL_UNSIGNED_BYTE)
    );
     

    // 处理 Logger 和 Plotter
    mspImgExpoTimeLogger = std::shared_ptr<pangolin::DataLog>(
        new pangolin::DataLog());

    {
        std::vector<std::string> vstrLExpoTimeLabels;
        vstrLExpoTimeLabels.push_back(std::string("Left Expose Time"));
        vstrLExpoTimeLabels.push_back(std::string("Right Expose Time"));
        mspImgExpoTimeLogger->SetLabels(vstrLExpoTimeLabels);
    }

    mspExpoTimePlotter = std::shared_ptr<pangolin::Plotter>(
        new pangolin::Plotter(
            mspImgExpoTimeLogger.get(),     // Logger
            0.0f,                           // left
            300.0f,                         // right
            1000.0f,                           // bottom
            4000.0f,                         // top
            10.0f,                          // x-section
            100.0f                          // y-section
        )
    );

    mspExpoTimePlotter->SetBounds(
        pangolin::Attach::Pix(mnStatusBarHigh),
        pangolin::Attach::Pix(mfDepthImageViewerHigh + mnStatusBarHigh),
        // 先这么设置着
        pangolin::Attach::ReversePix(mfPloterWidth),
        pangolin::DisplayBase().right
    );

    mspExpoTimePlotter->Track("$i");

    pangolin::DisplayBase().AddDisplay(*mspExpoTimePlotter);

    // IMU 的 Logger 和 Ploter
    mspAccelLogger = std::shared_ptr<pangolin::DataLog>(
        new pangolin::DataLog());
    {
        std::vector<std::string> vstrAccelLabels;
        vstrAccelLabels.push_back(std::string("Accel X"));
        vstrAccelLabels.push_back(std::string("Accel Y"));
        vstrAccelLabels.push_back(std::string("Accel Z"));
        mspAccelLogger->SetLabels(vstrAccelLabels);
    }

    mspAccelPlotter = std::shared_ptr<pangolin::Plotter>(
        new pangolin::Plotter(
            mspAccelLogger.get(),     // Logger
            0.0f,                           // left
            3000.0f,                         // right
            -2.0f,                           // bottom
            +2.0f,                         // top
            100.0f,                          // x-section
            2.0f                          // y-section
        )
    );
    mspAccelPlotter->SetBounds(
        pangolin::Attach::Pix(2 * mfDepthImageViewerHigh + mnStatusBarHigh),
        // pangolin::DisplayBase().top,
        pangolin::Attach::Pix(3 * mfDepthImageViewerHigh + mnStatusBarHigh),
        // 先这么设置着
        pangolin::Attach::ReversePix(mfPloterWidth),
        pangolin::DisplayBase().right
    );
    mspAccelPlotter->Track("$i");
    pangolin::DisplayBase().AddDisplay(*mspAccelPlotter);

    mspGyroLogger = std::shared_ptr<pangolin::DataLog>(
        new pangolin::DataLog());
    {
        std::vector<std::string> vstrGyroLabels;
        vstrGyroLabels.push_back(std::string("Gyro X"));
        vstrGyroLabels.push_back(std::string("Gyro Y"));
        vstrGyroLabels.push_back(std::string("Gyro Z"));
        mspGyroLogger->SetLabels(vstrGyroLabels);
    }

    mspGyroPlotter = std::shared_ptr<pangolin::Plotter>(
        new pangolin::Plotter(
                mspGyroLogger.get(),     // Logger
                0.0f,                           // left
                3000.0f,                         // right
                -100.0f,                           // bottom
                +100.0f,                         // top
                100.0f,                          // x-section
                10.0f                          // y-section
        )
    );
    mspGyroPlotter->SetBounds(
        pangolin::Attach::Pix(3 * mfDepthImageViewerHigh + mnStatusBarHigh),
        // pangolin::DisplayBase().top,
        pangolin::Attach::Pix(4 * mfDepthImageViewerHigh + mnStatusBarHigh),
        // 先这么设置着
        pangolin::Attach::ReversePix(mfPloterWidth),
        pangolin::DisplayBase().right
    );
    mspGyroPlotter->Track("$i");
    pangolin::DisplayBase().AddDisplay(*mspGyroPlotter);

    // 温度
    mspIMUTempLogger = std::shared_ptr<pangolin::DataLog>(
        new pangolin::DataLog());
    {
        std::vector<std::string> vstrAccelLabels;
        vstrAccelLabels.push_back(std::string("IMU Temperature"));
        mspIMUTempLogger->SetLabels(vstrAccelLabels);
    }

    mspIMUTempPlotter = std::shared_ptr<pangolin::Plotter>(
        new pangolin::Plotter(
            mspIMUTempLogger.get(),     // Logger
            0.0f,                           // left
            6000.0f,                         // right
            20.0f,                           // bottom
            45.0f,                         // top
            200.0f,                          // x-section
            2.0f                          // y-section
        )
    );
    mspIMUTempPlotter->SetBounds(
        pangolin::Attach::Pix(mfDepthImageViewerHigh + mnStatusBarHigh),
        // pangolin::DisplayBase().top,
        pangolin::Attach::Pix(2 * mfDepthImageViewerHigh + mnStatusBarHigh),
        // 先这么设置着
        pangolin::Attach::ReversePix(mfPloterWidth),
        pangolin::DisplayBase().right
    );
    mspIMUTempPlotter->Track("$i");
    pangolin::DisplayBase().AddDisplay(*mspIMUTempPlotter);


    cv::Mat imageNon = cv::imread("./img/no_image.png");
    if(imageNon.empty())
    {
        std::cout<<"\e[1;31mCan not open imageNon!\e[0m"<<std::endl;
        exit(0);
    }

    // TODO 快捷键处理
    // pangolin::RegisterKeyPressCallback('\t', Viewer::NonFullScreen);
    pangolin::RegisterKeyPressCallback('\e', 
        std::bind(&Viewer::OnESC, this));

    // 初始化各个纹理
    cv2glIamge(*mupLeftImageTexture, imageNon);
    cv2glIamge(*mupRightImageTexture,imageNon);
    cv2glIamge(*mupDepthImageTexture,imageNon);
    mImgLeft = mImgRight = mImgDepth = imageNon;

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
        DrawRightImageTexture();
        

        // 深度图像更新
        depthImgViewer.Activate();
        DrawDepthImageTexture();
       

        //底部状态栏更新
        statusViewer.Activate();
        DrawStatusBarImg();

        pangolin::FinishFrame();

        // 为了方便录屏使用, 可以避免录屏出现卡死的现象
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    std::cout<<"\e[1;33mStop request get.\e[0m"<<std::endl;

    // 销毁所有窗口
    pangolin::DestroyWindow(mstrWindowTitle);
    // 避免点击 x 退出窗口但是主线程中却没有任何提示
    OnESC();

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
void Viewer::DrawStatusBarImg(void)
{
    {
        std::lock_guard<std::mutex> lock(mMutexStatusBar);


        if(mbStatusBarUpdated)
        {
            LOG(DEBUG)<<"[Viewer::DrawStatusBarImg] mnStatusBarHigh = "<<mnStatusBarHigh<<", mnStatusBarWidth = "<<mnStatusBarWidth;

            mbStatusBarUpdated = false;
            cv::Mat statusImg(
                mnStatusBarHigh,
                mnStatusBarWidth,
                CV_8UC3,
                cv::Scalar(0.2*255,0.2*255,0.2*255));
            
            cv::putText(
                statusImg,
                mstrStatusBar,
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
        }
    }
    
    // 更新
    // NOTE  默认是 CPU => GPU 数据传输是四字节对齐, 但是有可能导致图像缩放之后并不能够严格一行四个字节, 所以这里重新设置一下
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    mupStatusImageTexture->Upload(mpStatusImageCache,GL_RGB,GL_UNSIGNED_BYTE);
    mupStatusImageTexture->RenderToViewport();
    glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
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

    if(mbCloudUpdated)
    {
        mbCloudUpdated = false;
        // 有了新的需要生成的点云, 上锁
        std::lock_guard<std::mutex> lockDepthImg(mMutexDepthImage);
        std::lock_guard<std::mutex> lockLeftImg(mMutexLeftImage);

        // 清空已有点云数据
        mvCloudPoints.clear();
        size_t rows = mImgDepth.rows;
        size_t cols = mImgDepth.cols;

        for(size_t nIdr = 0; nIdr < rows; ++nIdr)
        {
            for(size_t nIdc = 0; nIdc < cols; ++nIdc)
            {
                unsigned int intensity = mImgDepth.at<uint16_t>(nIdr, nIdc);
                // 深度值不合法, 那么我们就不管了
                if(intensity < mnDepthMin || intensity > mnDepthMax)
                {
                    continue;
                }

                float fDepth = static_cast<float>(intensity)/1000.0f;


                // 深度值合法才会生成新的点
                mvCloudPoints.emplace_back(
                    mImgLeft.at<uint8_t>(nIdr, 3*nIdc + 2),
                    mImgLeft.at<uint8_t>(nIdr, 3*nIdc + 1),
                    mImgLeft.at<uint8_t>(nIdr, 3*nIdc),
                    
                    
                    
                    // 0,
                    // 255,
                    // 0,
                    fDepth * (nIdc - mdLcx)/mdLfx,
                    fDepth * (nIdr - mdLcy)/mdLfy,
                    fDepth
                );
            }
        }    
    }

    // 没有需要重新绘制的点云, 直接显示已经有的点云就可以了
    glPointSize(2);
    glBegin(GL_POINTS);

    for(const auto& point : mvCloudPoints)
    {
        glColor3f(point.r/255.0, point.g/255.0, point.b/255.0);
        glVertex3f(point.x, point.y, point.z);
    }

    glEnd();
    
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
        mnLeftTimestamp = nLeftTimestamp;
    }

    // 曝光时间更新
    {
        std::lock_guard<std::mutex> lock(mMutexImageExposeTime);
        
        mnLeftExposeTime = nExposeTime;
        // 如果此时右目图像已经有曝光时间数据
        if(mbRightETUpdated)
        {
            mspImgExpoTimeLogger->Log(mnLeftExposeTime, mnRightExposeTime);
            
            // 然后清空标志
            mbRightETUpdated = false;
            mbLeftETUpdated = false;
        }
        else
        {
            mbLeftETUpdated = true;
        }
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

void Viewer::UpdateRightImage(
        const cv::Mat&  imgRight,
        const uint16_t& nExposeTime,
        const uint32_t& nRightTimestamp)
{
    bool bLastUpdateFlag;
    {
        std::lock_guard<std::mutex> lock(mMutexRightImage);
        bLastUpdateFlag = mbRightImagesUpdated;
        mbRightImagesUpdated = true;
        
        mImgRight = imgRight.clone();
        mnRightTimestamp = nRightTimestamp;
    }

    // 曝光时间更新
    {
        std::lock_guard<std::mutex> lock(mMutexImageExposeTime);
        
        mnRightExposeTime = nExposeTime;
        // 如果此时左目图像已经有曝光时间数据
        if(mbLeftETUpdated)
        {
            mspImgExpoTimeLogger->Log(mnLeftExposeTime, mnRightExposeTime);
            // 然后清空标志
            mbRightETUpdated = false;
            mbLeftETUpdated = false;
        }
        else
        {
            mbRightETUpdated = true;
        }
    }

    // 写在这里是考虑到调用GLOG会耽误一些时间
    if(bLastUpdateFlag)
    {
        // 说明上一次更新后数据还没有来得及被使用
        LOG(WARN)<<"[Viewer::UpdateRightImage] last right frame was not used!";
    }

}

void Viewer::DrawRightImageTexture(void)
{
    std::unique_lock<std::mutex> lock(mMutexRightImage);
    glColor3f(1.0,1.0,1.0);
    mbRightImagesUpdated = false;
    if(cv2glIamge(*mupRightImageTexture, mImgRight))
        // 随机纹理
        mupRightImageTexture->RenderToViewport();
}

void Viewer::UpdateDepthImage(
        const cv::Mat&  imgDepth,
        const uint32_t& nDepthTimestamp)
{
    bool bLastUpdateFlag;
    {
        std::lock_guard<std::mutex> lock(mMutexDepthImage);
        bLastUpdateFlag = mbDepthImagesUpdated;
        mbDepthImagesUpdated = true;
        
        mImgDepth = imgDepth.clone();
        mnDepthTimestamp = nDepthTimestamp;
    }

    // 写在这里是考虑到调用 GLOG 会耽误一些时间
    if(bLastUpdateFlag)
    {
        // 说明上一次更新后数据还没有来得及被使用
        LOG(WARN)<<"[Viewer::UpdateDepthImage] last depth frame was not used!";
    }
}

void Viewer::DrawDepthImageTexture(void)
{
    std::unique_lock<std::mutex> lock(mMutexDepthImage);
    glColor3f(1.0,1.0,1.0);

    if(mbDepthImagesUpdated == false || mImgDepth.empty() == true)
    {
        // 没有发生的深度图像的更新, 或者是更新成为了空图像, 就直接使用上次的绘制结果
        mupDepthImageTexture->RenderToViewport();
        return ;
    }

    // 发生的深度图像的更新, 需要重新绘制
    mbDepthImagesUpdated = false;
    
    // 如果不为空, 为了进行可视化, 这里要重新写点东西
    size_t rows = mImgDepth.rows;
    size_t cols = mImgDepth.cols;

    unsigned int index;

    for(size_t nIdr = 0; nIdr < rows; ++nIdr)
    {
        for(size_t nIdc = 0; nIdc < cols; ++nIdc)
        {
            unsigned int intensity = mImgDepth.at<uint16_t>(rows-1-nIdr,nIdc); // +(((unsigned int)(miCurrentDepth.at<unsigned char>(rows-1-i,j*2+1)))<<8);
                    
            if(intensity < mnDepthMin)         index=1534;

            else 
            {
                index = (intensity-mnDepthMin) * mdDepthColoredFactor;
                //如果映射超过了彩色区，说明太远了，直接显示成白色
                if(index > 1534) index = 1535;
            }

            //这里目前就只移动了7位了～
            mpImageCache[nIdr * 3 * cols+nIdc * 3    ] = mvHueCircle[index].r;
            mpImageCache[nIdr * 3 * cols+nIdc * 3 + 1] = mvHueCircle[index].g;
            mpImageCache[nIdr * 3 * cols+nIdc * 3 + 2] = mvHueCircle[index].b;

        }
    }

    mupDepthImageTexture->Upload(mpImageCache,GL_RGB,GL_UNSIGNED_BYTE);
    mupDepthImageTexture->RenderToViewport();

    // 点云也需要重新绘制
    mbCloudUpdated = true;
}

void Viewer::UpdateStatusBar(const std::string& strStatusString)
{
    std::lock_guard<std::mutex> lock(mMutexStatusBar);
    mstrStatusBar  = strStatusString;
    mbStatusBarUpdated = true;
}

// 更新 IMU 数据
void Viewer::UpdateAccel(
    const double dX, const double dY, const double dZ,
    const double dT,
    const uint64_t nTimestamp)
{
    {
        std::lock_guard<std::mutex> lock(mMutexIMUAccel);
        mdAccX = dX;
        mdAccY = dY;
        mdAccZ = dZ;
        mnAccTimestamp = nTimestamp;
    }
    mspAccelLogger->Log(mdAccX, mdAccY, mdAccZ);

    // 更新温度
    {
        std::lock_guard<std::mutex> lock(mMutexIMUTemp);
        mdIMUTemp = dT;
    }
    mspIMUTempLogger->Log(mdIMUTemp);
}

void Viewer::UpdateGyro(
    const double dX, const double dY, const double dZ,
    const double dT,
    const uint64_t nTimestamp)
{
    {
        std::lock_guard<std::mutex> lock(mMutexIMUGyro);
        mdGyroX = dX;
        mdGyroY = dY;
        mdGyroZ = dZ;
        mnGyroTimestamp = nTimestamp;
    }
    // 更新Logger
    mspGyroLogger->Log(mdGyroX, mdGyroY, mdGyroZ);

    // 更新温度
    {
        std::lock_guard<std::mutex> lock(mMutexIMUTemp);
        mdIMUTemp = dT;
    }
    mspIMUTempLogger->Log(mdIMUTemp);   
}




