#include "SplitViewerTestCaseHelper.h"
#include "CStringManager/CStringManagerAPI.h"
#include <algorithm>

const std::map<SplitViewerTestCaseId, SplitViewerTestCase> SplitViewerTestCaseHelper::kCases = {
    {SPLITVIEWER_TEST_CASE_1,
        SplitViewerTestCase(L"基础树创建和垂直分割", SPLITVIEWER_TEST_INSTANT, L"独立空文档的根叶子执行垂直分割",
            L"根变为垂直分割节点，两个独立子叶子均存在", 5000)},
    {SPLITVIEWER_TEST_CASE_2,
        SplitViewerTestCase(L"浮动图层创建", SPLITVIEWER_TEST_INSTANT, L"在独立文档中创建第一个浮动图层",
            L"图层数为1且选中索引为0", 5000)},
    {SPLITVIEWER_TEST_CASE_3,
        SplitViewerTestCase(L"整层删除释放嵌套分屏", SPLITVIEWER_TEST_INSTANT, L"新图层内分屏后删除整层",
            L"图层数归零且选中背景板，不留下子树", 5000)},
    {SPLITVIEWER_TEST_CASE_4,
        SplitViewerTestCase(L"分割区域计算", SPLITVIEWER_TEST_INSTANT, L"默认比例的垂直树分割单位矩形",
            L"左右内容区域和分割线的宽度均为正", 5000)},
    {SPLITVIEWER_TEST_CASE_5,
        SplitViewerTestCase(L"配置序列化和反序列化", SPLITVIEWER_TEST_INSTANT,
            L"基础树图片与一浮动层编码为UTF16并在新文档解析", L"恢复相同分割树和一浮动层，编码与解析均实际成功", 5000)},
    {SPLITVIEWER_TEST_CASE_6,
        SplitViewerTestCase(L"PNG配置包写入和提取", SPLITVIEWER_TEST_INSTANT, L"16像素PNG预览与配置打包后提取",
            L"封装和提取成功，配置字节逐字节一致", 5000)},
    {SPLITVIEWER_TEST_CASE_7,
        SplitViewerTestCase(L"图片自动适配比例", SPLITVIEWER_TEST_INSTANT, L"400乘200图片适配200乘200内容区",
            L"保持宽高比的比例精确为0.5", 5000)},
    {SPLITVIEWER_TEST_CASE_101,
        SplitViewerTestCase(L"空状态配色与加号", SPLITVIEWER_TEST_INSTANT, L"显示全新空舞台并采样叶子与加号区域",
            L"叶子为旧版灰色，加号底色、线条与中心位置符合字面量预期", 20000)},
    {SPLITVIEWER_TEST_CASE_102,
        SplitViewerTestCase(L"加号取消保持空状态", SPLITVIEWER_TEST_INSTANT, L"点击空叶子加号，在文件选择中取消",
            L"叶子仍为空，灰色和加号不变", 20000)},
    {SPLITVIEWER_TEST_CASE_103,
        SplitViewerTestCase(L"菜单水平分割", SPLITVIEWER_TEST_INSTANT, L"在根叶子右键选择水平分割",
            L"上、下两个可见区域与一条水平分割线形成", 20000)},
    {SPLITVIEWER_TEST_CASE_104,
        SplitViewerTestCase(L"上下分屏独立加号", SPLITVIEWER_TEST_INSTANT, L"对空舞台执行水平分割后绘制",
            L"上下两区各自中心都有加号", 20000)},
    {SPLITVIEWER_TEST_CASE_105,
        SplitViewerTestCase(L"上半加号加载可见", SPLITVIEWER_TEST_INSTANT, L"在水平分割的上半区点击加号选择红色PNG",
            L"上半区显示红图，下半区仍为空", 20000)},
    {SPLITVIEWER_TEST_CASE_106,
        SplitViewerTestCase(L"下半加号加载可见", SPLITVIEWER_TEST_INSTANT, L"在水平分割的下半区点击加号选择蓝色PNG",
            L"下半区显示蓝图，上半区内容不被覆盖", 20000)},
    {SPLITVIEWER_TEST_CASE_107,
        SplitViewerTestCase(L"放大不越过分屏", SPLITVIEWER_TEST_INSTANT, L"在分屏内连续滚轮放大图片",
            L"图片可放大，邻区及分割线像素不被越界图像覆盖", 20000)},
    {SPLITVIEWER_TEST_CASE_108,
        SplitViewerTestCase(L"保存落盘反映界面状态", SPLITVIEWER_TEST_INSTANT, L"真实界面分屏并加载图片后保存.sv",
            L"落盘配置包含实际分割关系及相应图片路径", 20000)},
    {SPLITVIEWER_TEST_CASE_109,
        SplitViewerTestCase(L"图片偏移使用像素", SPLITVIEWER_TEST_INSTANT, L"在有图片的叶子内拖动后保存配置",
            L"OffsetX与OffsetY按实际鼠标位移以像素保存", 20000)},
    {SPLITVIEWER_TEST_CASE_110,
        SplitViewerTestCase(L"嵌套垂直分割", SPLITVIEWER_TEST_INSTANT, L"在已有水平分割的一个叶子再垂直分割",
            L"该叶子形成左右两区，其他区域不变", 20000)},
    {SPLITVIEWER_TEST_CASE_111,
        SplitViewerTestCase(L"嵌套保留内容及加号", SPLITVIEWER_TEST_INSTANT, L"对已有图片的叶子执行嵌套分割",
            L"图片保留在第一子区，第二子区为空且显示加号", 20000)},
    {SPLITVIEWER_TEST_CASE_112,
        SplitViewerTestCase(L"拖放按落点加载", SPLITVIEWER_TEST_INSTANT, L"将不同颜色PNG拖放到指定叶子坐标",
            L"仅落点所在叶子显示对应颜色", 20000)},
    {SPLITVIEWER_TEST_CASE_113,
        SplitViewerTestCase(L"基础层删除分屏", SPLITVIEWER_TEST_INSTANT, L"在基础层非根叶子选择删除当前分屏",
            L"对应父节点折叠，兄弟成为该区域内容", 20000)},
    {SPLITVIEWER_TEST_CASE_114,
        SplitViewerTestCase(L"分割线拖动落盘", SPLITVIEWER_TEST_INSTANT, L"拖动分割线到新位置后保存配置",
            L"落盘Ratio与真实布局一致，未停留在默认比例", 20000)},
    {SPLITVIEWER_TEST_CASE_115,
        SplitViewerTestCase(L"删除折叠保留兄弟图片", SPLITVIEWER_TEST_INSTANT, L"删除一个分屏并检查兄弟图片",
            L"兄弟图片仍可见且路径保留，不因父节点释放丢失", 20000)},
    {SPLITVIEWER_TEST_CASE_116,
        SplitViewerTestCase(L"根叶子删除禁用", SPLITVIEWER_TEST_INSTANT, L"在唯一根叶子的右键菜单检查删除分屏",
            L"该操作置灰且无法删除整个基础树", 20000)},
    {SPLITVIEWER_TEST_CASE_117,
        SplitViewerTestCase(L"错误图片保留原内容", SPLITVIEWER_TEST_INSTANT, L"有正常图片时加载损坏图片",
            L"出现失败反馈，原图片和文档仍可见", 20000)},
    {SPLITVIEWER_TEST_CASE_118,
        SplitViewerTestCase(L"新建图层按钮", SPLITVIEWER_TEST_INSTANT, L"点击工具栏的新建图层",
            L"出现默认位置与尺寸的浮动层，内容为空", 20000)},
    {SPLITVIEWER_TEST_CASE_119,
        SplitViewerTestCase(L"浮动图层放大裁剪", SPLITVIEWER_TEST_INSTANT, L"加载浮动层图片并滚轮放大",
            L"浮动层图片被自身边界裁剪，基础层邻区像素不变", 20000)},
    {SPLITVIEWER_TEST_CASE_120,
        SplitViewerTestCase(L"浮动图层平移裁剪", SPLITVIEWER_TEST_INSTANT, L"在浮动层内平移较大图片",
            L"图片偏移变化，但任何部分不能越出浮动层", 20000)},
    {SPLITVIEWER_TEST_CASE_121,
        SplitViewerTestCase(L"Ctrl只移动图层", SPLITVIEWER_TEST_INSTANT, L"按住Ctrl在浮动层内部拖动",
            L"整层位置改变而尺寸保持，图片内部偏移不替代层移动", 20000)},
    {SPLITVIEWER_TEST_CASE_122,
        SplitViewerTestCase(L"图层右边缘调整", SPLITVIEWER_TEST_INSTANT, L"拖动浮动层右边缘",
            L"仅对应边界和宽度改变，工作区约束仍生效", 20000)},
    {SPLITVIEWER_TEST_CASE_123,
        SplitViewerTestCase(L"浮动层内分屏", SPLITVIEWER_TEST_INSTANT, L"在浮动层叶子内水平分割",
            L"浮动层形成独立上下分屏，基础树不变", 20000)},
    {SPLITVIEWER_TEST_CASE_124,
        SplitViewerTestCase(L"浮动层下半加号", SPLITVIEWER_TEST_INSTANT, L"绘制水平分割的空浮动层",
            L"两个浮动叶子分别显示独立加号", 20000)},
    {SPLITVIEWER_TEST_CASE_125,
        SplitViewerTestCase(L"浮动层下半加载图片", SPLITVIEWER_TEST_INSTANT, L"点击浮动层下半区加号选择图片",
            L"图片在下半区可见，上半区与基础层不串改", 20000)},
    {SPLITVIEWER_TEST_CASE_126,
        SplitViewerTestCase(L"按屏幕舞台尺寸导出", SPLITVIEWER_TEST_INSTANT, L"按另存图片入口导出当前舞台",
            L"图片尺寸来自屏幕适配后的舞台，工具栏和外边距不进入结果", 20000)},
    {SPLITVIEWER_TEST_CASE_127,
        SplitViewerTestCase(L"SV缩略图界限", SPLITVIEWER_TEST_INSTANT, L"保存配置并独立解码PNG预览",
            L"缩略图只包含舞台，最大边不超过320像素，取整误差不超过1", 20000)},
    {SPLITVIEWER_TEST_CASE_128,
        SplitViewerTestCase(L"拖入配置恢复", SPLITVIEWER_TEST_INSTANT, L"将已保存的.sv拖放到另一产品窗口",
            L"基础树、浮动层及图片布局实际恢复", 20000)},
    {SPLITVIEWER_TEST_CASE_129,
        SplitViewerTestCase(L"恢复图层图片可见", SPLITVIEWER_TEST_INSTANT, L"恢复带浮动层图片的配置并绘制",
            L"浮动层图片在正确区域可见，不只恢复路径标记", 20000)},
    {SPLITVIEWER_TEST_CASE_130,
        SplitViewerTestCase(L"损坏配置不破坏文档", SPLITVIEWER_TEST_INSTANT, L"有正常场景时打开损坏.sv",
            L"报告错误，原树、图层和内容保持", 20000)},
    {SPLITVIEWER_TEST_CASE_131,
        SplitViewerTestCase(L"重载后继续交互", SPLITVIEWER_TEST_INSTANT, L"重新加载有效配置后再分屏与交互",
            L"操作作用于新树，没有使用上一个文档的旧节点", 20000)},
    {SPLITVIEWER_TEST_CASE_132,
        SplitViewerTestCase(L"边距双击不全屏", SPLITVIEWER_TEST_INSTANT, L"双击舞台外侧灰色边距",
            L"保持普通窗口，工具栏仍可见", 20000)},
    {SPLITVIEWER_TEST_CASE_133,
        SplitViewerTestCase(L"双击只显示舞台", SPLITVIEWER_TEST_INSTANT, L"在内容区双击进入全屏",
            L"只显示舞台内容，工具栏和普通窗口边框隐藏", 20000)},
    {SPLITVIEWER_TEST_CASE_134,
        SplitViewerTestCase(
            L"Esc恢复窗口及工具栏", SPLITVIEWER_TEST_INSTANT, L"从内容全屏按Esc退出", L"普通几何及工具栏恢复", 20000)},
    {SPLITVIEWER_TEST_CASE_135,
        SplitViewerTestCase(
            L"F11进入全屏", SPLITVIEWER_TEST_INSTANT, L"普通窗口按F11", L"进入内容全屏并隐藏工具栏", 20000)},
    {SPLITVIEWER_TEST_CASE_136,
        SplitViewerTestCase(L"F11恢复最大化", SPLITVIEWER_TEST_INSTANT, L"最大化窗口进入全屏后再次F11",
            L"恢复最大化状态，不能被延迟消息改为普通窗口", 20000)},
    {SPLITVIEWER_TEST_CASE_137,
        SplitViewerTestCase(L"F1隐藏边框落盘", SPLITVIEWER_TEST_INSTANT, L"按F1隐藏边框后保存配置",
            L"界面无分割缝隙且BorderVisible保存为关闭", 20000)},
    {SPLITVIEWER_TEST_CASE_138,
        SplitViewerTestCase(L"原生容器创建", SPLITVIEWER_TEST_INSTANT, L"用测试外部窗口创建原生嵌入容器",
            L"存在真实原生子窗口及对应父子关系", 20000)},
    {SPLITVIEWER_TEST_CASE_139,
        SplitViewerTestCase(L"原生窗口快照", SPLITVIEWER_TEST_INSTANT, L"获取测试嵌入窗口快照",
            L"实际得到非空图像，不能以容器存在代替快照", 20000)},
    {SPLITVIEWER_TEST_CASE_140,
        SplitViewerTestCase(L"解除恢复原窗口", SPLITVIEWER_TEST_INSTANT, L"解除测试外部窗口嵌入",
            L"外部窗口存活并恢复独立顶层，容器释放", 20000)},
    {SPLITVIEWER_TEST_CASE_141,
        SplitViewerTestCase(L"独立旧配置字段解析", SPLITVIEWER_TEST_INSTANT, L"读取独立手写旧版UTF16字段",
            L"正确恢复Version及树、图层、路径、变换字段", 20000)},
    {SPLITVIEWER_TEST_CASE_142,
        SplitViewerTestCase(L"循环节点拒绝且不换文档", SPLITVIEWER_TEST_INSTANT, L"读入含循环引用的手写配置",
            L"解析失败且原文档不被替换", 20000)},
    {SPLITVIEWER_TEST_CASE_143,
        SplitViewerTestCase(L"缺失节点拒绝", SPLITVIEWER_TEST_INSTANT, L"读入引用不存在节点的手写配置",
            L"解析失败，不创建缺失节点的部分树", 20000)},
    {SPLITVIEWER_TEST_CASE_144,
        SplitViewerTestCase(L"奇数UTF16拒绝", SPLITVIEWER_TEST_INSTANT, L"读入奇数字节长度的UTF16配置",
            L"拒绝不完整编码，原文档保持", 20000)},
    {SPLITVIEWER_TEST_CASE_145,
        SplitViewerTestCase(L"未知版本拒绝", SPLITVIEWER_TEST_INSTANT, L"读入不支持的配置版本",
            L"明确失败，不将未知格式当作当前版本", 20000)},
    {SPLITVIEWER_TEST_CASE_146,
        SplitViewerTestCase(L"配置CRC损坏拒绝", SPLITVIEWER_TEST_INSTANT, L"破坏.sv中svCf数据或CRC",
            L"CRC校验拒绝提取，保留原输出", 20000)},
    {SPLITVIEWER_TEST_CASE_147,
        SplitViewerTestCase(L"旧尾部标记兼容", SPLITVIEWER_TEST_INSTANT, L"在PNG尾部追加旧标记和UTF16配置",
            L"能按旧格式提取完全相同的配置字节", 20000)},
    {SPLITVIEWER_TEST_CASE_148,
        SplitViewerTestCase(L"绝对分割矩形尺寸", SPLITVIEWER_TEST_INSTANT, L"按绝对矩形分割父区并比较各边界",
            L"两个内容区和分割线满足独立计算的绝对尺寸", 20000)},
    {SPLITVIEWER_TEST_CASE_149,
        SplitViewerTestCase(L"无边框分割无缝", SPLITVIEWER_TEST_INSTANT, L"在隐藏边框状态分割工作区",
            L"子区域连续铺满父区，没有残留分割空隙", 20000)},
    {SPLITVIEWER_TEST_CASE_150,
        SplitViewerTestCase(L"多格滚轮运算", SPLITVIEWER_TEST_INSTANT, L"对图片连续输入多个120单位滚轮格",
            L"比例按每格1.05的字面量运算，不能把多格当一格", 20000)},
    {SPLITVIEWER_TEST_CASE_151,
        SplitViewerTestCase(
            L"Ctrl精细缩放", SPLITVIEWER_TEST_INSTANT, L"按住Ctrl输入滚轮事件", L"每格使用1.01精细步长", 20000)},
    {SPLITVIEWER_TEST_CASE_152,
        SplitViewerTestCase(L"缩放上下界", SPLITVIEWER_TEST_INSTANT, L"连续缩放至极大和极小",
            L"比例保持在适配比例的0.05至50倍之间", 20000)},
    {SPLITVIEWER_TEST_CASE_153,
        SplitViewerTestCase(L"手动视图重映射", SPLITVIEWER_TEST_INSTANT, L"改变手动视图的内容区宽高",
            L"图片比例及像素偏移按旧版尺寸重映射关系保持", 20000)},
    {SPLITVIEWER_TEST_CASE_154,
        SplitViewerTestCase(L"Windows系统鼠标拖入", SPLITVIEWER_TEST_INSTANT,
            L"通过真实系统输入将自产外部窗口拖入空叶子", L"系统钩子命中并形成真实嵌入父子关系", 20000)},
    {SPLITVIEWER_TEST_CASE_155,
        SplitViewerTestCase(L"Windows拖出还原", SPLITVIEWER_TEST_INSTANT, L"将自产嵌入窗口拖出舞台",
            L"恢复独立顶层和可见性，宿主对应区域清空", 20000)},
    {SPLITVIEWER_TEST_CASE_156,
        SplitViewerTestCase(L"关闭恢复被拖入窗口", SPLITVIEWER_TEST_INSTANT, L"关闭含自产外部窗口的宿主",
            L"窗口仍存活并恢复独立，不能误终止非关闭策略进程", 20000)},
    {SPLITVIEWER_TEST_CASE_157,
        SplitViewerTestCase(L"PNG/JPEG/BMP/TIFF及GIF失败反馈", SPLITVIEWER_TEST_INSTANT,
            L"分别导出PNG、JPEG、BMP、TIFF及GIF", L"四种可编码格式能解码实际文件，GIF不能编码时明确失败", 20000)},
    {SPLITVIEWER_TEST_CASE_158,
        SplitViewerTestCase(L"缩放提示按时消失", SPLITVIEWER_TEST_INSTANT, L"滚轮缩放后观察提示并等待1100毫秒",
            L"显示相对适配百分比，最后一次操作约1秒后消失", 20000)},
    {SPLITVIEWER_TEST_CASE_159,
        SplitViewerTestCase(L"两个原生分屏分别恢复", SPLITVIEWER_TEST_INSTANT, L"两个分屏分别嵌入自产窗口后关闭宿主",
            L"两窗口独立恢复，释放一个不能影响另一个", 20000)},
    {SPLITVIEWER_TEST_CASE_160,
        SplitViewerTestCase(L"原生嵌入窗口随内容全屏缩放", SPLITVIEWER_TEST_INSTANT, L"将原生窗口嵌入后双击内容全屏",
            L"每个真实客户区随分屏同步扩大，不能只改Qt外壳", 20000)},
    {SPLITVIEWER_TEST_CASE_161,
        SplitViewerTestCase(L"真实播放器拖入后视频像素可见并适配", SPLITVIEWER_TEST_INSTANT,
            L"真实LumaPlayer播放逐帧AVI并通过系统拖入", L"桌面四象限颜色可见且按视频比例适配，帧编号实际变化", 20000)},
    {SPLITVIEWER_TEST_CASE_162,
        SplitViewerTestCase(L"双击全屏后视频等比放大且继续播放", SPLITVIEWER_TEST_INSTANT, L"对真实嵌入播放器双击全屏",
            L"视频在扩大后的分屏内等比占满且播放持续，进程和嵌入身份保持", 20000)},
    {SPLITVIEWER_TEST_CASE_163,
        SplitViewerTestCase(L"退出全屏后视频保持可见", SPLITVIEWER_TEST_INSTANT, L"对真实全屏嵌入播放器再次双击",
            L"恢复原分屏位置和尺寸，视频继续播放", 20000)},
    {SPLITVIEWER_TEST_CASE_164,
        SplitViewerTestCase(L"移动和调整宿主窗口后视频适配", SPLITVIEWER_TEST_INSTANT, L"移动并调整含视频的宿主窗口",
            L"桌面视频四象限仍按客户区适配", 20000)},
    {SPLITVIEWER_TEST_CASE_165,
        SplitViewerTestCase(L"视频分屏和相邻图片互不遮挡", SPLITVIEWER_TEST_INSTANT, L"分割真实视频叶子并加载相邻图片",
            L"图片与视频同时可见且互不遮挡", 20000)},
    {SPLITVIEWER_TEST_CASE_166,
        SplitViewerTestCase(L"单击暂停继续与重复全屏后播放", SPLITVIEWER_TEST_INSTANT,
            L"真实视频单击暂停、继续并再次进入退出全屏", L"暂停时帧编号停止，继续及全屏后帧编号变化，画面始终可见",
            20000)},
    {SPLITVIEWER_TEST_CASE_167,
        SplitViewerTestCase(L"拖出后关闭配置的播放器进程", SPLITVIEWER_TEST_INSTANT, L"把实际LumaPlayer拖出工作区",
            L"默认关闭集合命中并关闭该实例，源叶子恢复空白", 20000)},
    {SPLITVIEWER_TEST_CASE_168,
        SplitViewerTestCase(L"关闭宿主同时关闭嵌入播放器", SPLITVIEWER_TEST_INSTANT,
            L"真实LumaPlayer嵌入后正常关闭宿主", L"被嵌入实例随宿主关闭，不恢复独立播放器", 20000)},
    {SPLITVIEWER_TEST_CASE_169,
        SplitViewerTestCase(L"重复设置已持有根节点保留树与内容", SPLITVIEWER_TEST_INSTANT,
            L"把文档当前持有的根指针再次设置为根", L"不重复释放，原树结构和内容保持", 20000)},
    {SPLITVIEWER_TEST_CASE_170,
        SplitViewerTestCase(L"重复接收已持有图层不重复释放", SPLITVIEWER_TEST_INSTANT, L"把已持有图层再次追加并重置",
            L"图层数量不重复增加，树只释放一次", 20000)},
    {SPLITVIEWER_TEST_CASE_171,
        SplitViewerTestCase(L"嵌入缺失宿主时安全拒绝", SPLITVIEWER_TEST_INSTANT, L"以有效外部句柄和空宿主调用嵌入",
            L"返回失败，外部窗口仍可见且几何、归属未改变", 20000)},
    {SPLITVIEWER_TEST_CASE_172,
        SplitViewerTestCase(L"可见控件使用QtControls并保持显示", SPLITVIEWER_TEST_INSTANT,
            L"显示实际产品窗口并检查可见控件", L"主窗口、画布、工具栏、状态栏及标签使用QtControls并正常显示", 20000)},
    {SPLITVIEWER_TEST_CASE_173,
        SplitViewerTestCase(L"真实文件框取消保持空白工作区", SPLITVIEWER_TEST_INSTANT,
            L"通过产品加号启动测试托管选择器再取消", L"实际点击取消并保持空舞台，托管窗口全部回收", 20000)},
    {SPLITVIEWER_TEST_CASE_174,
        SplitViewerTestCase(L"真实文件框确认加载中文空格路径图片", SPLITVIEWER_TEST_INSTANT,
            L"在托管选择器确认带中文和空格的PNG路径", L"文件路径正确返回并实际显示红色图片", 20000)},
    {SPLITVIEWER_TEST_CASE_175,
        SplitViewerTestCase(L"托管覆盖确认取消重选及确认", SPLITVIEWER_TEST_INSTANT,
            L"托管保存选择器选择既有文件并取消再确认覆盖", L"取消可重新选择，确认返回原路径，选择器自身不改写文件",
            20000)},
    {SPLITVIEWER_TEST_CASE_176,
        SplitViewerTestCase(L"关于按钮使用DialogManager提示框", SPLITVIEWER_TEST_INSTANT,
            L"点击产品关于按钮并操作知道了", L"PNG图标、标题和版本及浅色内容存在，DialogManager回收全部弹窗", 20000)},
    {SPLITVIEWER_TEST_CASE_177,
        SplitViewerTestCase(L"托管输入框接受及取消", SPLITVIEWER_TEST_INSTANT, L"打开托管输入框分别接受文本与取消",
            L"接受回传正确文本，取消不伪造接受结果", 20000)},
    {SPLITVIEWER_TEST_CASE_178,
        SplitViewerTestCase(L"基础层删除图层置灰", SPLITVIEWER_TEST_INSTANT, L"在基础层右键检查删除图层",
            L"动作置灰，基础层不能被删除", 20000)},
    {SPLITVIEWER_TEST_CASE_179,
        SplitViewerTestCase(L"右键删除浮动图层包含嵌套分屏", SPLITVIEWER_TEST_INSTANT,
            L"对有嵌套分屏的浮动层右键删除整层", L"整层及其所有子分屏移除，基础层保持", 20000)},
    {SPLITVIEWER_TEST_CASE_180,
        SplitViewerTestCase(L"Delete键按焦点删除浮动图层", SPLITVIEWER_TEST_INSTANT, L"聚焦浮动层画布后按Delete",
            L"删除整个浮动层，异步暂停输入后仍恢复正确画布焦点", 20000)},
    {SPLITVIEWER_TEST_CASE_181,
        SplitViewerTestCase(L"Delete键不影响基础层", SPLITVIEWER_TEST_INSTANT, L"基础层画布获得焦点后按Delete",
            L"基础树和内容不变", 20000)},
    {SPLITVIEWER_TEST_CASE_182,
        SplitViewerTestCase(L"浮动图层左上角移动光标与整体拖动", SPLITVIEWER_TEST_INSTANT, L"悬停浮动层左上热区并拖动",
            L"显示四向移动光标，整层移动而尺寸不变", 20000)},
    {SPLITVIEWER_TEST_CASE_183,
        SplitViewerTestCase(L"关于窗口标题栏叉号关闭", SPLITVIEWER_TEST_INSTANT,
            L"打开关于框，检查布局后点击标题细线叉",
            L"资源图标、18像素粗体标题、520乘340主体及居中正确，叉关闭并回收窗口", 20000)},
    {SPLITVIEWER_TEST_CASE_184,
        SplitViewerTestCase(L"阴影参数、主体几何与透明渐变", SPLITVIEWER_TEST_INSTANT,
            L"在DialogBase切换关闭、零、负数及正阴影参数",
            L"主体几何不变，正参数外侧为透明衰减软影，禁用时无影且极值不溢出", 20000)},
    {SPLITVIEWER_TEST_CASE_185,
        SplitViewerTestCase(L"关于框桌面轮廓拖动四边完整", SPLITVIEWER_TEST_INSTANT,
            L"临时选择系统轮廓模式，通过真实输入拖动关于标题",
            L"移动中目标黑框四边完整，松开到目标且无残框；结束恢复系统设置", 20000)},
    {SPLITVIEWER_TEST_CASE_186,
        SplitViewerTestCase(L"关于框桌面整窗拖动无额外框", SPLITVIEWER_TEST_INSTANT,
            L"临时选择系统整窗模式，通过真实输入拖动关于标题",
            L"主体随鼠标移动，无额外目标黑框；静止后阴影恢复且系统设置还原", 20000)},
    {SPLITVIEWER_TEST_CASE_187,
        SplitViewerTestCase(L"阴影显示隐藏、焦点与回收", SPLITVIEWER_TEST_INSTANT,
            L"三次显示隐藏、最小化恢复及阴影参数切换后销毁主体", L"阴影跟随可见性，焦点仍在正文，销毁后无孤立影子",
            20000)},
    {SPLITVIEWER_TEST_CASE_188,
        SplitViewerTestCase(L"轮廓拖动Esc取消并释放捕获", SPLITVIEWER_TEST_INSTANT, L"轮廓模式拖动标题中按Esc取消",
            L"主体保持原位置，阴影恢复且鼠标捕获释放，无目标残框", 20000)},
    {SPLITVIEWER_TEST_CASE_189,
        SplitViewerTestCase(L"浮动层8像素热区光标与拖动一致", SPLITVIEWER_TEST_INSTANT,
            L"浮动层左上内缩8逻辑像素处移动鼠标并拖动", L"四向光标与实际整层移动一致，层尺寸保持", 20000)},
    {SPLITVIEWER_TEST_CASE_190,
        SplitViewerTestCase(L"加号与右键图片过滤器一致且支持TIFF", SPLITVIEWER_TEST_INSTANT,
            L"分别通过加号与右键加载并取消选择", L"两个入口过滤器相同，保留TIFF、JFIF；视频仅在有效环境配置下加入",
            20000)},
    {SPLITVIEWER_TEST_CASE_191,
        SplitViewerTestCase(L"阴影2与4切换保持主体几何", SPLITVIEWER_TEST_INSTANT,
            L"同一DialogBase按2到4再到2切换阴影级别", L"主体520乘340和全局位置不变，阴影范围随级别扩大再恢复", 20000)},
    {SPLITVIEWER_TEST_CASE_192,
        SplitViewerTestCase(L"异常配置拒绝并保留原文档", SPLITVIEWER_TEST_INSTANT,
            L"读入负数或超限LayerCount及非正图片Scale", L"拒绝坏配置，原路径、图层及选择保持", 20000)},
    {SPLITVIEWER_TEST_CASE_193,
        SplitViewerTestCase(L"Core非有限比例和非叶删除防护", SPLITVIEWER_TEST_INSTANT,
            L"传入NaN或无穷舞台比例，并对非叶节点删除", L"比例仍有限有效，删除拒绝且子树保留", 20000)},
    {SPLITVIEWER_TEST_CASE_194,
        SplitViewerTestCase(L"严格数值解析与坏配置事务性", SPLITVIEWER_TEST_INSTANT,
            L"解析整数溢出、后缀、NaN、Inf和溢出指数，并读相应坏配置",
            L"严格拒绝非法值且保留输出与原文档；合法空白数值仍能解析", 20000)},
    {SPLITVIEWER_TEST_CASE_195,
        SplitViewerTestCase(L"UTF16代理对及失败保留输出", SPLITVIEWER_TEST_INSTANT,
            L"独立输入非BMP代理对及孤立代理、奇数字节、错误BOM",
            L"合法16或32位宽字符往返相同，非法编码拒绝且原输出不变", 20000)},
    {SPLITVIEWER_TEST_CASE_196,
        SplitViewerTestCase(L"几何溢出与非法分屏方向", SPLITVIEWER_TEST_INSTANT,
            L"对适配、视图重映射传入非有限及溢出数值和非法分屏方向",
            L"适配返回有效值，溢出不改原视图，非法方向不创建子树", 20000)},
    {SPLITVIEWER_TEST_CASE_197,
        SplitViewerTestCase(L"图片缓存共享引用与回收", SPLITVIEWER_TEST_INSTANT,
            L"多个叶子共用路径后逐步删除引用并清理缓存", L"存在引用的图片保留，无引用项及最后引用删除后缓存回收",
            20000)},
    {SPLITVIEWER_TEST_CASE_198,
        SplitViewerTestCase(L"中文路径二进制限长读取", SPLITVIEWER_TEST_INSTANT,
            L"对中文空格路径文件做恰好限长、超限、缺失及零长读取",
            L"边界成功，超限与缺失失败保留输出，空文件允许零上限", 20000)},
    {SPLITVIEWER_TEST_CASE_199,
        SplitViewerTestCase(L"PNG配置原位封装与长度边界", SPLITVIEWER_TEST_INSTANT,
            L"原位封装提取PNG，另试坏PNG、超大配置和非零IEND", L"原位配置字节一致，所有非法输入拒绝且旧输出不变",
            20000)},
    {SPLITVIEWER_TEST_CASE_200,
        SplitViewerTestCase(L"文件框返回时删除目标图层", SPLITVIEWER_TEST_INSTANT,
            L"加号打开文件选择后、返回路径前删除目标浮动层", L"整个目标层被删除，返回路径不能写入旧节点或背景板原蓝图",
            20000)},
    {SPLITVIEWER_TEST_CASE_201,
        SplitViewerTestCase(L"配置保存树深度与图层上限", SPLITVIEWER_TEST_INSTANT,
            L"通过底层公开模型构造超深树或257图层，再序列化", L"超限保存失败且保留哨兵输出，正常文档仍可保存", 20000)},
    {SPLITVIEWER_TEST_CASE_202,
        SplitViewerTestCase(L"Test主界面异步批次与关闭收尾", SPLITVIEWER_TEST_INSTANT,
            L"实际Test选择194并启动、重复点击及请求关闭",
            L"只启动一个批次，输入在途禁用，输出为QtControls；关闭等待子进程安全收尾", 20000)},
    {SPLITVIEWER_TEST_CASE_203,
        SplitViewerTestCase(L"四方向和嵌套固定目标", SPLITVIEWER_TEST_INSTANT, L"对上下左右及嵌套分屏查询方向目标",
            L"右侧多候选固定最上，下面多候选固定最左，其余按同样顺序", 20000)},
    {SPLITVIEWER_TEST_CASE_204,
        SplitViewerTestCase(L"无效及跨图层迁移保持状态", SPLITVIEWER_TEST_INSTANT,
            L"以空源、空目标、跨树目标或非法目标执行迁移", L"拒绝并保留所有原内容和结构", 20000)},
    {SPLITVIEWER_TEST_CASE_205,
        SplitViewerTestCase(L"图片和视频内容迁移及源清空", SPLITVIEWER_TEST_INSTANT,
            L"将带变换的图片或视频内容移动到同树目标", L"内容与路径及变换随迁移，源清空且树结构不变", 20000)},
    {SPLITVIEWER_TEST_CASE_206,
        SplitViewerTestCase(L"视频路径字段及旧图片兼容", SPLITVIEWER_TEST_INSTANT,
            L"序列化带视频的文档并读入旧纯图片配置", L"新视频路径和类型保存恢复，旧图片字段保持兼容", 20000)},
    {SPLITVIEWER_TEST_CASE_207,
        SplitViewerTestCase(L"四方向菜单移动与源加号", SPLITVIEWER_TEST_INSTANT, L"通过真实右键菜单逐一执行四方向移动",
            L"目标出现原内容，源为灰色加号；空源和无目标动作禁用", 20000)},
    {SPLITVIEWER_TEST_CASE_208,
        SplitViewerTestCase(L"覆盖确认取消及接受", SPLITVIEWER_TEST_INSTANT,
            L"目标已有内容时移动，分别取消和接受覆盖弹窗", L"取消不改双方，接受替换固定目标并清空源", 20000)},
    {SPLITVIEWER_TEST_CASE_209,
        SplitViewerTestCase(L"浮动图层范围隔离", SPLITVIEWER_TEST_INSTANT, L"浮动层含多个分屏时执行移动并检查基础层",
            L"只在当前层树内移动，基础层和其他浮动层不串改", 20000)},
    {SPLITVIEWER_TEST_CASE_210,
        SplitViewerTestCase(L"环境变量筛选及失败保留图片", SPLITVIEWER_TEST_INSTANT,
            L"检查未设置、空、失效及有效LUMAPLAYER环境并尝试坏视频",
            L"仅有效文件增加视频筛选，加载失败保留原图片并报告错误", 20000)},
    {SPLITVIEWER_TEST_CASE_211,
        SplitViewerTestCase(L"真实视频加载移动及全屏", SPLITVIEWER_TEST_INSTANT,
            L"加载真实AVI并移动到另一分屏，再进入退出全屏",
            L"窗口PID保持，画面及逐帧编号持续变化，源清空且客户区尺寸正确", 20000)},
    {SPLITVIEWER_TEST_CASE_212,
        SplitViewerTestCase(L"视频sv保存并重启恢复", SPLITVIEWER_TEST_INSTANT,
            L"保存带视频路径的.sv后关闭并在新窗口恢复", L"独立落盘字段正确，新播放器载入原视频并嵌入相应叶子", 20000)},
    {SPLITVIEWER_TEST_CASE_213,
        SplitViewerTestCase(L"关闭策略及多开进程隔离", SPLITVIEWER_TEST_INSTANT,
            L"默认及扩展关闭集合下移除嵌入窗口，并保留另一同名进程",
            L"只关闭所属实例，集合外窗口恢复独立；不按名字批量关闭", 20000)},
    {SPLITVIEWER_TEST_CASE_214,
        SplitViewerTestCase(L"在途启动删除与替换收尾", SPLITVIEWER_TEST_INSTANT,
            L"播放器启动在途时删除目标、替换内容或关闭宿主",
            L"过期回调不访问旧节点，临时进程与容器回收，当前内容不串改", 20000)},
    {SPLITVIEWER_TEST_CASE_215,
        SplitViewerTestCase(L"无窗口启动超时与原内容保留", SPLITVIEWER_TEST_INSTANT,
            L"用不会生成窗口的程序触发视频窗口等待超时", L"明确报告启动失败，原图片仍可见且相关在途资源回收", 20000)},
    {SPLITVIEWER_TEST_CASE_216,
        SplitViewerTestCase(L"恢复失败保留路径并允许加号重试", SPLITVIEWER_TEST_INSTANT,
            L"在失效环境或视频路径下读.sv，随后重试加载", L"失败路径仍持久化，其他内容可用，空区加号允许重试", 20000)},
    {SPLITVIEWER_TEST_CASE_217,
        SplitViewerTestCase(L"Core配置快照及无效配置隔离", SPLITVIEWER_TEST_INSTANT,
            L"两个Core使用0.4与0.6快照，并输入NaN、零周期及2字节预算",
            L"实例独立，无效初始化及超限封装提取拒绝且保留输出；停止后可受控重启", 20000)},
    {SPLITVIEWER_TEST_CASE_218,
        SplitViewerTestCase(L"用例登记与数字范围解析", SPLITVIEWER_TEST_INSTANT,
            L"校验目录、解析选择及非法编号，核验缺项、重复、取消和溢出汇总",
            L"选择升序去重且非法输入保留输出；只有实际数量完整且零失败的唯一汇总通过", 20000)},
    {SPLITVIEWER_TEST_CASE_219,
        SplitViewerTestCase(L"弹窗会话重复初始化和注册回滚", SPLITVIEWER_TEST_INSTANT,
            L"已有弹窗会话重复init，另一会话重复init和uninit，再实际打开关于框",
            L"本会话重复成功，竞争会话拒绝且不销毁原工厂，真实关于按钮仍可用", 20000)},
    {SPLITVIEWER_TEST_CASE_220,
        SplitViewerTestCase(L"独立Core文档反复分屏恢复压力", SPLITVIEWER_TEST_PRESSURE,
            L"显式选择后重复100轮独立Core创建、分割及停止重启",
            L"各实例根和内容互不共享，每轮终态正确；本轮默认回归不自动执行", 60000)},
    {SPLITVIEWER_TEST_CASE_221,
        SplitViewerTestCase(L"窗口任务关闭与多实例隔离", SPLITVIEWER_TEST_INSTANT,
            L"两个独立TaskManager提交任务，一个立即重复关闭，另一个正常完成",
            L"关闭后拒绝提交且不回调销毁宿主，另一实例只交付一次结果，两个窗口队列收敛", 10000)},
    {SPLITVIEWER_TEST_CASE_222,
        SplitViewerTestCase(L"场景快照独占与失败事务", SPLITVIEWER_TEST_INSTANT,
            L"复制场景后修改副本，再复制停止的源或节点配置更宽松但超文档预算的树",
            L"树与字符串独占且身份映射正确；停止或超文档预算的复制失败，目的文档与映射保持", 10000)},
    {SPLITVIEWER_TEST_CASE_223,
        SplitViewerTestCase(L"异步解码失败保留原图片", SPLITVIEWER_TEST_INSTANT, L"先真实加载红图，再异步解码损坏PNG",
            L"解码失败可观察，旧红图及路径保持而非显示空白", 10000)},
    {SPLITVIEWER_TEST_CASE_224,
        SplitViewerTestCase(L"解码在途关闭主窗口", SPLITVIEWER_TEST_INSTANT, L"实际图片解码提交后立即关闭主窗口",
            L"关闭期间事件循环继续，队列收敛后窗口消失且无过期错误弹窗", 10000)},
    {SPLITVIEWER_TEST_CASE_225,
        SplitViewerTestCase(L"嵌入视频自行改位后保持分屏", SPLITVIEWER_TEST_INSTANT,
            L"空窗口加载自产AVI，分别在基础层、移动到右侧后和全屏时把播放器改为偏移123、87及198乘126，再静置",
            L"不触发布局重绘也自动回到宿主客户区原点并填满；真实四象限画面可见且持续播放，关闭只回收本次播放器",
            25000)},
    {SPLITVIEWER_TEST_CASE_226,
        SplitViewerTestCase(L"播放器内部关闭后进程退出", SPLITVIEWER_TEST_INSTANT,
            L"从加号加载真实LumaPlayer及自产AVI，发送播放器自己的WM_CLOSE，宿主保持打开",
            L"本次播放器的稳定进程句柄进入退出状态，宿主仍可加载图片，不仅检查视频窗口消失", 20000)},
    {SPLITVIEWER_TEST_CASE_227,
        SplitViewerTestCase(L"嵌入窗口销毁后回收残留进程", SPLITVIEWER_TEST_INSTANT,
            L"配置仅Test自身进程名可关闭，联动加载测试子进程；子窗口关闭时销毁HWND但事件循环不退出",
            L"先观察HWND销毁且进程仍活着，再由宿主自动回收该稳定身份，宿主继续使用；其他实例不受影响", 15000)},
    {SPLITVIEWER_TEST_CASE_228,
        SplitViewerTestCase(L"窗口先销毁再关闭宿主不残留进程", SPLITVIEWER_TEST_INSTANT,
            L"联动加载窗口销毁后仍存活的Test子进程，窗口销毁后直接关闭宿主",
            L"宿主收尾仍记得已嵌入的进程身份，子进程退出后宿主关闭，没有隐藏残留进程", 15000)}};

