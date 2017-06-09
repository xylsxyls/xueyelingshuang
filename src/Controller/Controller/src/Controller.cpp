#include "Controller.h"
#include <thread>

vector<Obstacles*> Controller::vecOutObstacles;

Controller::~Controller()
{
    for (auto itmapObstacles = mapObstacles.begin(); itmapObstacles != mapObstacles.end(); ++itmapObstacles)
    {
        vecOutObstacles.push_back(itmapObstacles->second);
    }
    mapObstacles.clear();
}

void Controller::ReleaseOutObstacles()
{
    int i = -1;
    while (i++ != vecOutObstacles.size() - 1)
    {
        delete vecOutObstacles.at(i);
    }
    vecOutObstacles.clear();
}

Obstacles* Controller::CreateObstacles(int num)
{
    return CreateObstacles(num, "", true);
}

Obstacles* Controller::CreateObstacles(int num, string strControl)
{
    return CreateObstacles(num, strControl, false);
}

Obstacles* Controller::CreateObstacles(int num, bool ifChoke)
{
    return CreateObstacles(num, "", ifChoke);
}

Obstacles* Controller::CreateObstacles(int num, string strControl, bool ifChoke)
{
    //如果当前编号有障碍器则返回空
    if (mapObstacles.find(num) != mapObstacles.end())
    {
        return nullptr;
    }
    Obstacles* obstacles = Obstacles::CreateApi(strControl, ifChoke);
    if (obstacles != nullptr)
    {
        mapObstacles[num] = obstacles;
        return obstacles;
    }
    return nullptr;
}

void Controller::ReleaseObstacles()
{
    for (auto itmapObstacles = mapObstacles.begin(); itmapObstacles != mapObstacles.end(); ++itmapObstacles)
    {
        delete itmapObstacles->second;
    }
    mapObstacles.clear();
}

void Controller::SetChoke(int num){
    mapObstacles[num]->current = true;
}

void Controller::SetUnChoke(int num){
    mapObstacles[num]->current = false;
}

void Controller::ChokeSetChoke(int num)
{
    mapObstacles[num]->current = true;
    while (mapObstacles[num]->status != Obstacles::insideChoke)
    {
        std::chrono::milliseconds dura(1);
        std::this_thread::sleep_for(dura);
    }
}

void Controller::ChokeSetUnChoke(int num)
{
    mapObstacles[num]->current = false;
    while (mapObstacles[num]->status != Obstacles::insideUnChoke)
    {
        std::chrono::milliseconds dura(1);
        std::this_thread::sleep_for(dura);
    }
}

void Controller::ChokeToControl(int num)
{
    while (mapObstacles[num]->status == Obstacles::outside)
    {
        std::chrono::milliseconds dura(1);
        std::this_thread::sleep_for(dura);
    }
}

void Controller::SetTempControl(int num, string nextControl){
    mapObstacles[num]->mutex.lock();
    mapObstacles[num]->strNextControl = nextControl;
    mapObstacles[num]->mutex.unlock();
}