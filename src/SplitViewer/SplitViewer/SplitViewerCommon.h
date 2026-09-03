#pragma once

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#define _WIN32_IE 0x0600
#ifndef WINVER
#define WINVER 0x0601
#endif
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0601
#endif

#include <windows.h>
#include <windowsx.h>
#include <commctrl.h>
#include <commdlg.h>
#include <objidl.h>
#define SPLITVIEWER_GDIPLUS_MINMAX
#ifndef min
#define min(a, b) (((a) < (b)) ? (a) : (b))
#endif
#ifndef max
#define max(a, b) (((a) > (b)) ? (a) : (b))
#endif
#include <gdiplus.h>
#ifdef SPLITVIEWER_GDIPLUS_MINMAX
#undef min
#undef max
#undef SPLITVIEWER_GDIPLUS_MINMAX
#endif
#include <shlobj.h>
#include <shellapi.h>
#include <shlwapi.h>
#include <strsafe.h>

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>
struct SplitViewerTempFileGuard;
const wchar_t kSplitViewerWindowClass[] = L"SplitViewerWindow";
const wchar_t kSplitViewerDragFrameClass[] = L"SplitViewerDragFrame";
const wchar_t kSplitViewerAboutWindowClass[] = L"SplitViewerAboutWindow";
const wchar_t kSplitViewerAppTitle[] = L"\u5206\u5C4F\u770B\u56FE";
const wchar_t kSplitViewerAppVersion[] = L"1.0";
const wchar_t kSplitViewerFontName[] = L"\u6977\u4F53";

const int kSplitViewerToolbarHeight = 42;
const int kSplitViewerMargin = 10;
const int kSplitViewerButtonHeight = 28;
const int kSplitViewerButtonWidth = 34;
const int kSplitViewerButtonGap = 8;
const int kSplitViewerSplitterSize = 2;
const int kSplitViewerSplitterHoverSize = 4;
const int kSplitViewerSplitterHitPadding = 5;
const int kSplitViewerPlusButtonSize = 42;
const int kSplitViewerMinPaneSize = 48;
const int kSplitViewerMinLayerWidth = 90;
const int kSplitViewerMinLayerHeight = 70;
const int kSplitViewerLayerHitSize = 7;
const int kSplitViewerConfigThumbnailMaxEdge = 320;
const int kSplitViewerExternalDropMoveThreshold = 8;
const UINT_PTR SPLITVIEWER_TIMER_TIP = 1;
const UINT SPLITVIEWER_WM_APP_EMBED_EXTERNAL_WINDOW = WM_APP + 101;
const UINT SPLITVIEWER_WM_APP_TOGGLE_FULLSCREEN = WM_APP + 102;
const char kSplitViewerConfigMarker[] = "\r\n--SPLITVIEWER_CONFIG_UTF16LE--\r\n";
const BYTE kSplitViewerPngSignature[] = { 0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A };
const BYTE kSplitViewerPngIendChunkType[] = { 'I', 'E', 'N', 'D' };
const BYTE kSplitViewerPngConfigChunkType[] = { 's', 'v', 'C', 'f' };

const COLORREF kSplitViewerWindowBackColor = RGB(238, 238, 238);
const COLORREF kSplitViewerToolbarColor = RGB(246, 246, 246);
const COLORREF kSplitViewerGrayColor = RGB(128, 128, 128);
const COLORREF kSplitViewerSplitterColor = RGB(188, 188, 188);
const COLORREF kSplitViewerSplitterHoverColor = RGB(214, 214, 214);
const COLORREF kSplitViewerFrameColor = RGB(32, 32, 32);
const COLORREF kSplitViewerSelectedFrameColor = RGB(255, 0, 0);
const COLORREF kSplitViewerExternalDropFrameColor = RGB(0, 170, 230);
const COLORREF kSplitViewerExternalDragOutFrameColor = RGB(255, 0, 0);
const COLORREF kSplitViewerFrameTransparentColor = RGB(255, 0, 255);

enum SplitViewerControlId
{
    SPLITVIEWER_ID_SAVE_CONFIG = 1001,
        SPLITVIEWER_ID_SAVE_IMAGE,
        SPLITVIEWER_ID_NEW_LAYER,
        SPLITVIEWER_ID_FULLSCREEN,
        SPLITVIEWER_ID_ABOUT
};

