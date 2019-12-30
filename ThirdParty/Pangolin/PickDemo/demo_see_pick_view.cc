#include <pangolin/pangolin.h>
#include <iostream>
#include <GL/glut.h>
#include <map>
#include <vector>

using namespace std;

// 定义当前使用到的对象
#define OBJECT_TYPE_KEYFRAME    1
#define OBJECT_TYPE_TOPVERTEX   2
#define OBJECT_TYPE_VOXELMPT    3

typedef struct _SelectedObjectInfo
{
    double                      fMinDepth;
    double                      fMaxDepth;
    std::vector<size_t>         vNameIds;
}SelectedObjectInfo;

// 用于关联名字id和实际物体的数据类型
typedef std::map<size_t,std::pair<size_t, void*> > NameIdMapType;
typedef std::vector<SelectedObjectInfo>             VSelectedObjectInfosType;

// render 模式
void DrawKeyFrame(void);
void DrawTopoVertex(void);
void DrawVoxelMpt(void);

// select 模式
void DrawVoxelMptSelectMode(void);

// 鼠标事件的响应函数
void OnMyMouse(pangolin::MouseButton button, int x, int y, bool pressed, int button_state);
void OnMyMouseMotion(int new_x, int new_y, int old_x, int old_y, int button_state);
void OnMyKeyBoard(unsigned char key, int x, int y, bool pressed);

// 如果进入了选择模式，则在此内部执行
size_t SelectObject(pangolin::View& view,pangolin::OpenGlRenderState& RenderState);

// 全局变量，表示是否有要选取的内容 -- 具体应用的时候下面的这些内容就可以是类的成员变量了
bool isSelecting=false;
// 全局变量，记录要选取的物体所在的像素坐标
int select_x,select_y;
// 生成名字所使用的计数器，每次在进入SelectObject函数的时候设置成为0
size_t nameCnt=0;
// 保存名字和实际对象关系的Map
NameIdMapType            nameIdMap;
VSelectedObjectInfosType vSObjInfo;

// 缓存当前帧状态
pangolin::OpenGlMatrix modelview,projection;


int main( int /*argc*/, char** /*argv*/ )
{
    pangolin::CreateWindowAndBind("Main",640,640);
    glEnable(GL_DEPTH_TEST);

    // Define Projection and initial ModelView matrix
    pangolin::OpenGlRenderState s_cam(
        pangolin::ProjectionMatrix(640,640,420,420,320,240,0.2,100),
        pangolin::ModelViewLookAt(-2,2,-2, 0,0,0, pangolin::AxisY)
    );

    // Create Interactive View in window
    pangolin::Handler3D handler(s_cam);
    handler.SetOnMouseFunction(OnMyMouse);
    handler.SetOnMouseMotionFunction(OnMyMouseMotion);
    handler.SetOnKeyBoardFunction(OnMyKeyBoard);
    
    pangolin::View& d_cam = pangolin::CreateDisplay()
            .SetBounds(0.0, 1.0, 0.0, 1.0, -640.0f/640.0f)
            .SetHandler(&handler);


    while( !pangolin::ShouldQuit() )
    {
        // Clear screen and activate view to render into
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        d_cam.Activate(s_cam);

        if(isSelecting)
        {
            glClearColor(0.3f,0.3f,0.3f,1.0f);

            auto selectedObjs = SelectObject(d_cam,s_cam);
            cout<<"[mian] Selected Objects = "<<selectedObjs<<endl;
            pangolin::FinishFrame();

            continue;
        }
        else
        {
            
            // 初始
            glClearColor(0.3f,0.3f,0.3f,1.0f);
            glMatrixMode(GL_MODELVIEW);


            // 绘制关键帧
            DrawKeyFrame();
            DrawTopoVertex();
            DrawVoxelMpt();   
            
        }

        // Swap frames and Process Events
        pangolin::FinishFrame();
    }

    
    return 0;
}