bool SplitViewerTestCaseHelper::validate()
{
    if (kCases.size() != 135)
    {
        return false;
    }
    for (const auto& item : kCases)
    {
        if ((item.first > SPLITVIEWER_TEST_CASE_7 && item.first < SPLITVIEWER_TEST_CASE_101) ||
            item.first < SPLITVIEWER_TEST_CASE_1 || item.first > SPLITVIEWER_TEST_CASE_228 ||
            item.second.m_name.empty() || item.second.m_scene.empty() || item.second.m_expected.empty() ||
            item.second.m_timeoutMs <= 0 ||
            (item.second.m_type != SPLITVIEWER_TEST_INSTANT && item.second.m_type != SPLITVIEWER_TEST_PRESSURE))
        {
            return false;
        }
    }
    return true;
}

bool SplitViewerTestCaseHelper::parse(const std::wstring& selection, std::vector<int32_t>& ids, std::wstring& error)
{
    std::set<int32_t> selected;
    if (selection.empty())
    {
        error = L"用例选择不能为空";
        return false;
    }
    size_t start = 0;
    while (start <= selection.size())
    {
        const size_t comma = selection.find(L',', start);
        const std::wstring item = selection.substr(start, comma == std::wstring::npos ? comma : comma - start);
        const size_t dash = item.find(L'-');
        const int32_t first = CStringManager::parseInt32(item.substr(0, dash), -1);
        const int32_t last = dash == std::wstring::npos ? first : CStringManager::parseInt32(item.substr(dash + 1), -1);
        if (first < 1 || last > 228 || first > last ||
            (dash != std::wstring::npos && item.find(L'-', dash + 1) != std::wstring::npos))
        {
            error = L"用例编号或范围无效";
            return false;
        }
        for (int32_t id = first; id <= last; ++id)
        {
            if (kCases.find(static_cast<SplitViewerTestCaseId>(id)) == kCases.end())
            {
                error = L"用例不存在或已删除";
                return false;
            }
            selected.insert(id);
        }
        if (comma == std::wstring::npos)
        {
            break;
        }
        start = comma + 1;
    }
    std::vector<int32_t> parsed(selected.begin(), selected.end());
    ids.swap(parsed);
    error.clear();
    return true;
}

