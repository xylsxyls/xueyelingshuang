#pragma once

/** QApplication存活期间持有帮助工厂与弹窗；异常展开也按Qt生命周期回收 */
class LumaPlayerDialogSession
{
public:
    /** 注册帮助工厂，失败由应用调用点记录 */
    LumaPlayerDialogSession();
    /** 在QApplication析构前释放DialogManager及其Qt对象 */
    ~LumaPlayerDialogSession();
private:
    LumaPlayerDialogSession(const LumaPlayerDialogSession&);
    LumaPlayerDialogSession& operator=(const LumaPlayerDialogSession&);
};