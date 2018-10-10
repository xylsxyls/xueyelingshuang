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
#include "CStopWatch/CStopWatchAPI.h"
#include "ScreenScript/ScreenScriptAPI.h"
#include "CStringManager/CStringManagerAPI.h"

Storage storage;
int32_t beginPlay;
int32_t computer = 1;
int32_t camp = 1;

class SkillTask : public CountDownTask
{
public:
    void DoTask()
    {
        CKeyboard::KeyDown(CKeyboard::Ctrl);
        CKeyboard::KeyDown('R');
        CKeyboard::KeyUp(CKeyboard::Ctrl);
        CKeyboard::KeyUp('R');
        //CKeyboard::KeyDown(CKeyboard::Ctrl);
        //CKeyboard::KeyDown('W');
        //CKeyboard::KeyUp(CKeyboard::Ctrl);
        //CKeyboard::KeyUp('W');
        CKeyboard::KeyDown(CKeyboard::Ctrl);
        CKeyboard::KeyDown('E');
        CKeyboard::KeyUp(CKeyboard::Ctrl);
        CKeyboard::KeyUp('E');
        CKeyboard::KeyDown(CKeyboard::Ctrl);
        CKeyboard::KeyDown('Q');
        CKeyboard::KeyUp(CKeyboard::Ctrl);
        CKeyboard::KeyUp('Q');
        

        int32_t threadId = 0;
        CStopWatch::CountDown(3 * 60 * 1000, new SkillTask, 1, threadId);
    }
};

class AutoTask : public CTask
{
public:
    void DoTask()
    {
        Sleep(5000);
        while (true)
        {
            if (beginPlay == 1)
            {
                beginPlay = 0;
                goto GAME;
            }
            else if (beginPlay == 2)
            {
                beginPlay = 0;
                goto CHOOSE;
            }
        GAME:
            int32_t threadId = 0;
            CStopWatch::CountDown(10000, new SkillTask, 1, threadId);
            while (true)
            {
                xyls::Point point = storage.find<xyls::Point>(CRandom::Int(camp * camp, camp * 3));
                CMouse::MoveAbsolute(xyls::Rect(point, 3));
                CMouse::RightClick();
                Sleep(30000);
                CMouse::MoveAbsolute(point);
                CMouse::LeftClick();

                CKeyboard::InputString("eeqq");
                int32_t count = 20;
                while (count-- != 0)
                {
                    CMouse::MoveAbsolute(xyls::Rect(CSystem::screenCenterPoint(), 100), 100);
                    CMouse::RightClick(200);
                }

                ScreenScript::FindClick("shi.png");
                ScreenScript::FindClick("OtherOk.png");
                ScreenScript::FindClick("EndShop.png");

                if (ScreenScript::FindClick("EndGame.png"))
                {
                    break;
                }

                if (ScreenScript::FindClick("BackHome.png"))
                {
                    break;
                }

                if (ScreenScript::FindClick("PlayAgain.png"))
                {
                    break;
                }
            }
        CHOOSE:
            while (true)
            {
                ScreenScript::FindClick("Play.png");
                ScreenScript::FindClick("Rank.png");
                ScreenScript::FindClick("Confirm.png");
				ScreenScript::FindClick("PlayAgain.png");
				ScreenScript::FindClick("OtherOk.png");
				ScreenScript::FindClick("OtherOk2.png");
                ScreenScript::FindClick("EndFriend.png");
                ScreenScript::FindClick("ErrorOk.png");
                ScreenScript::FindClick("Choose.png");
                ScreenScript::FindClick("zhonglu.png");
                ScreenScript::FindClick("daye.png");
                ScreenScript::FindClick("Begin.png");
				ScreenScript::FindClick("Agree.png");
                ScreenScript::FindClick("NoBody.png");
                ScreenScript::FindClick("Forbidden.png");
                if (ScreenScript::FindClick("First.png"))
                {
                    Sleep(1000);
                }
                if (ScreenScript::FindClick("Second.png"))
                {
                    Sleep(1000);
                }
                if (ScreenScript::FindClick("Third.png"))
                {
                    Sleep(1000);
                }
                ScreenScript::FindClick("OK.png");
                if (ScreenScript::FindPic(CStringManager::Format("lan%d.png", computer)))
                {
                    camp = 1;
                    break;
                }
                else if (ScreenScript::FindPic(CStringManager::Format("zi%d.png", computer)))
                {
                    camp = 2;
                    break;
                }
            }
        }
    }
};

int32_t main()
{
    CDump::instance().DeclareDumpFile();
    xyls::Rect rect(CSystem::GetWindowResolution());
    if (rect == xyls::Rect(0, 0, 1366, 768))
    {
        computer = 1;
    }
    else if(rect == xyls::Rect(0, 0, 1920, 1080))
    {
        computer = 2;
    }

    printf("从哪里开始，游戏开始1，选择开始2\n");
    int32_t begin = 1;
    std::cin >> begin;
    beginPlay = begin;

    if (beginPlay == 1)
    {
        printf("请输入阵营，蓝方1，紫方2\n");
        std::cin >> camp;
    }

    auto threadId = CTaskThreadManager::Instance().Init();
    auto thread = CTaskThreadManager::Instance().GetThreadInterface(threadId);
    
    if (computer == 1)
    {
        storage.add(1, xyls::Point(1204, 657));
        storage.add(2, xyls::Point(1223, 681));
        storage.add(3, xyls::Point(1277, 728));
        storage.add(4, xyls::Point(1256, 609));
        storage.add(5, xyls::Point(1313, 655));
        storage.add(6, xyls::Point(1333, 682));
    }
    else if (computer == 2)
    {
        storage.add(1, xyls::Point(1689, 924));
        storage.add(2, xyls::Point(1720, 958));
        storage.add(3, xyls::Point(1798, 1023));
        storage.add(4, xyls::Point(1767, 856));
        storage.add(5, xyls::Point(1845, 923));
        storage.add(6, xyls::Point(1872, 956));
    }

    std::shared_ptr<AutoTask> spLanTask;
    spLanTask.reset(new AutoTask);
    thread->PostTask(spLanTask, 1);
    
    system("pause");
	return 0;
}