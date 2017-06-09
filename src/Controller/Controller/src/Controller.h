#pragma once
#include "Obstacles.h"
#include "ControllerMacro.h"
#include <map>
#include <vector>
using namespace std;

//遥控器，控制障碍器状态
class ControllerAPI Controller{
    friend class Obstacles;
public:
    //遥控器一旦损毁所有它产生的障碍器全部失效，但是不会释放障碍器内存，障碍器转为野外状态
    ~Controller();
    //将野外状态障碍器内存全部释放
    static void ReleaseOutObstacles();
public:
    //生成，第一个参数是障碍器编号，如果传入了已有的障碍器编号则返回空，默认阻塞
    Obstacles* CreateObstacles(int num);
    //生成，前面按照strControl执行，后面不阻塞
    Obstacles* CreateObstacles(int num, string strControl);
    //生成，根据ifChoke判断一直阻塞或者一直不阻塞
    Obstacles* CreateObstacles(int num, bool ifChoke);
    //生成
    Obstacles* CreateObstacles(int num, string strControl, bool ifChoke);
    //释放，将此遥控器所产生的障碍器释放
    void ReleaseObstacles();
    //设置障碍器为阻塞，此函数不阻塞
    void SetChoke(int num);
    //设置障碍器为非阻塞，此函数不阻塞
    void SetUnChoke(int num);
    //设置障碍器为阻塞，此函数阻塞，当再次运行到障碍器所设位置时或当前正在阻塞时，函数返回
    void ChokeSetChoke(int num);
    //设置障碍器为非阻塞，此函数阻塞，当再次运行到障碍器所设位置时函数返回
    void ChokeSetUnChoke(int num);
    //函数阻塞到再次执行到障碍器，不改变障碍器状态
    void ChokeToControl(int num);
    //接下来的几个
    void SetTempControl(int num, string nextControl);

private:
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4251)
#endif
    map<int, Obstacles*> mapObstacles;
    static vector<Obstacles*> vecOutObstacles;
#ifdef _MSC_VER
#pragma warning(pop)
#endif

};