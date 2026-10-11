#pragma once
#include <stdint.h>
#include <string>

/** 固定测试编号，与目录及命令行入口一致；1—7为Core，8—100保留，101起为界面及集成场景
*/
enum SplitViewerTestCaseId
{
    // 验证基础树创建和垂直分割
    SPLITVIEWER_TEST_CASE_1 = 1,
    // 验证浮动图层创建
    SPLITVIEWER_TEST_CASE_2 = 2,
    // 验证整层删除释放嵌套分屏
    SPLITVIEWER_TEST_CASE_3 = 3,
    // 验证分割区域计算
    SPLITVIEWER_TEST_CASE_4 = 4,
    // 验证配置序列化和反序列化
    SPLITVIEWER_TEST_CASE_5 = 5,
    // 验证PNG配置包写入和提取
    SPLITVIEWER_TEST_CASE_6 = 6,
    // 验证图片自动适配比例
    SPLITVIEWER_TEST_CASE_7 = 7,
    // 验证空状态配色与加号
    SPLITVIEWER_TEST_CASE_101 = 101,
    // 验证加号取消保持空状态
    SPLITVIEWER_TEST_CASE_102 = 102,
    // 验证菜单水平分割
    SPLITVIEWER_TEST_CASE_103 = 103,
    // 验证上下分屏独立加号
    SPLITVIEWER_TEST_CASE_104 = 104,
    // 验证上半加号加载可见
    SPLITVIEWER_TEST_CASE_105 = 105,
    // 验证下半加号加载可见
    SPLITVIEWER_TEST_CASE_106 = 106,
    // 验证放大不越过分屏
    SPLITVIEWER_TEST_CASE_107 = 107,
    // 验证保存落盘反映界面状态
    SPLITVIEWER_TEST_CASE_108 = 108,
    // 验证图片偏移使用像素
    SPLITVIEWER_TEST_CASE_109 = 109,
    // 验证嵌套垂直分割
    SPLITVIEWER_TEST_CASE_110 = 110,
    // 验证嵌套保留内容及加号
    SPLITVIEWER_TEST_CASE_111 = 111,
    // 验证拖放按落点加载
    SPLITVIEWER_TEST_CASE_112 = 112,
    // 验证基础层删除分屏
    SPLITVIEWER_TEST_CASE_113 = 113,
    // 验证分割线拖动落盘
    SPLITVIEWER_TEST_CASE_114 = 114,
    // 验证删除折叠保留兄弟图片
    SPLITVIEWER_TEST_CASE_115 = 115,
    // 验证根叶子删除禁用
    SPLITVIEWER_TEST_CASE_116 = 116,
    // 验证错误图片保留原内容
    SPLITVIEWER_TEST_CASE_117 = 117,
    // 验证新建图层按钮
    SPLITVIEWER_TEST_CASE_118 = 118,
    // 验证浮动图层放大裁剪
    SPLITVIEWER_TEST_CASE_119 = 119,
    // 验证浮动图层平移裁剪
    SPLITVIEWER_TEST_CASE_120 = 120,
    // 验证Ctrl只移动图层
    SPLITVIEWER_TEST_CASE_121 = 121,
    // 验证图层右边缘调整
    SPLITVIEWER_TEST_CASE_122 = 122,
    // 验证浮动层内分屏
    SPLITVIEWER_TEST_CASE_123 = 123,
    // 验证浮动层下半加号
    SPLITVIEWER_TEST_CASE_124 = 124,
    // 验证浮动层下半加载图片
    SPLITVIEWER_TEST_CASE_125 = 125,
    // 验证按屏幕舞台尺寸导出
    SPLITVIEWER_TEST_CASE_126 = 126,
    // 验证SV缩略图界限
    SPLITVIEWER_TEST_CASE_127 = 127,
    // 验证拖入配置恢复
    SPLITVIEWER_TEST_CASE_128 = 128,
    // 验证恢复图层图片可见
    SPLITVIEWER_TEST_CASE_129 = 129,
    // 验证损坏配置不破坏文档
    SPLITVIEWER_TEST_CASE_130 = 130,
    // 验证重载后继续交互
    SPLITVIEWER_TEST_CASE_131 = 131,
    // 验证边距双击不全屏
    SPLITVIEWER_TEST_CASE_132 = 132,
    // 验证双击只显示舞台
    SPLITVIEWER_TEST_CASE_133 = 133,
    // 验证Esc恢复窗口及工具栏
    SPLITVIEWER_TEST_CASE_134 = 134,
    // 验证F11进入全屏
    SPLITVIEWER_TEST_CASE_135 = 135,
    // 验证F11恢复最大化
    SPLITVIEWER_TEST_CASE_136 = 136,
    // 验证F1隐藏边框落盘
    SPLITVIEWER_TEST_CASE_137 = 137,
    // 验证原生容器创建
    SPLITVIEWER_TEST_CASE_138 = 138,
    // 验证原生窗口快照
    SPLITVIEWER_TEST_CASE_139 = 139,
    // 验证解除恢复原窗口
    SPLITVIEWER_TEST_CASE_140 = 140,
    // 验证独立旧配置字段解析
    SPLITVIEWER_TEST_CASE_141 = 141,
    // 验证循环节点拒绝且不换文档
    SPLITVIEWER_TEST_CASE_142 = 142,
    // 验证缺失节点拒绝
    SPLITVIEWER_TEST_CASE_143 = 143,
    // 验证奇数UTF16拒绝
    SPLITVIEWER_TEST_CASE_144 = 144,
    // 验证未知版本拒绝
    SPLITVIEWER_TEST_CASE_145 = 145,
    // 验证配置CRC损坏拒绝
    SPLITVIEWER_TEST_CASE_146 = 146,
    // 验证旧尾部标记兼容
    SPLITVIEWER_TEST_CASE_147 = 147,
    // 验证绝对分割矩形尺寸
    SPLITVIEWER_TEST_CASE_148 = 148,
    // 验证无边框分割无缝
    SPLITVIEWER_TEST_CASE_149 = 149,
    // 验证多格滚轮运算
    SPLITVIEWER_TEST_CASE_150 = 150,
    // 验证Ctrl精细缩放
    SPLITVIEWER_TEST_CASE_151 = 151,
    // 验证缩放上下界
    SPLITVIEWER_TEST_CASE_152 = 152,
    // 验证手动视图重映射
    SPLITVIEWER_TEST_CASE_153 = 153,
    // 验证Windows系统鼠标拖入
    SPLITVIEWER_TEST_CASE_154 = 154,
    // 验证Windows拖出还原
    SPLITVIEWER_TEST_CASE_155 = 155,
    // 验证关闭恢复被拖入窗口
    SPLITVIEWER_TEST_CASE_156 = 156,
    // 验证PNG/JPEG/BMP/TIFF及GIF失败反馈
    SPLITVIEWER_TEST_CASE_157 = 157,
    // 验证缩放提示按时消失
    SPLITVIEWER_TEST_CASE_158 = 158,
    // 验证两个原生分屏分别恢复
    SPLITVIEWER_TEST_CASE_159 = 159,
    // 验证原生嵌入窗口随内容全屏缩放
    SPLITVIEWER_TEST_CASE_160 = 160,
    // 验证真实播放器拖入后视频像素可见并适配
    SPLITVIEWER_TEST_CASE_161 = 161,
    // 验证双击全屏后视频等比放大且继续播放
    SPLITVIEWER_TEST_CASE_162 = 162,
    // 验证退出全屏后视频保持可见
    SPLITVIEWER_TEST_CASE_163 = 163,
    // 验证移动和调整宿主窗口后视频适配
    SPLITVIEWER_TEST_CASE_164 = 164,
    // 验证视频分屏和相邻图片互不遮挡
    SPLITVIEWER_TEST_CASE_165 = 165,
    // 验证单击暂停继续与重复全屏后播放
    SPLITVIEWER_TEST_CASE_166 = 166,
    // 验证拖出后关闭配置的播放器进程
    SPLITVIEWER_TEST_CASE_167 = 167,
    // 验证关闭宿主同时关闭嵌入播放器
    SPLITVIEWER_TEST_CASE_168 = 168,
    // 验证重复设置已持有根节点保留树与内容
    SPLITVIEWER_TEST_CASE_169 = 169,
    // 验证重复接收已持有图层不重复释放
    SPLITVIEWER_TEST_CASE_170 = 170,
    // 验证嵌入缺失宿主时安全拒绝
    SPLITVIEWER_TEST_CASE_171 = 171,
    // 验证可见控件使用QtControls并保持显示
    SPLITVIEWER_TEST_CASE_172 = 172,
    // 验证真实文件框取消保持空白工作区
    SPLITVIEWER_TEST_CASE_173 = 173,
    // 验证真实文件框确认加载中文空格路径图片
    SPLITVIEWER_TEST_CASE_174 = 174,
    // 验证托管覆盖确认取消重选及确认
    SPLITVIEWER_TEST_CASE_175 = 175,
    // 验证关于按钮使用DialogManager提示框
    SPLITVIEWER_TEST_CASE_176 = 176,
    // 验证托管输入框接受及取消
    SPLITVIEWER_TEST_CASE_177 = 177,
    // 验证基础层删除图层置灰
    SPLITVIEWER_TEST_CASE_178 = 178,
    // 验证右键删除浮动图层包含嵌套分屏
    SPLITVIEWER_TEST_CASE_179 = 179,
    // 验证Delete键按焦点删除浮动图层
    SPLITVIEWER_TEST_CASE_180 = 180,
    // 验证Delete键不影响基础层
    SPLITVIEWER_TEST_CASE_181 = 181,
    // 验证浮动图层左上角移动光标与整体拖动
    SPLITVIEWER_TEST_CASE_182 = 182,
    // 验证关于窗口标题栏叉号关闭
    SPLITVIEWER_TEST_CASE_183 = 183,
    // 验证阴影参数、主体几何与透明渐变
    SPLITVIEWER_TEST_CASE_184 = 184,
    // 验证关于框桌面轮廓拖动四边完整
    SPLITVIEWER_TEST_CASE_185 = 185,
    // 验证关于框桌面整窗拖动无额外框
    SPLITVIEWER_TEST_CASE_186 = 186,
    // 验证阴影显示隐藏、焦点与回收
    SPLITVIEWER_TEST_CASE_187 = 187,
    // 验证轮廓拖动Esc取消并释放捕获
    SPLITVIEWER_TEST_CASE_188 = 188,
    // 验证浮动层8像素热区光标与拖动一致
    SPLITVIEWER_TEST_CASE_189 = 189,
    // 验证加号与右键图片过滤器一致且支持TIFF
    SPLITVIEWER_TEST_CASE_190 = 190,
    // 验证阴影2与4切换保持主体几何
    SPLITVIEWER_TEST_CASE_191 = 191,
    // 验证异常配置拒绝并保留原文档
    SPLITVIEWER_TEST_CASE_192 = 192,
    // 验证Core非有限比例和非叶删除防护
    SPLITVIEWER_TEST_CASE_193 = 193,
    // 验证严格数值解析与坏配置事务性
    SPLITVIEWER_TEST_CASE_194 = 194,
    // 验证UTF16代理对及失败保留输出
    SPLITVIEWER_TEST_CASE_195 = 195,
    // 验证几何溢出与非法分屏方向
    SPLITVIEWER_TEST_CASE_196 = 196,
    // 验证图片缓存共享引用与回收
    SPLITVIEWER_TEST_CASE_197 = 197,
    // 验证中文路径二进制限长读取
    SPLITVIEWER_TEST_CASE_198 = 198,
    // 验证PNG配置原位封装与长度边界
    SPLITVIEWER_TEST_CASE_199 = 199,
    // 验证文件框返回时删除目标图层
    SPLITVIEWER_TEST_CASE_200 = 200,
    // 验证配置保存树深度与图层上限
    SPLITVIEWER_TEST_CASE_201 = 201,
    // 验证Test主界面异步批次与关闭收尾
    SPLITVIEWER_TEST_CASE_202 = 202,
    // 验证四方向和嵌套固定目标
    SPLITVIEWER_TEST_CASE_203 = 203,
    // 验证无效及跨图层迁移保持状态
    SPLITVIEWER_TEST_CASE_204 = 204,
    // 验证图片和视频内容迁移及源清空
    SPLITVIEWER_TEST_CASE_205 = 205,
    // 验证视频路径字段及旧图片兼容
    SPLITVIEWER_TEST_CASE_206 = 206,
    // 验证四方向菜单移动与源加号
    SPLITVIEWER_TEST_CASE_207 = 207,
    // 验证覆盖确认取消及接受
    SPLITVIEWER_TEST_CASE_208 = 208,
    // 验证浮动图层范围隔离
    SPLITVIEWER_TEST_CASE_209 = 209,
    // 验证环境变量筛选及失败保留图片
    SPLITVIEWER_TEST_CASE_210 = 210,
    // 验证真实视频加载移动及全屏
    SPLITVIEWER_TEST_CASE_211 = 211,
    // 验证视频sv保存并重启恢复
    SPLITVIEWER_TEST_CASE_212 = 212,
    // 验证关闭策略及多开进程隔离
    SPLITVIEWER_TEST_CASE_213 = 213,
    // 验证在途启动删除与替换收尾
    SPLITVIEWER_TEST_CASE_214 = 214,
    // 验证无窗口启动超时与原内容保留
    SPLITVIEWER_TEST_CASE_215 = 215,
    // 验证恢复失败保留路径并允许加号重试
    SPLITVIEWER_TEST_CASE_216 = 216,
    // 验证Core配置快照及无效配置隔离
    SPLITVIEWER_TEST_CASE_217 = 217,
    // 验证用例登记与数字范围解析
    SPLITVIEWER_TEST_CASE_218 = 218,
    // 验证弹窗会话重复初始化和注册回滚
    SPLITVIEWER_TEST_CASE_219 = 219,
    // 验证独立Core文档反复分屏恢复压力
    SPLITVIEWER_TEST_CASE_220 = 220,
    // 验证窗口任务关闭与多实例隔离
    SPLITVIEWER_TEST_CASE_221 = 221,
    // 验证场景快照独占与失败事务
    SPLITVIEWER_TEST_CASE_222 = 222,
    // 验证异步解码失败保留原图片
    SPLITVIEWER_TEST_CASE_223 = 223,
    // 验证解码在途关闭主窗口
    SPLITVIEWER_TEST_CASE_224 = 224,
    // 验证嵌入视频自行改位后保持分屏
    SPLITVIEWER_TEST_CASE_225 = 225,
    // 验证播放器内部关闭后进程退出
    SPLITVIEWER_TEST_CASE_226 = 226,
    // 验证嵌入窗口销毁后回收残留进程
    SPLITVIEWER_TEST_CASE_227 = 227,
    // 验证窗口先销毁再关闭宿主不残留进程
    SPLITVIEWER_TEST_CASE_228 = 228
};

