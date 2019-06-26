#pragma once

/** 窗口返回值
*/
enum DialogResult
{
    /** 错误值，用于初始化
    */
    ERROR_RESULT = 1000,

    /** 确认按钮
    */
    ACCEPT_BUTTON,

    /** 取消按钮
    */
    IGNORE_BUTTON,

    /** 超时
    */
    TIME_OUT,

    /** Esc退出
    */
    ESC_EXIT,

    /** Alt+F4退出
    */
    ALT_F4_EXIT,

    /** 右上角的关闭
    */
    RIGHT_TOP_EXIT,

    /** 代码销毁
    */
    CODE_DESTROY = -1,

    /** 弹框因为UserId重复而关闭
    */
    BUSY = -2
};