size_t SelectObject(pangolin::View& view,pangolin::OpenGlRenderState& RenderState)
{
    nameCnt=0;
    nameIdMap.clear();
    vSObjInfo.clear();

    cout<<"[SelectObject] Selecting Object ..."<<endl;

    // 创建一个保存选择结果的数组   
    GLuint selectBuff[32]={0};
    GLint hits, viewport[4];      

    // 获得viewport    
    glGetIntegerv(GL_VIEWPORT, viewport); 

    // 初始化 selectBuff
    glSelectBuffer(32, selectBuff); 

    // 进入选择模式
    glRenderMode(GL_SELECT);
    // 初始化名字栈
    glInitNames();
    // 进入投影阶段准备拾取
    glMatrixMode(GL_PROJECTION);

    // 将当前使用的投影矩阵压入堆栈
    glLoadIdentity();

    //设置要进行pick的区域  
    gluPickMatrix(select_x,
                  select_y,
                  // select_x,                 // 窗口中心由之前的鼠标事件给定
                //   viewport[3]-select_y,     // 窗口中心，这里将窗口坐标系转成OpenGL坐标系
                  10,10,                    // 选择框的大小为2，2    
                  viewport);                  // 视口信息，包括视口的起始位置和大小    
    
    // glMatrixMode(GL_PROJECTION);
    RenderState.GetProjectionMatrix().Multiply();

    glMatrixMode(GL_MODELVIEW);
    RenderState.GetModelViewMatrix().Load();

    glPushName(1);
    DrawKeyFrame();
    glLoadName(2);
    DrawTopoVertex();
    glLoadName(3);
    DrawVoxelMpt();
    glPopName();

    // DrawVoxelMptSelectMode();

    // cout<<"[SelectObject] Names:"<<nameCnt<<endl;
    

    hits=0;
    hits = glRenderMode(GL_RENDER);

    glMatrixMode(GL_MODELVIEW);
    // glLoadIdentity();

    DrawKeyFrame();
    DrawTopoVertex();
    DrawVoxelMpt();

    // isSelecting=false;

    return hits;

}



