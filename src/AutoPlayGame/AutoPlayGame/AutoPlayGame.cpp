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

Storage storage;

class AutoTask : public CTask
{
public:
    void DoTask()
    {
        Sleep(5000);
        while (true)
        {
            CMouse::MoveAbsolute(storage.find<xyls::Point>(CRandom::Int(1, 3)));
            CMouse::RightClick();
            Sleep(10000);
            CKeyboard::KeyPress(VK_SPACE);
            
            int32_t count = 3;
            while (count-- != 0)
            {
                CMouse::MoveAbsolute(xyls::Rect(CSystem::screenCenterPoint(), 30));
                CMouse::RightClick();
                Sleep(1000);
            }
        }
    }
};

int32_t main()
{
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
        storage.add(3, xyls::Point(1280, 730));
    }
    else if (camp == 2 && com == 1)
    {
        storage.add(1, xyls::Point(1256, 609));
        storage.add(2, xyls::Point(1314, 658));
        storage.add(3, xyls::Point(1333, 682));
    }
    else if (camp == 1 && com == 2)
    {
        storage.add(1, xyls::Point(1693, 924));
        storage.add(2, xyls::Point(1720, 958));
        storage.add(3, xyls::Point(1800, 1027));
    }
    else if (camp == 2 && com == 2)
    {
        storage.add(1, xyls::Point(1767, 858));
        storage.add(2, xyls::Point(1848, 926));
        storage.add(3, xyls::Point(1875, 960));
    }

    std::shared_ptr<AutoTask> spLanTask;
    spLanTask.reset(new AutoTask);
    thread->PostTask(spLanTask, 1);
    
    system("pause");
	return 0;
}