enum SplitViewerMenuId
{
    SPLITVIEWER_MENU_SPLIT_HORIZONTAL = 40001,
        SPLITVIEWER_MENU_SPLIT_VERTICAL,
        SPLITVIEWER_MENU_DELETE_SPLIT
};

enum SplitViewerNodeKind
{
    SPLITVIEWER_NODE_LEAF = 0,
        SPLITVIEWER_NODE_SPLIT = 1
};

enum SplitViewerSplitDirection
{
    SPLITVIEWER_SPLIT_HORIZONTAL = 0,
        SPLITVIEWER_SPLIT_VERTICAL = 1
};

enum SplitViewerDragMode
{
    SPLITVIEWER_DRAG_NONE = 0,
        SPLITVIEWER_DRAG_IMAGE,
        SPLITVIEWER_DRAG_SPLITTER,
        SPLITVIEWER_DRAG_LAYER_MOVE,
        SPLITVIEWER_DRAG_LAYER_RESIZE
};

enum SplitViewerResizeHit
{
    SPLITVIEWER_RESIZE_NONE = 0,
        SPLITVIEWER_RESIZE_LEFT = 1,
        SPLITVIEWER_RESIZE_RIGHT = 2,
        SPLITVIEWER_RESIZE_TOP = 4,
        SPLITVIEWER_RESIZE_BOTTOM = 8
};
/** 计算矩形宽度
@param [in] rc 矩形
@return 返回right-left
*/
int SplitViewerRectWidth(const RECT& rc);

/** 计算矩形高度
@param [in] rc 矩形
@return 返回bottom-top
*/
int SplitViewerRectHeight(const RECT& rc);

/** 构造Win32矩形
@param [in] left 左坐标
@param [in] top 上坐标
@param [in] right 右坐标
@param [in] bottom 下坐标
@return 返回RECT对象
*/
RECT SplitViewerMakeRect(int left, int top, int right, int bottom);

/** 判断矩形是否有可绘制面积
@param [in] rc 矩形
@return 返回true表示宽高均大于0
*/
bool SplitViewerIsRectUsable(const RECT& rc);

/** 判断两个矩形坐标是否完全一致
@param [in] left 左侧矩形
@param [in] right 右侧矩形
@return 返回true表示坐标完全相同
*/
bool SplitViewerAreRectsEqual(const RECT& left, const RECT& right);

/** 判断点是否落在矩形内部
@param [in] rc 矩形
@param [in] pt 点坐标
@return 返回true表示点在矩形内
*/
bool SplitViewerPointInRect(const RECT& rc, POINT pt);

/** 返回扩展后的矩形，不修改传入矩形对象
@param [in] rc 原矩形
@param [in] dx 水平方向扩展像素
@param [in] dy 垂直方向扩展像素
@return 返回扩展后的矩形
*/
RECT SplitViewerInflatedRect(RECT rc, int dx, int dy);

/** 将double值限制在指定范围内
@param [in] value 原值
@param [in] low 最小值
@param [in] high 最大值
@return 返回限制后的值
*/
double SplitViewerClampDouble(double value, double low, double high);

/** 将int值限制在指定范围内
@param [in] value 原值
@param [in] low 最小值
@param [in] high 最大值
@return 返回限制后的值
*/
int SplitViewerClampInt(int value, int low, int high);

/** 格式化整数为宽字符串
@param [in] value 整数值
@return 返回格式化后的宽字符串
*/
std::wstring SplitViewerFormatInt(int value);

/** 格式化浮点数为配置文件使用的宽字符串
@param [in] value 浮点值
@return 返回格式化后的宽字符串
*/
std::wstring SplitViewerFormatDouble(double value);

/** 不区分大小写比较两个宽字符串
@param [in] left 左侧字符串
@param [in] right 右侧字符串
@return 返回true表示两个字符串相同
*/
bool SplitViewerSameText(const wchar_t* left, const wchar_t* right);

/** 判断路径是否已经带文件扩展名
@param [in] path 文件路径
@return 返回true表示路径带扩展名
*/
bool SplitViewerHasExtension(const std::wstring& path);

/** 生成毫秒级时间戳文件名主体
@return 返回yyyyMMddHHmmssSSS格式文本
*/
std::wstring SplitViewerMakeTimestampName();

/** 在路径未带扩展名时补默认扩展名
@param [in] path 原路径
@param [in] ext 默认扩展名
@return 返回带扩展名的路径
*/
std::wstring SplitViewerWithDefaultExtension(const std::wstring& path, const wchar_t* ext);

