#pragma once

/** QApplication存活期间持有帮助工厂与弹窗；异常展开也按Qt生命周期回收
*/
class LumaPlayerDialogSession
{
public:
    /** 注册帮助工厂，失败由应用调用点记录
    */
    LumaPlayerDialogSession();

    /** 在QApplication析构前释放DialogManager及其Qt对象
    */
    ~LumaPlayerDialogSession();

private:
    /** 初始化LumaPlayerDialogSession对象的状态，不转移输入借用对象的所有权
    @param [in] other 禁止拷贝的源对象，不取得所有权
    */
    LumaPlayerDialogSession(const LumaPlayerDialogSession& other);

    /** 禁止对象赋值，避免复制资源所有权
    @param [in] other 禁止拷贝的源对象，不取得所有权
    @return 禁止赋值，该函数没有实现
    */
    LumaPlayerDialogSession& operator=(const LumaPlayerDialogSession& other);
};