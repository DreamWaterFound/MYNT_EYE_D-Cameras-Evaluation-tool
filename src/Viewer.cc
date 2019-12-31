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


#include "Viewer.h"
// 原则上不应该包含这个头文件的, 不过为了获取当前窗口的大小我只找到了这样的方式
#include <pangolin/display/display_internal.h>


Viewer::Viewer()
    :
    //  msCurRGBWinName(config->getCurRGBWinName()),
    //  msCurDepthWinName(config->getCurDepthWinName()),
    //  mpConfig(config),
     
     mbFloatRGBView(false),mbFloatDepthView(false),
     mbRequestStop(false),mbStateStop(true),mbESCPressed(false),
     
     mpImageCache(nullptr)
{
    // //颜色环初始化
    // mvHueCircle.reserve(1536);
    // mvHueCircle.resize(1536);

    // for (int i = 0;i < 255;i++)
	// {
	// 	mvHueCircle[i].r = 255;
	// 	mvHueCircle[i].g = i;
	// 	mvHueCircle[i].b = 0;

	// 	mvHueCircle[i+255].r = 255-i;
	// 	mvHueCircle[i+255].g = 255;
	// 	mvHueCircle[i+255].b = 0;

	// 	mvHueCircle[i+511].r = 0;
	// 	mvHueCircle[i+511].g = 255;
	// 	mvHueCircle[i+511].b = i;

	// 	mvHueCircle[i+767].r = 0;
	// 	mvHueCircle[i+767].g = 255-i;
	// 	mvHueCircle[i+767].b = 255;

	// 	mvHueCircle[i+1023].r = i;
	// 	mvHueCircle[i+1023].g = 0;
	// 	mvHueCircle[i+1023].b = 255;

	// 	mvHueCircle[i+1279].r = 255;
	// 	mvHueCircle[i+1279].g = 0;
	// 	mvHueCircle[i+1279].b = 255-i;
	// }

	// mvHueCircle[1534].r = 0;
	// mvHueCircle[1534].g = 0;
	// mvHueCircle[1534].b = 0;

	// mvHueCircle[1535].r = 255;
	// mvHueCircle[1535].g = 255;
	// mvHueCircle[1535].b = 255;

    // mnDepthMax=mpConfig->getDepthMax();
    // mnDepthMin=mpConfig->getDepthMin();
    // mfDepthColoredFactor=1.0*1536/(mnDepthMax-mnDepthMin);

    // // 颜色环是生成了,这个颜色环是没有什么太多问题的;但是记得颜色范围是0~255,如果使用opengl的话需要转换到0~1



    //创建进程
    // NOTE 
    // mtViewer = make_shared<thread>(bind(&Viewer::run,this));
}


Viewer::~Viewer()
{
    cv::destroyAllWindows();
    // DEPRECATED
    // mbInstanceCreated=false;

    // 删除缓存
    delete[] mpImageCache;
    delete[] mpStatusImageCache;
}

// DEPRECATED
// Viewer::Ptr Viewer::createInstance(Config::Ptr config)
// {
//     if(mbInstanceCreated)
//     {
//         //已经创建过实例
//         cout<<"\e[1;31mA Viewer instance has been created!\e[0m"<<endl;
//         throw std::exception();
//         return Viewer::Ptr(nullptr);
//     }
//     else
//     {
//         mbInstanceCreated=true;
//         return Viewer::Ptr(new Viewer(config));
//     }
// }

