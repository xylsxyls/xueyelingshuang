#include "AutoPlayGame.h"
#include <stdint.h>
#include <stdio.h>
#include "CRandom/CRandomAPI.h"
#include "Storage/StorageAPI.h"
#include "Point/PointAPI.h"
#include "CTaskThreadManager/CTaskThreadManagerAPI.h"
#include <iostream>
#include "CMouse/CMouseAPI.h"
#include "CKeyboard/CKeyboardAPI.h"
#include "CSystem/CSystemAPI.h"
#include "CDump/CDumpAPI.h"

Storage storage;

class AutoTask : public CTask
{
public:
    void DoTask()
    {
        Sleep(5000);
        while (true)
        {
            xyls::Point point = storage.find<xyls::Point>(CRandom::Int(1, 3));
            CMouse::MoveAbsolute(xyls::Rect(point, 3));
            CMouse::RightClick();
            Sleep(30000);
            CMouse::MoveAbsolute(point);
            CMouse::LeftClick();
            
            int32_t count = 20;
            while (count-- != 0)
            {
                CMouse::MoveAbsolute(xyls::Rect(CSystem::screenCenterPoint(), 100), 100);
                CMouse::RightClick(200);
                CKeyboard::InputString("qqww");
            }
        }
    }
};

int32_t main()
{
    CDump::instance().DeclareDumpFile();
    int32_t com = 1;
    xyls::Rect rect(CSystem::GetWindowResolution());
    if (rect == xyls::Rect(0, 0, 1366, 768))
    {
        com = 1;
    }
    else if(rect == xyls::Rect(0, 0, 1920, 1080))
    {
        com = 2;
    }

    printf("请输入阵营，蓝方1紫方2：\n");
    int32_t camp = 1;
    std::cin >> camp;

    auto threadId = CTaskThreadManager::Instance().Init();
    auto thread = CTaskThreadManager::Instance().GetThreadInterface(threadId);
    
    if (camp == 1 && com == 1)
    {
        storage.add(1, xyls::Point(1204, 657));
        storage.add(2, xyls::Point(1223, 681));
        storage.add(3, xyls::Point(1277, 728));
    }
    else if (camp == 2 && com == 1)
    {
        storage.add(1, xyls::Point(1256, 609));
        storage.add(2, xyls::Point(1314, 658));
        storage.add(3, xyls::Point(1333, 682));
    }
    else if (camp == 1 && com == 2)
    {
        storage.add(1, xyls::Point(1689, 924));
        storage.add(2, xyls::Point(1720, 958));
        storage.add(3, xyls::Point(1798, 1023));
    }
    else if (camp == 2 && com == 2)
    {
        storage.add(1, xyls::Point(1767, 856));
        storage.add(2, xyls::Point(1848, 921));
        storage.add(3, xyls::Point(1872, 956));
    }

    std::shared_ptr<AutoTask> spLanTask;
    spLanTask.reset(new AutoTask);
    thread->PostTask(spLanTask, 1);
    

    system("pause");
	return 0;
}