/** 判断路径是否是SplitViewer配置文件
@param [in] path 文件路径
@return 返回true表示扩展名为.sv
*/
bool SplitViewerIsConfigPath(const wchar_t* path);

/** 查找GDI+图片编码器CLSID
@param [in] format 图片MIME格式
@param [out] clsid 编码器CLSID
@return 返回编码器索引，失败返回-1
*/
int SplitViewerGetEncoderClsid(const WCHAR* format, CLSID* clsid);

/** 根据文件扩展名选择保存图片的MIME格式
@param [in] path 图片保存路径
@return 返回GDI+编码器MIME文本
*/
const WCHAR* SplitViewerImageMimeForPath(const std::wstring& path);

/** 生成配置文件中的节点section名
@param [in] prefix section前缀
@param [in] id 节点编号
@return 返回section名称
*/
std::wstring SplitViewerProfileSectionName(const std::wstring& prefix, int id);

/** 写入配置文件字符串值
@param [in] file 配置文件路径
@param [in] section section名称
@param [in] key 键名
@param [in] value 值文本
*/
void SplitViewerWriteProfileString(const std::wstring& file, const std::wstring& section, const wchar_t* key, const std::wstring& value);

/** 写入配置文件整数值
@param [in] file 配置文件路径
@param [in] section section名称
@param [in] key 键名
@param [in] value 整数值
*/
void SplitViewerWriteProfileInt(const std::wstring& file, const std::wstring& section, const wchar_t* key, int value);

/** 写入配置文件浮点值
@param [in] file 配置文件路径
@param [in] section section名称
@param [in] key 键名
@param [in] value 浮点值
*/
void SplitViewerWriteProfileDouble(const std::wstring& file, const std::wstring& section, const wchar_t* key, double value);

/** 读取配置文件字符串值
@param [in] file 配置文件路径
@param [in] section section名称
@param [in] key 键名
@param [in] defaultValue 默认值
@return 返回读取到的值或默认值
*/
std::wstring SplitViewerReadProfileString(const std::wstring& file, const std::wstring& section, const wchar_t* key, const wchar_t* defaultValue);

/** 读取配置文件整数值
@param [in] file 配置文件路径
@param [in] section section名称
@param [in] key 键名
@param [in] defaultValue 默认值
@return 返回读取到的值或默认值
*/
int SplitViewerReadProfileIntValue(const std::wstring& file, const std::wstring& section, const wchar_t* key, int defaultValue);

/** 读取配置文件浮点值
@param [in] file 配置文件路径
@param [in] section section名称
@param [in] key 键名
@param [in] defaultValue 默认值
@return 返回读取到的值或默认值
*/
double SplitViewerReadProfileDoubleValue(const std::wstring& file, const std::wstring& section, const wchar_t* key, double defaultValue);

/** 创建带UTF-16LE BOM的配置临时文件
@param [in] file 文件路径
@return 返回true表示创建成功
*/
bool SplitViewerCreateUtf16ProfileFile(const std::wstring& file);

/** 读取整个文件到字节数组
@param [in] file 文件路径
@param [out] bytes 文件字节
@return 返回true表示读取成功
*/
bool SplitViewerReadFileBytes(const std::wstring& file, std::vector<BYTE>& bytes);

/** 写入字节数组到文件
@param [in] file 文件路径
@param [in] bytes 字节数据，可在size为0时为空
@param [in] size 字节数量
@return 返回true表示写入成功
*/
bool SplitViewerWriteFileBytes(const std::wstring& file, const BYTE* bytes, size_t size);

/** 创建临时配置文件路径
@param [out] path 临时文件路径
@return 返回true表示路径创建成功
*/
bool SplitViewerCreateTempProfilePath(std::wstring& path);

/** 判断字节数组是否以PNG签名开头
@param [in] bytes 字节数组
@return 返回true表示是PNG文件
*/
bool SplitViewerStartsWithPng(const std::vector<BYTE>& bytes);

/** 在字节数组中查找指定模式
@param [in] bytes 字节数组
@param [in] pattern 待查找模式
@param [in] patternSize 模式字节数
@param [out] index 找到位置
@return 返回true表示找到
*/
bool SplitViewerFindBytes(const std::vector<BYTE>& bytes, const BYTE* pattern, size_t patternSize, size_t& index);