std::vector<int32_t> SplitViewerTestCaseHelper::byType(SplitViewerTestCaseType type)
{
    std::vector<int32_t> ids;
    for (const auto& item : kCases)
    {
        if (item.second.m_type == type)
        {
            ids.push_back(static_cast<int32_t>(item.first));
        }
    }
    return ids;
}

bool SplitViewerTestCaseHelper::reportSucceeded(const std::string& report, int32_t expectedCount)
{
    if (expectedCount <= 0 || report.find('\0') != std::string::npos)
    {
        return false;
    }
    const size_t end = report.find_last_not_of("\r\n");
    if (end == std::string::npos)
    {
        return false;
    }
    const size_t newline = report.rfind('\n', end);
    const size_t start = newline == std::string::npos ? 0 : newline + 1;
    const std::string summary = report.substr(start, end - start + 1);
    const std::string kTotal = "total=";
    const std::string kFailures = " failures=";
    if (summary.compare(0, kTotal.size(), kTotal) != 0 || report.find(kTotal) != start)
    {
        return false;
    }
    const size_t separator = summary.find(kFailures);
    if (separator == std::string::npos || separator <= kTotal.size())
    {
        return false;
    }
    const std::string total = summary.substr(kTotal.size(), separator - kTotal.size());
    const std::string failures = summary.substr(separator + kFailures.size());
    if (total.empty() || failures.empty() || total.find_first_not_of("0123456789") != std::string::npos ||
        failures.find_first_not_of("0123456789") != std::string::npos)
    {
        return false;
    }
    return CStringManager::parseInt32(std::wstring(total.begin(), total.end()), -1) == expectedCount &&
        CStringManager::parseInt32(std::wstring(failures.begin(), failures.end()), -1) == 0;
}