//进程主函数
void Viewer::run(void)
{
    mbStateStop=false;

    //============================== 配置部分 ===================================
    //创建窗口
    pangolin::PangolinGl& windowHandle = dynamic_cast<pangolin::PangolinGl&>(
        pangolin::CreateWindowAndBind(
        mstrWindowTitle,
        mnWindowWitdh,
        mnWindowHigh));

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
        //   pangolin::Attach::Pix(mnPanelWitdh + mfLeftImageViewerWidth),
          (1.0f)*mnImageWidth/mnImageHigh);

    pangolin::GlTexture leftImageTexture(
        mnImageWidth, mnImageHigh,
        GL_RGB,
        false,
        0,
        GL_RGB,
        GL_UNSIGNED_BYTE);

    // 图形窗口2
    pangolin::View& rightImgViewer = pangolin::Display("Right_Image")
      .SetBounds(
          pangolin::Attach::Pix(mfRightImageViewerHigh + mnStatusBarHigh),
          pangolin::Attach::Pix(mnStatusBarHigh),
          pangolin::Attach::ReversePix(
                mfRightImageViewerWidth +
                mfDepthImageViewerWidth),
          pangolin::Attach::ReversePix(mfDepthImageViewerWidth),
          
        //   pangolin::Attach::Pix(mnPanelWitdh + mfLeftImageViewerWidth + mfRightImageViewerWidth),
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

    //快捷键处理
    //pangolin::RegisterKeyPressCallback('\t', Viewer::NonFullScreen);

//    cv::namedWindow("Current Frame");
    
    //=========================== 显示刷新部分 ================================

    while( !pangolin::ShouldQuit() && !isRequestStop() )
    {
        //清空信息准备绘制
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        // DEBUG 处理按钮信息
        // mbQuitAfterEnding   = (bool)widgetBtnQuitMode;
        // mbFloatRGBView      = (bool)widgetBtnFloatRGBView;
        // mbFloatDepthView    = (bool)widgetBtnFloatDepthView;
        // mbDepthColored      = (bool)widgetBtnDepthColored;
        

        // MapViewer刷新
        mapViewer.Activate(scenceRender);
        drawMapViewer();
        glColor3f(0.3,0.3,0.3);
        glFlush();

        
        // 左目图像更新
        // widgetBtnFloatRGBView = mbFloatRGBView = drawRGBView();
        leftImgViewer.Activate();
        glColor3f(1.0,1.0,1.0);
        if(cv2glIamge(leftImageTexture,image1))
            // 随机纹理
            leftImageTexture.RenderToViewport();

        // 右目图像更新
        // widgetBtnFloatDepthView=mbFloatDepthView=drawDepthView(depthImageTexture,mbDepthColored,isOk);
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

    //销毁所有窗口
    // TODO 
    pangolin::DestroyWindow(mstrWindowTitle);

    //这里笼统地认为已经按下了ESC键
    setESCPressed();
    setStateStop();
}

//等待当前查看器结束
void Viewer::waitForStopped(void)
{
    if(mbQuitAfterEnding)
        requestStop();

    // ? 还有用吗
    // if(mtViewer->joinable())
    //     mtViewer->join();
 
}

bool Viewer::cv2glIamge(pangolin::GlTexture& imageTexture, const cv::Mat img)
{
    if(img.empty()) return false;

    cv::Mat rgbFormatImg=img.clone();

    if(img.type()==CV_8UC1)
        cv::cvtColor(img,rgbFormatImg,CV_GRAY2RGB);
    
    //imshow("Current Frame",img);

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
    
    //waitKey(1);

    return true;
}

void Viewer::drawStatusBarImg(pangolin::GlTexture& imageTexture)
{
    
    std::stringstream ss;
    ss<<"Status Bar Demo.";

    cv::Mat statusImg(
        mnStatusBarHigh,
        mnStatusBarWidth,
        CV_8UC3,
        cv::Scalar(0.2*255,0.2*255,0.2*255));
    
    putText(
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

void Viewer::drawMapViewer(void)
{
    
    // DEPRECATED
    //改变背景颜色
    glClearColor(0.5,0.5,0.5,1.0);
    //绘制世界坐标系
    // pangolin::glDrawAxis(mfAxisSize);

    glColor3f(0.3f,0.3f,0.3f);
    glLineWidth(0.1);
    pangolin::glDraw_y0(1.0, 100);
    glLineWidth(2);
    pangolin::glDrawAxis(mfAxisSize);

    
    //绘制相机位姿.这里只是绘制了一个最基本的处于坐标远点的相机位姿
    Eigen::Isometry3d T=Eigen::Isometry3d::Identity();
    {
        // TODO 这个锁需要修改
        std::lock_guard<std::mutex> lock(mMutexFrameData);
        T=mmCacheFrameTwc;
    }

    
    drawCamera(T);
    /*

    //绘制地图点
    drawMap();
    */
    
}

//REVIEW 初始化的时候使用特征点法，VO使用半直接法？

//TODO 接口这里输入的位姿变换矩阵,最后要变成什么形式Twc还是tcw,还有待商榷
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

void Viewer::drawCamera(Eigen::Isometry3d Twc,double r,double g,double b)
{
    drawCamera(Twc,mfCameraSize,r,g,b);
}

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


bool Viewer::drawRGBView(void)
{
    /*
    static bool bLastFloatMode=false;

    
    //如果显示浮动窗口的设置发生了改变
    if(bLastFloatMode!=mbFloatRGBView)
    {   
        //现在要求显示浮动窗口
        if(mbFloatRGBView)
            {cv::namedWindow(msCurRGBWinName,WINDOW_AUTOSIZE|WINDOW_KEEPRATIO);}
        else    //如果现在是要求关闭窗口
        {
            if(getWindowProperty(msCurRGBWinName,WND_PROP_AUTOSIZE)>0)
                cv::destroyWindow(msCurRGBWinName);
        }

        bLastFloatMode=mbFloatRGBView;
    }


    vector<KeyPoint> vKeyPoints;
    {
        std::lock_guard<std::mutex> lock(mMutexFrameData);
        miCurrentRGB=miCacheCurrentRGB;
        vKeyPoints=mvCacheKeyPoints;
    }

    //绘制特征点
    int nKP=vKeyPoints.size();
    for(int i=0;i<nKP;i++)
    {
        //绘制特征点
        cv::circle(miCurrentRGB,
            cv::Point(vKeyPoints[i].pt.x,vKeyPoints[i].pt.y),
            2,                      //半径
            cv::Scalar(0,255,0),    //绿色
            1,                     //填充
            LINE_8,
            0);
    }

    //如果当前需要显示
    if(mbFloatRGBView)
    {
        if(!miCurrentRGB.empty())
        {
            //cout<<"Viewer : rgb:"<<miCurrentRGB.rows<<" , "<<miCurrentRGB.cols<<endl;
            imshow(msCurRGBWinName,miCurrentRGB);    
        }
        else
        {
            cout<<"Viewer: rgb image is empty!"<<endl;
        }
        
        
        //判断窗口是否被用户关闭了
        if( waitKey(1)==27 || getWindowProperty(msCurRGBWinName,WND_PROP_AUTOSIZE)<0)
        { return false; }
        else
        { return true;  }
        
    }
    else
    {
        //如果不需要显示
        return false;
    }

    */

    // DEBUG
    return true;
}

bool Viewer::drawDepthView(pangolin::GlTexture& imageTexture,bool isColored,bool& isOk)
{
    /*
    static bool bLastFloatMode=false;
    

    isOk=false;
    cv::Mat coloredDepthImg=cv::Mat(miCurrentDepth.rows,miCurrentDepth.cols,CV_8UC3);

    //如果显示浮动窗口的设置发生了改变
    if(bLastFloatMode!=mbFloatDepthView)
    {   
        //现在要求显示浮动窗口
        if(mbFloatDepthView)
            {cv::namedWindow(msCurDepthWinName,WINDOW_AUTOSIZE|WINDOW_KEEPRATIO);}
        else    //如果现在是要求关闭窗口
        {   //这里如果不进行判断的话还是容易出现段错误
            if(getWindowProperty(msCurDepthWinName,WND_PROP_AUTOSIZE)>0)
                cv::destroyWindow(msCurDepthWinName);
        }
        bLastFloatMode=mbFloatDepthView;
    }

    {
        std::lock_guard<std::mutex> lock(mMutexFrameData);
        //if(!miCacheCurrentDepth.empty())
            //NOTICE 其实这里有冗余
        //    miCacheCurrentDepth.convertTo(miCurrentDepth,CV_16UC1,1.0/(mpConfig->getDepthFactor()));
        miCurrentDepth=miCacheCurrentDepth.clone();
    }

    if(!miCurrentDepth.empty())
    {
        size_t rows=miCurrentDepth.rows;
        size_t cols=miCurrentDepth.cols;

        if(isColored)
        {
            unsigned int index;
            //彩色模式
            for(int i=0;i<rows;i++) {
                for(int j=0;j<cols;j++)
                {
                    unsigned int intensity = miCurrentDepth.at<unsigned char>(rows-1-i,j*2) +(((unsigned int)(miCurrentDepth.at<unsigned char>(rows-1-i,j*2+1)))<<8);
                    
                    if(intensity<mnDepthMin)         index=1534;
                    else 
                    {
                        index=(intensity-mnDepthMin)*mfDepthColoredFactor;
                        //如果映射超过了彩色区，说明太远了，直接显示成白色
                        if(index>1534) index=1535;
                    }

                    //这里目前就只移动了7位了～
                    mpImageCache[i*3*cols+j*3  ]=mvHueCircle[index].r;
                    mpImageCache[i*3*cols+j*3+1]=mvHueCircle[index].g;
                    mpImageCache[i*3*cols+j*3+2]=mvHueCircle[index].b;

                    //同时生成伪彩色图
                    coloredDepthImg.at<unsigned char>(rows-1-i,j*3+0)=mvHueCircle[index].b;
                    coloredDepthImg.at<unsigned char>(rows-1-i,j*3+1)=mvHueCircle[index].g;
                    coloredDepthImg.at<unsigned char>(rows-1-i,j*3+2)=mvHueCircle[index].r;
                    
                }
            }
        }
        else
        {
            //把纹理也画一下
            for(int i=0;i<rows;i++) {
                for(int j=0;j<cols;j++)
                {
                    unsigned int intensity = miCurrentDepth.at<unsigned char>(rows-1-i,j*2) +(((unsigned int)(miCurrentDepth.at<unsigned char>(rows-1-i,j*2+1)))<<8);

                    //这里目前就只移动了7位了～
                    mpImageCache[i*3*cols+j*3]=(unsigned char)((intensity>>7));
                    mpImageCache[i*3*cols+j*3+1]=mpImageCache[i*3*cols+j*3];
                    mpImageCache[i*3*cols+j*3+2]=mpImageCache[i*3*cols+j*3];
        
                                

                }
            }
        }
 
        //TODO 其实这里应当返回是否成功的结果，从而控制是否绘图
        imageTexture.Upload(mpImageCache,GL_RGB,GL_UNSIGNED_BYTE);        
        isOk=true;
        
    }

        
    if(mbFloatDepthView)
    {
        if(!miCurrentDepth.empty())
        {
            if(isColored)  imshow(msCurDepthWinName,coloredDepthImg);
            else           imshow(msCurDepthWinName,miCurrentDepth);
        }
        else
        {
            cout<<"Viewer: depth image is empty!"<<endl;
        }
        
        //判断窗口是否被用户关闭了addMapPoint(mapPoint);

        if( waitKey(1)==27 || getWindowProperty(msCurDepthWinName,WND_PROP_AUTOSIZE)<0)
        { return false; }
        else
        { return true;  }
    }
    else
    {
        return false;
    }
    */

    // DEBUG
    return true;

}

/*
void Viewer::updateCurrentFrame(Frame::Ptr pCurrFrame)
{
    
    std::lock_guard<std::mutex> lock(mMutexFrameData);

    if(pCurrFrame->mbIsBad)
        //那么就暂时不做任何的处理
        return ;
    
    //如果帧的图像是正常的话
    miCacheCurrentDepth=pCurrFrame->miDepthImgOrigin.clone();
    miCacheCurrentRGB=pCurrFrame->miRGBImg.clone();
    mvCacheKeyPoints=pCurrFrame->mvKeyPoints;
    mmCacheFrameTwc=pCurrFrame->Twc;

    //这里其实还应该有更多的数据应该缓存
    //例如特征点信息
    
}
*/

/*
void Viewer::updateGlobalMap(Map::Ptr map)
{
    
    //地图直接画，不用上锁
    //这里的线程锁只是对付自己Viewer的
    std::lock_guard<std::mutex> lock(mMutexMapData);
    mvCacheMap=map->getAllMapPoints();

}
*/

void Viewer::drawMap(void)
{
    /*
    glPointSize(mpConfig->getMapPointSize());
    glBegin(GL_POINTS);

    //
    //glColor3f(1.0,1.0,1.0);


    std::lock_guard<std::mutex> lock(mMutexMapData);

    
    for(const auto mapPoint : mvCacheMap)
    {
        Eigen::Vector3d pose=mapPoint->getPose();
        Eigen::Vector3d color=mapPoint->getColor();
        glColor3f(color[0]/255.0,color[1]/255.0,color[2]/255.0);

        //glColor3f(1.0,1.0,1.0);
        //glVertex3f(0.01*(pose[0]-320), 0.01*(pose[1]-240), pose[2]*0.00001);
//        glVertex3f(0.0001*pose[0], 0.0001*pose[1], 0.0001*pose[2]);
        glVertex3f(pose[0], pose[1], pose[2]);

    }

    glEnd();
    */

}
