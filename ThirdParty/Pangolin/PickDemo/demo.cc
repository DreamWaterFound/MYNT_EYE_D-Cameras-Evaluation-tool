#include <pangolin/pangolin.h>
#include <iostream>
#include <GL/glut.h>
#include <map>
#include <vector>
#include <set>
#include <chrono>
#include <thread>

using namespace std;

// 定义当前使用到的对象
#define OBJECT_TYPE_KEYFRAME    1
#define OBJECT_TYPE_TOPVERTEX   2
#define OBJECT_TYPE_VOXELMPT    3

// 将常用的使用类来实现,这样指针那个字段就能够进行检测了
class KeyFrame
{
public:
    KeyFrame(double x, double y,double z)
        {mpdPosition[0]=x,mpdPosition[1]=y,mpdPosition[2]=z;}
    inline double* GetPosition(void){return mpdPosition;}

private:
    // 它就只有一个成员变量记录它的位置
    double mpdPosition[3];
};

class TopoVertex
{
public:
    TopoVertex(double x, double y,double z)
        {mpdPosition[0]=x,mpdPosition[1]=y,mpdPosition[2]=z;}
    inline double* GetPosition(void){return mpdPosition;}

private:
    // 它就只有一个成员变量记录它的位置
    double mpdPosition[3];
};

class VoxelMpt
{
public:
    VoxelMpt(double x, double y,double z)
        {mpdPosition[0]=x,mpdPosition[1]=y,mpdPosition[2]=z;}
    inline double* GetPosition(void){return mpdPosition;}

private:
    // 它就只有一个成员变量记录它的位置
    double mpdPosition[3];
};

typedef struct _SelectedObjectInfo
{
    double                      fMinDepth;
    double                      fMaxDepth;
    std::vector<size_t>         vNameIds;
}SelectedObjectInfo;

// 用于关联名字id和实际物体的数据类型
typedef std::map<size_t,std::pair<size_t, void*> > NameIdMapType;
typedef std::vector<SelectedObjectInfo>             VSelectedObjectInfosType;

// 产生绘图所需要的数据
void GenerateAllObjects(void);

// render 模式
void DrawKeyFrame(void);
void DrawTopoVertex(void);
void DrawVoxelMpt(void);

void DrawKeyFrameSelected(void);
void DrawTopoVertexSelected(void);
void DrawVoxelMptSelected(void);

// select 模式
void DrawKeyFrameSelectMode(void);
void DrawTopoVertexSelectMode(void);
void DrawVoxelMptSelectMode(void);

// 鼠标事件的响应函数
void OnMyMouse(pangolin::MouseButton button, int x, int y, bool pressed, int button_state);
void OnMyMouseMotion(int new_x, int new_y, int old_x, int old_y, int button_state);
void OnMyKeyBoard(unsigned char key, int x, int y, bool pressed);

// 如果进入了选择模式，则在此内部执行
size_t SelectObject(pangolin::View& view,pangolin::OpenGlRenderState& RenderState);

// 当前程序生成和要使用的指针都放在这里
std::set<KeyFrame*> setpKeyFrames;
std::set<TopoVertex*> setpTopoVerteics;
std::set<VoxelMpt*> setpVoxelMpts;

std::set<KeyFrame*> setpSelectedKeyFrames;
std::set<TopoVertex*> setpSelectedTopoVerteics;
std::set<VoxelMpt*> setpSelectedVoxelMpts;



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
// pangolin::OpenGlMatrix modelview,projection;


