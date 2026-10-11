#ifndef SPLITVIEWER_UI_TESTS_H
#define SPLITVIEWER_UI_TESTS_H
#include <QtCore/QStringList>

/** 执行所选真实UI及Core回归
@param [in] reportDirectory 证据目录
@param [in] selectedCase 数字用例，0表示全部
@return 失败数，非法选择返回2
*/
int SplitViewerRunUiTests(const QString& reportDirectory, int selectedCase = 0);

/** 获取按编号排序的用例名称
@return 含编号的名称列表
*/
QStringList SplitViewerAuditCaseNames();
#endif