void DrawKeyFrame(void)
{
    const float &w = 0.5;
    const float h = w*0.75;
    const float z = w*0.6;

    for(int i=-2; i<2; i++)
    {
        glPushMatrix();
        glTranslatef(0.0, i*2.0f, 1.0);    

        glLineWidth(1);
        glColor3f(1.0f,0.64f,1.0f);
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
}


void DrawTopoVertex(void)
{
    const double a=2.0f/5.0f;
    const double b=a*sqrt(2);

    for(int i=-2;i<2;++i)
    {
        glPushMatrix();
        // 暂时还是使用这个吧
        glLineWidth(1.0f);
        glTranslatef(i*2.0f, 0.0, 0.0);    

        // 拓扑节点的颜色
        glColor3f(0.0f,1.0f,1.0f);
        glBegin(GL_LINES);
        glVertex3f(a,a,0);
        glVertex3f(a,-a,0);
        glVertex3f(a,-a,0);
        glVertex3f(-a,-a,0);
        glVertex3f(-a,-a,0);
        glVertex3f(-a,a,0);
        glVertex3f(-a,a,0);
        glVertex3f(a,a,0);

        glVertex3f(a,a,0);
        glVertex3f(0,0,b);
        glVertex3f(a,-a,0);
        glVertex3f(0,0,b);
        glVertex3f(-a,-a,0);
        glVertex3f(0,0,b);
        glVertex3f(-a,a,0);
        glVertex3f(0,0,b);

        glVertex3f(a,a,0);
        glVertex3f(0,0,-b);
        glVertex3f(a,-a,0);
        glVertex3f(0,0,-b);
        glVertex3f(-a,-a,0);
        glVertex3f(0,0,-b);
        glVertex3f(-a,a,0);
        glVertex3f(0,0,-b);

        glEnd();
        glPopMatrix();
    }
}

void DrawVoxelMpt(void)
{
    const double base=0.5;
    double a=0.5*base,
           b=0.5*base,
           c=0.5*base;

    for(int i=-2;i<2;++i)
    {
        // 显示绘制框
        glLineWidth (1);
        // 颜色就是白色了
        glColor4f(1.0f,1.0f,1.0f,1.0f);

        glPushMatrix();
        glTranslatef(1.0f, 0.5f, i*2.0f);    
        
        // 绘制一个正方体
        glBegin(GL_LINE_STRIP);
        // 上半个平面
        glVertex3f(a,b,c);
        glVertex3f(a,-b,c);
        glVertex3f(-a,-b,c);
        glVertex3f(-a,b,c);
        glVertex3f(a,b,c);
        // 下半个平面
        glVertex3f(a,b,-c);
        glVertex3f(a,-b,-c);
        glVertex3f(-a,-b,-c);
        glVertex3f(-a,b,-c);
        glVertex3f(a,b,-c);
        // 补充另外几条线
        glEnd();

        glBegin(GL_LINES);
        glVertex3f(a,-b,c);
        glVertex3f(a,-b,-c);

        glVertex3f(-a,-b,c);
        glVertex3f(-a,-b,-c);

        glVertex3f(-a,b,c);
        glVertex3f(-a,b,-c);
        glEnd();   


        // 接下来是绘制平面
        glColor4f(0.6f,0.6f,0.6f,0.9f);
        glBegin(GL_QUAD_STRIP);
        glVertex3f(a,b,c);
        glVertex3f(a,-b,c);

        glVertex3f(-a,b,c);
        glVertex3f(-a,-b,c);

        glVertex3f(-a,b,-c);
        glVertex3f(-a,-b,-c);

        glVertex3f(a,b,-c);
        glVertex3f(a,-b,-c);

        glVertex3f(a,b,c);
        glVertex3f(a,-b,c);
        glEnd();   
        
        glBegin(GL_QUADS);
        glVertex3f(a,b,c);
        glVertex3f(a,b,-c);
        glVertex3f(-a,b,-c);
        glVertex3f(-a,b,c);

        glVertex3f(a,-b,c);
        glVertex3f(a,-b,-c);
        glVertex3f(-a,-b,-c);
        glVertex3f(-a,-b,c);
        glEnd();   

        glPopMatrix();
    }
}

void DrawVoxelMptSelectMode(void)
{
    const double base=0.5;
    double a=0.5*base,
           b=0.5*base,
           c=0.5*base;

    for(int i=-2;i<2;++i)
    {
        // 维护名字,在这个实验中指针类型不需要赋值
        ++nameCnt;
        nameIdMap.insert(std::pair<size_t,std::pair<size_t, void*> >(
                            nameCnt,
                            std::pair<size_t, void*>(
                                OBJECT_TYPE_VOXELMPT,
                                nullptr)));
        glPushName(nameCnt);
        // 显示绘制框
        glLineWidth (1);
        glPushMatrix();
        glTranslatef(1.0f, 0.5f, i*2.0f);    
        // 绘制一个正方体
        glBegin(GL_LINE_STRIP);
        // 上半个平面
        glVertex3f(a,b,c);
        glVertex3f(a,-b,c);
        glVertex3f(-a,-b,c);
        glVertex3f(-a,b,c);
        glVertex3f(a,b,c);
        // 下半个平面
        glVertex3f(a,b,-c);
        glVertex3f(a,-b,-c);
        glVertex3f(-a,-b,-c);
        glVertex3f(-a,b,-c);
        glVertex3f(a,b,-c);
        // 补充另外几条线
        glEnd();

        glBegin(GL_LINES);
        glVertex3f(a,-b,c);
        glVertex3f(a,-b,-c);
        glVertex3f(-a,-b,c);
        glVertex3f(-a,-b,-c);
        glVertex3f(-a,b,c);
        glVertex3f(-a,b,-c);
        glEnd();   

        // 接下来是绘制平面
        glBegin(GL_QUAD_STRIP);
        glVertex3f(a,b,c);
        glVertex3f(a,-b,c);
        glVertex3f(-a,b,c);
        glVertex3f(-a,-b,c);
        glVertex3f(-a,b,-c);
        glVertex3f(-a,-b,-c);
        glVertex3f(a,b,-c);
        glVertex3f(a,-b,-c);
        glVertex3f(a,b,c);
        glVertex3f(a,-b,c);
        glEnd();   
        
        glBegin(GL_QUADS);
        glVertex3f(a,b,c);
        glVertex3f(a,b,-c);
        glVertex3f(-a,b,-c);
        glVertex3f(-a,b,c);
        glVertex3f(a,-b,c);
        glVertex3f(a,-b,-c);
        glVertex3f(-a,-b,-c);
        glVertex3f(-a,-b,c);
        glEnd();   

        glPopMatrix();
        glPopName();  
    }
}

void OnMyMouse(pangolin::MouseButton button, int x, int y, bool pressed, int button_state)
{
    if(pressed && button == pangolin::MouseButtonRight)
    {
        // cout<<"My Mouse on ("<<x<<","<<y<<")"<<endl;
        cout<<"Selecting on ("<<x<<","<<y<<"):"<<endl;
        isSelecting=!isSelecting;
        select_x=x;
        select_y=y;
    }
    else if(pressed && button == pangolin::MouseButtonMiddle)
    {
        // cout<<"Selecting on ("<<x<<","<<y<<"):"<<endl;
        // isSelecting=true;
        // select_x=x;
        // select_y=y;
        
    }
}

void OnMyMouseMotion(int new_x, int new_y, int old_x, int old_y, int button_state)
{
    // if(button_state)
    // {
    //     cout<<"My Mouse moving on ("<<new_x<<","<<new_y<<") and button_state="<<button_state<<endl;

    // }

}

void OnMyKeyBoard(unsigned char key, int x, int y, bool pressed)
{
    // if(pressed)
    // {
    //     cout<<"My Key code "<<key<<" on ("<<x<<","<<y<<")"<<endl;
    // }
}