int main( int /*argc*/, char** /*argv*/ )
{
    pangolin::CreateWindowAndBind("Main",640,480);
    glEnable(GL_DEPTH_TEST);

    pangolin::CreatePanel("menu").SetBounds(0.0,1.0,0.0,pangolin::Attach::Pix(175));
    pangolin::Var<bool> menuFollowCamera("menu.Follow Camera",true,true);
    pangolin::Var<bool> menuShowPoints("menu.Show Points",true,true);

    // Define Projection and initial ModelView matrix
    pangolin::OpenGlRenderState s_cam(
        pangolin::ProjectionMatrix(640,480,420,420,320,240,0.2,100),
        pangolin::ModelViewLookAt(-2,2,-2, 0,0,0, pangolin::AxisY)
    );

    // Create Interactive View in window
    pangolin::Handler3D handler(s_cam);
    handler.SetOnMouseFunction(OnMyMouse);
    handler.SetOnMouseMotionFunction(OnMyMouseMotion);
    handler.SetOnKeyBoardFunction(OnMyKeyBoard);
    
    pangolin::View& d_cam = pangolin::CreateDisplay()
            .SetBounds(0.0, 1.0, 0.0, 1.0, -640.0f/480.0f)
            .SetHandler(&handler);

    // 产生要绘制的物体对象
    GenerateAllObjects();


    while( !pangolin::ShouldQuit() )
    {
        // Clear screen and activate view to render into
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        d_cam.Activate(s_cam);

        if(isSelecting)
        {
            glClearColor(0.3f,0.3f,0.3f,1.0f);

            SelectObject(d_cam,s_cam);
            // cout<<"[mian] Selected Objects = "<<selectedObjs<<endl;
            // pangolin::FinishFrame();

            // continue;
        }
        // else
        {
            d_cam.Activate(s_cam);

            
            // 初始
            glClearColor(0.3f,0.3f,0.3f,1.0f);
            glMatrixMode(GL_MODELVIEW);


            // 绘制已经被选中的对象,这个一定要先绘制
            DrawKeyFrameSelected();
            DrawTopoVertexSelected();
            DrawVoxelMptSelected();   

            // 绘制关键帧
            DrawKeyFrame();
            DrawTopoVertex();
            DrawVoxelMpt();      
        }


        // Swap frames and Process Events
        pangolin::FinishFrame();

        std::this_thread::sleep_for(std::chrono::milliseconds(20));

    }

    
    return 0;
}


size_t SelectObject(pangolin::View& view,pangolin::OpenGlRenderState& RenderState)
{
    nameCnt=0;
    nameIdMap.clear();
    vSObjInfo.clear();

    setpSelectedKeyFrames.clear();
    setpSelectedTopoVerteics.clear();
    setpSelectedVoxelMpts.clear();
    

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
                  10,10,                    // 选择框的大小为2，2    
                  viewport);                  // 视口信息，包括视口的起始位置和大小    
    
    RenderState.GetProjectionMatrix().Multiply();

    glMatrixMode(GL_MODELVIEW);
    RenderState.GetModelViewMatrix().Load();
    pangolin::OpenGlMatrix MatrixT;
    if(RenderState.GetFollowState(MatrixT))
    {
        // 如果是在跟踪状态也要使用这个矩阵
        MatrixT.Multiply();
    }


    // 绘制
    DrawKeyFrameSelectMode();
    DrawVoxelMptSelectMode();   
    DrawTopoVertexSelectMode();


    hits=0;
    hits = glRenderMode(GL_RENDER);

    // 开始分析
    cout<<"[SelectObject] Total Names:"<<nameCnt<<endl;
    cout<<"[SelectObject] Hited:"<<hits<<endl;
    // 读指针
    int pointer=0;
    for(int i=0;i<hits;++i)
    {
        int numNames=selectBuff[pointer++];
        cout<<"[SelectObject] \tHited Object #"<<i+1<<": \r\n\t\t";
        cout<<"have names: "<<numNames<<"\r\n\t\t";
        cout<<"min depth: "<<((double)(selectBuff[pointer++]))/0xffffffff<<"\r\n\t\t";
        cout<<"max depth: "<<((double)(selectBuff[pointer++]))/0xffffffff<<"\r\n\t\t";

        for(int j=0;j<numNames;++j)
        {
            auto name=selectBuff[pointer++];
            size_t type=nameIdMap[name].first;
            cout<<"name["<<j+1<<"]: "<<name<<"\ttyepe:";
            switch (type)
            {
                case OBJECT_TYPE_KEYFRAME:
                    cout<<"KeyFrame"<<endl;
                    setpSelectedKeyFrames.insert((KeyFrame*)nameIdMap[name].second);
                    break;
                case OBJECT_TYPE_TOPVERTEX:
                    cout<<"TopoVertex"<<endl;
                    setpSelectedTopoVerteics.insert((TopoVertex*)nameIdMap[name].second);
                    break;
                case OBJECT_TYPE_VOXELMPT:
                    cout<<"VoxelMpt"<<endl;
                    setpSelectedVoxelMpts.insert((VoxelMpt*)nameIdMap[name].second);
                    break;
                default:
                    cout<<"Unkonwn"<<endl;
            }
        }
        cout<<endl;
    }

    glMatrixMode(GL_MODELVIEW);

    isSelecting=false;

    return hits;

}