/** 用例执行类别；默认批次只选择瞬时用例，压力用例须显式选择
*/
enum SplitViewerTestCaseType
{
    // 常规功能及边界回归，默认瞬时批次包含此类用例
    SPLITVIEWER_TEST_INSTANT = 0,
    // 长时重复或资源压力场景，默认批次排除，显式压力或全部入口才执行
    SPLITVIEWER_TEST_PRESSURE = 1
};

/** 稳定用例登记，只描述独立预期，不存放运行结果
*/
struct SplitViewerTestCase
{
public:
    // 中文功能名称
    std::wstring m_name;
    // 瞬时或压力属性
    SplitViewerTestCaseType m_type;
    // 真实输入场景
    std::wstring m_scene;
    // 必须满足的独立行为预期
    std::wstring m_expected;
    // 单项看门狗上限，毫秒
    int32_t m_timeoutMs;

public:
    /** 初始化不可变目录项
    @param [in] name 中文功能名
    @param [in] type 瞬时或压力属性
    @param [in] scene 独立准备和真实触发步骤
    @param [in] expected 结果断言依据
    @param [in] timeoutMs 超时保护上限，毫秒
    */
    SplitViewerTestCase(const std::wstring& name, SplitViewerTestCaseType type, const std::wstring& scene,
        const std::wstring& expected, int32_t timeoutMs);
};