/** 从PNG字节流读取大端uint32
@param [in] bytes 字节数组
@param [in] index 起始位置
@return 返回读取到的DWORD值
*/
DWORD SplitViewerReadPngUInt32(const std::vector<BYTE>& bytes, size_t index);

/** 向PNG字节流追加大端uint32
@param [in,out] bytes 字节数组
@param [in] value 要追加的DWORD值
*/
void SplitViewerAppendPngUInt32(std::vector<BYTE>& bytes, DWORD value);

/** 更新PNG CRC32值
@param [in] crc 当前CRC值
@param [in] bytes 待计算字节
@param [in] size 字节数量
@return 返回更新后的CRC值
*/
DWORD SplitViewerUpdatePngCrc32(DWORD crc, const BYTE* bytes, size_t size);

/** 计算PNG chunk的CRC32值
@param [in] chunkType chunk类型
@param [in] data chunk数据
@param [in] dataSize chunk数据字节数
@return 返回CRC32值
*/
DWORD SplitViewerPngChunkCrc32(const BYTE* chunkType, const BYTE* data, size_t dataSize);

/** 向PNG字节流追加一个chunk
@param [in,out] bytes PNG字节流
@param [in] chunkType chunk类型
@param [in] data chunk数据
@return 返回true表示追加成功
*/
bool SplitViewerAppendPngChunk(std::vector<BYTE>& bytes, const BYTE* chunkType, const std::vector<BYTE>& data);

/** 将缩略图PNG和UTF-16配置内容打包为sv文件字节
@param [in] thumbnailPng 缩略图PNG字节
@param [in] configBytes UTF-16配置字节
@param [out] packageBytes 打包后的sv文件字节
@return 返回true表示打包成功
*/
bool SplitViewerBuildPngConfigPackage(const std::vector<BYTE>& thumbnailPng,
    const std::vector<BYTE>& configBytes,
    std::vector<BYTE>& packageBytes);

/** 从PNG自定义chunk中提取配置字节
@param [in] bytes sv文件字节
@param [out] configBytes 提取到的配置字节
@return 返回true表示提取成功
*/
bool SplitViewerExtractPngChunkConfig(const std::vector<BYTE>& bytes, std::vector<BYTE>& configBytes);

/** 从旧格式PNG尾部标记中提取配置字节
@param [in] bytes sv文件字节
@param [out] configBytes 提取到的配置字节
@return 返回true表示提取成功
*/
bool SplitViewerExtractTrailingConfig(const std::vector<BYTE>& bytes, std::vector<BYTE>& configBytes);

/** 从当前或旧版sv文件中提取配置字节
@param [in] bytes sv文件字节
@param [out] configBytes 提取到的配置字节
@return 返回true表示提取成功
*/
bool SplitViewerExtractEmbeddedConfig(const std::vector<BYTE>& bytes, std::vector<BYTE>& configBytes);

/** 写入带PNG缩略图的sv配置包
@param [in] file sv文件路径
@param [in] thumbnailPng 缩略图PNG字节
@param [in] configBytes UTF-16配置字节
@return 返回true表示写入成功
*/
bool SplitViewerWriteConfigPackage(const std::wstring& file,
    const std::vector<BYTE>& thumbnailPng,
    const std::vector<BYTE>& configBytes);

/** 准备用于GetPrivateProfile读取的配置文件，嵌入配置会释放到临时文件
@param [in] sourcePath 原sv文件路径
@param [out] profilePath 可供Profile API读取的文件路径
@param [in,out] tempProfile 临时文件守卫
@return 返回true表示准备成功
*/
bool SplitViewerPrepareProfileReadFile(const std::wstring& sourcePath,
    std::wstring& profilePath,
    SplitViewerTempFileGuard& tempProfile);

/** 写入注册表字符串值
@param [in] root 注册表根键
@param [in] subKey 子键路径
@param [in] valueName 值名，NULL表示默认值
@param [in] value 值文本
@return 返回true表示写入成功
*/
bool SplitViewerSetRegStringValue(HKEY root, const std::wstring& subKey, const wchar_t* valueName, const std::wstring& value);

/** 判断当前进程是否是临时验证程序
@param [in] exePath exe完整路径
@return 返回true表示应跳过sv文件类型注册
*/
bool SplitViewerIsVerifyExecutable(const std::wstring& exePath);

/** 注册.sv扩展名、默认打开命令和PNG缩略图处理方式
*/
void SplitViewerRegisterSvFileType();