#pragma once
#include <QString>
#include <QImage>
#include <QPoint>
class QWidget;
class PdfReaderThumbnailList;

/** 实际窗口拖拽的像素反馈、落点及滚轮回归，使用独立生成的PDF颜色判定
*/
class PdfReaderDragTests
{
public:
    /** 执行29至32号拖动用例，失败抛异常由统一入口记录
    @param [in] id 稳定用例编号
    @param [in] input 独立PDF素材路径
    @param [in] directory 本次截图目录
    */
    static void run(int id, const QString& input, const QString& directory);
private:
    /** 从截图寻找横跨几乎整个视口的细蓝线，排除PDF页图及其选中边框
    @param [in] image 视口截图
    @return 线条中心纵坐标；没有唯一细横线返回负数
    */
    static int lineY(const QImage& image);
    /** 抓取真实视口并要求存在蓝色插入横线
    @param [in] list 被测列表
    @param [in] path 保存的截图路径
    @return 横线中心纵坐标
    */
    static int captureLine(PdfReaderThumbnailList* list, const QString& path);
    /** 在鼠标左键仍按住时发送滚轮，保留真实指针位置
    @param [in] widget 同步接收事件的视口
    @param [in] point 视口坐标
    @param [in] delta 角度增量，每刻度120
    @param [in] control 是否同时按住Ctrl
    */
    static void heldWheel(QWidget* widget, const QPoint& point, int delta, bool control);
};