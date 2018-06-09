#include "stdafx.h"
#include "CqNoFlashTask.h"
#include "CKeyboard/CKeyboardAPI.h"
#include "CMouse/CMouseAPI.h"

extern std::atomic<bool> rightMouse;

void CqNoFlashTask::DoTask()
{
    rightMouse = false;

    /*CKeyboard::KeyDown('N');
    CKeyboard::KeyDown('Q');
    CKeyboard::KeyUp('N');
    CKeyboard::KeyUp('Q');
    Sleep(250);*/
    //Sleep(100);

    CKeyboard::InputString("wwwww", 0);
    Sleep(450);

    CKeyboard::InputString("r", 0);

    CMouse::RightManyClick(1, 0);
    CMouse::RightManyClick(8, 300);
    //CMouse::RightManyClick(2, 100);

    //Sleep(50);
    CKeyboard::InputString("3", 50);

    /*Sleep(40);
    CMouse::RightManyClick(6, 200);
    CMouse::RightManyClick(2, 100);

    CKeyboard::InputString("3", 0);*/

    //CKeyboard::InputString("q", 0);
    //CMouse::RightClick(40);
    //CKeyboard::InputString("q", 0);
    /*CMouse::RightClick(150);
    CMouse::RightManyClick(2, 100);
    Sleep(50);
    CKeyboard::InputString("3", 0);
    Sleep(500);*/


    //CKeyboard::KeyDown('N');
    //CKeyboard::KeyDown('Q');
    //CKeyboard::KeyUp('N');
    //CKeyboard::KeyUp('Q');
    //Sleep(250);
    //   
    //CKeyboard::InputString("r", 0);
    //Sleep(40);
    //CMouse::RightManyClick(2, 100);
    ////CKeyboard::InputString("q", 0);
    ////CMouse::RightClick(40);
    ////CKeyboard::InputString("q", 0);
    //CMouse::RightClick(150);
    //CMouse::RightManyClick(2, 100);
    //   Sleep(50);
    //   CKeyboard::InputString("3", 0);
    //Sleep(500);
    //CKeyboard::InputString("q", 0);
    //CMouse::RightManyClick(6, 200);

    rightMouse = true;
}