void DrawKeyFrame(void)
{
    const float &w = 0.5;
    const float h = w*0.75;
    const float z = w*0.6;

    // size_t len=setpKeyFrames.size();
    for(auto pKF: setpKeyFrames)
    {
        glPushMatrix();
        glTranslatef(pKF->GetPosition()[0],
                     pKF->GetPosition()[1],
                     pKF->GetPosition()[3]);

        glLineWidth(1);
        glColor4f(1.0f,0.64f,1.0f,1.0f);
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

/*
        // 平面的绘制尝试
        glColor4f(1.0f*0.5,0.64f*0.5,1.0f*0.5,0.1f);
        glBegin(GL_TRIANGLE_STRIP);
        glVertex3f(-w, h, z);
        glVertex3f(-w,-h, z);
        glVertex3f( 0, 0, 0);
        glVertex3f( w,-h, z);
        glEnd();
        glBegin(GL_TRIANGLE_STRIP);
        glVertex3f( w,-h, z);
        glVertex3f( w, h, z);
        glVertex3f( 0, 0, 0);
        glVertex3f(-w, h, z);
        glEnd();
        // glColor4f(1.0f,0.64f,1.0f,1.0f);
        glBegin(GL_QUADS);
        glVertex3f( w, h, z);
        glVertex3f( w,-h, z);
        glVertex3f(-w,-h, z);
        glVertex3f(-w, h, z);
        glEnd();
*/
        glPopMatrix();
    }
}


void DrawTopoVertex(void)
{
    const double a=2.0f/5.0f;
    const double b=a*sqrt(2);

    for(auto pTV: setpTopoVerteics)
    {
        glPushMatrix();
        // 暂时还是使用这个吧
        glLineWidth(1.0f);
        glTranslatef(pTV->GetPosition()[0],
                     pTV->GetPosition()[1],
                     pTV->GetPosition()[3]);

        // 拓扑节点的颜色
        glColor3f(0.0f,1.0f,1.0f);
        // TODO
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
/*
        // 绘制平面,四组对三角
        glColor3f(0.0f,1.0f*0.5,1.0f*0.5);
        glBegin(GL_TRIANGLE_STRIP);
        glVertex3f(-a, a, 0);
        glVertex3f(-a,-a, 0);
        glVertex3f( 0, 0, b);
        glVertex3f( a, -a, 0);
        glEnd();

        glBegin(GL_TRIANGLE_STRIP);
        glVertex3f( a,-a, 0);
        glVertex3f( a, a, 0);
        glVertex3f( 0, 0, b);
        glVertex3f(-a, a, 0);
        glEnd();

        glBegin(GL_TRIANGLE_STRIP);
        glVertex3f(-a, a, 0);
        glVertex3f(-a,-a, 0);
        glVertex3f( 0, 0,-b);
        glVertex3f( a, -a, 0);
        glEnd();

        glBegin(GL_TRIANGLE_STRIP);
        glVertex3f( a,-a, 0);
        glVertex3f( a, a, 0);
        glVertex3f( 0, 0,-b);
        glVertex3f(-a, a, 0);
        glEnd();
*/
        glPopMatrix();
    }
}

