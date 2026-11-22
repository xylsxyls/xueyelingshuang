#pragma once

/** 独立编译本提供方的接口回归，避免同名C/C++门面混用
*/
class NetCppBoundaryProbe
{
public:
    /** 检查C++公共头不泄漏C导出，宿主入口及非法参数契约
    */
    static void cppBoundary();
};