void DrawVoxelMpt(void)
{
    const double base=0.5;
    double a=0.5*base,
           b=0.5*base,
           c=0.5*base;

    for(auto pVMpt: setpVoxelMpts)
    {
        // 显示绘制框
        glLineWidth (1);
        // 颜色就是白色了
        glColor4f(1.0f,1.0f,1.0f,1.0f);

        glPushMatrix();
        glTranslatef(pVMpt->GetPosition()[0],
                     pVMpt->GetPosition()[1],
                     pVMpt->GetPosition()[2]);

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

    for(auto pVMpt: setpVoxelMpts)
    {
        // 维护名字,在这个实验中指针类型不需要赋值
        ++nameCnt;
        nameIdMap.insert(std::pair<size_t,std::pair<size_t, void*> >(
                            nameCnt,
                            std::pair<size_t, void*>(
                                OBJECT_TYPE_VOXELMPT,
                                pVMpt)));
        glPushName(nameCnt);
        
        // 显示绘制框
        glLineWidth (1);
        glPushMatrix();
        glTranslatef(pVMpt->GetPosition()[0],
                     pVMpt->GetPosition()[1],
                     pVMpt->GetPosition()[2]);

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
        
    }
    else if(pressed && button == pangolin::MouseButtonMiddle)
    {
        
    }
    else if(pressed && button == pangolin::MouseButtonLeft)
    {
        if(x>175)
        {
            cout<<"Selecting on ("<<x<<","<<y<<"):"<<endl;
            isSelecting=!isSelecting;
            select_x=x;
            select_y=y;
        }
        
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


void GenerateAllObjects(void)
{
    for(int i=-2;i<2;++i)
    {
        setpKeyFrames.insert(new KeyFrame(0.0,i*2.0,1.0));
        setpTopoVerteics.insert(new TopoVertex(i*2.0f,0.0,0.0));
        setpVoxelMpts.insert(new VoxelMpt(1.0,0.5,i*2.0));
    }
}


void DrawKeyFrameSelected(void)
{
    const float &w = 0.5;
    const float h = w*0.75;
    const float z = w*0.6;

    // size_t len=setpKeyFrames.size();
    for(auto pKF: setpSelectedKeyFrames)
    {
        glPushMatrix();
        glTranslatef(pKF->GetPosition()[0],
                     pKF->GetPosition()[1],
                     pKF->GetPosition()[3]);

        glLineWidth(5);
        glColor3f(1.0f,1.0f,0.0f);
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

        // 平面的绘制尝试
        glColor4f(1.0f*0.5,0.64f*0.5,1.0f*0.5,0.1f);
        glBegin(GL_TRIANGLE_STRIP);
        glVertex3f(-w, h, z);
        glVertex3f(-w,-h, z);
        glVertex3f( 0, 0, 0);
        glVertex3f( w,-h, z);
        glEnd();
        glBegin(GL_TRIANGLE_STRIP);
        glVertex3f( w,-h, z);
        glVertex3f( w, h, z);
        glVertex3f( 0, 0, 0);
        glVertex3f(-w, h, z);
        glEnd();
        // glColor4f(1.0f,0.64f,1.0f,1.0f);
        glBegin(GL_QUADS);
        glVertex3f( w, h, z);
        glVertex3f( w,-h, z);
        glVertex3f(-w,-h, z);
        glVertex3f(-w, h, z);
        glEnd();

        glPopMatrix();
    }
}

void DrawTopoVertexSelected(void)
{
    const double a=2.0f/5.0f;
    const double b=a*sqrt(2);

    for(auto pTV: setpSelectedTopoVerteics)
    {
        glPushMatrix();
        // 暂时还是使用这个吧
        glLineWidth(5.0f);
        glTranslatef(pTV->GetPosition()[0],
                     pTV->GetPosition()[1],
                     pTV->GetPosition()[3]);

        // 拓扑节点的颜色
        glColor3f(1.0f,1.0f,0.0f);
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

        glColor3f(0.0f,1.0f*0.5,1.0f*0.5);
        glBegin(GL_TRIANGLE_STRIP);
        glVertex3f(-a, a, 0);
        glVertex3f(-a,-a, 0);
        glVertex3f( 0, 0, b);
        glVertex3f( a, -a, 0);
        glEnd();

        glBegin(GL_TRIANGLE_STRIP);
        glVertex3f( a,-a, 0);
        glVertex3f( a, a, 0);
        glVertex3f( 0, 0, b);
        glVertex3f(-a, a, 0);
        glEnd();

        glBegin(GL_TRIANGLE_STRIP);
        glVertex3f(-a, a, 0);
        glVertex3f(-a,-a, 0);
        glVertex3f( 0, 0,-b);
        glVertex3f( a, -a, 0);
        glEnd();

        glBegin(GL_TRIANGLE_STRIP);
        glVertex3f( a,-a, 0);
        glVertex3f( a, a, 0);
        glVertex3f( 0, 0,-b);
        glVertex3f(-a, a, 0);
        glEnd();
        glPopMatrix();
    }
}

void DrawVoxelMptSelected(void)
{
    const double base=0.5;
    double a=0.5*base,
           b=0.5*base,
           c=0.5*base;

    for(auto pVMpt: setpSelectedVoxelMpts)
    {
        // 显示绘制框
        glLineWidth (5);
        // 颜色就是白色了
        glColor4f(1.0f,1.0f,1.0f,0.0f);

        glPushMatrix();
        glTranslatef(pVMpt->GetPosition()[0],
                     pVMpt->GetPosition()[1],
                     pVMpt->GetPosition()[2]);

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
        glColor4f(0.6f,0.6f,0.0f,0.9f);
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

void DrawKeyFrameSelectMode(void)
{
    const float &w = 0.5;
    const float h = w*0.75;
    const float z = w*0.6;

    // size_t len=setpKeyFrames.size();
    for(auto pKF: setpKeyFrames)
    {
        // 维护名字
        ++nameCnt;
        nameIdMap.insert(std::pair<size_t,std::pair<size_t, void*> >(
                            nameCnt,
                            std::pair<size_t, void*>(
                                OBJECT_TYPE_KEYFRAME,
                                pKF)));
        glPushName(nameCnt);
        glPushMatrix();
        glTranslatef(pKF->GetPosition()[0],
                     pKF->GetPosition()[1],
                     pKF->GetPosition()[3]);

        glLineWidth(1);

        // 平面的绘制尝试
        glColor4f(1.0f*0.5,0.64f*0.5,1.0f*0.5,0.1f);
        glBegin(GL_TRIANGLE_STRIP);
        glVertex3f(-w, h, z);
        glVertex3f(-w,-h, z);
        glVertex3f( 0, 0, 0);
        glVertex3f( w,-h, z);
        glEnd();
        glBegin(GL_TRIANGLE_STRIP);
        glVertex3f( w,-h, z);
        glVertex3f( w, h, z);
        glVertex3f( 0, 0, 0);
        glVertex3f(-w, h, z);
        glEnd();
        // glColor4f(1.0f,0.64f,1.0f,1.0f);
        glBegin(GL_QUADS);
        glVertex3f( w, h, z);
        glVertex3f( w,-h, z);
        glVertex3f(-w,-h, z);
        glVertex3f(-w, h, z);
        glEnd();
        /*
        // glColor3f(1.0f,0.64f,1.0f);
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
    */
        glPopMatrix();
        glPopName();  

    }
}

void DrawTopoVertexSelectMode(void)
{
    const double a=2.0f/5.0f;
    const double b=a*sqrt(2);

    for(auto pTV: setpTopoVerteics)
    {
        // 维护名字
        ++nameCnt;
        nameIdMap.insert(std::pair<size_t,std::pair<size_t, void*> >(
                            nameCnt,
                            std::pair<size_t, void*>(
                                OBJECT_TYPE_TOPVERTEX,
                                pTV)));

        glPushName(nameCnt);
        glPushMatrix();
        // 暂时还是使用这个吧
        glLineWidth(1.0f);
        glTranslatef(pTV->GetPosition()[0],
                     pTV->GetPosition()[1],
                     pTV->GetPosition()[3]);

        // // 拓扑节点的颜色
        // glColor3f(0.0f,1.0f,1.0f);
        // // TODO
        // glBegin(GL_LINES);
        // glVertex3f(a,a,0);
        // glVertex3f(a,-a,0);
        // glVertex3f(a,-a,0);
        // glVertex3f(-a,-a,0);
        // glVertex3f(-a,-a,0);
        // glVertex3f(-a,a,0);
        // glVertex3f(-a,a,0);
        // glVertex3f(a,a,0);

        // glVertex3f(a,a,0);
        // glVertex3f(0,0,b);
        // glVertex3f(a,-a,0);
        // glVertex3f(0,0,b);
        // glVertex3f(-a,-a,0);
        // glVertex3f(0,0,b);
        // glVertex3f(-a,a,0);
        // glVertex3f(0,0,b);

        // glVertex3f(a,a,0);
        // glVertex3f(0,0,-b);
        // glVertex3f(a,-a,0);
        // glVertex3f(0,0,-b);
        // glVertex3f(-a,-a,0);
        // glVertex3f(0,0,-b);
        // glVertex3f(-a,a,0);
        // glVertex3f(0,0,-b);

        // glEnd();

        // 绘制平面,四组对三角
        // glColor3f(0.0f,1.0f*0.5,1.0f*0.5);
        glBegin(GL_TRIANGLE_STRIP);
        glVertex3f(-a, a, 0);
        glVertex3f(-a,-a, 0);
        glVertex3f( 0, 0, b);
        glVertex3f( a, -a, 0);
        glEnd();

        glBegin(GL_TRIANGLE_STRIP);
        glVertex3f( a,-a, 0);
        glVertex3f( a, a, 0);
        glVertex3f( 0, 0, b);
        glVertex3f(-a, a, 0);
        glEnd();

        glBegin(GL_TRIANGLE_STRIP);
        glVertex3f(-a, a, 0);
        glVertex3f(-a,-a, 0);
        glVertex3f( 0, 0,-b);
        glVertex3f( a, -a, 0);
        glEnd();

        glBegin(GL_TRIANGLE_STRIP);
        glVertex3f( a,-a, 0);
        glVertex3f( a, a, 0);
        glVertex3f( 0, 0,-b);
        glVertex3f(-a, a, 0);
        glEnd();

        glPopMatrix();

        glPopName();
    }

}
