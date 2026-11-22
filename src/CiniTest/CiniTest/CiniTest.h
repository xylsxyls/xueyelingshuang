#pragma once
#include <string>

/** Cini 当前版本功能测试与性能口径

用例及预期（均为瞬时用例，各自构造配置和文件）
1 内存替换与查询。载入含 CRLF、LF、注释、UTF-8 值、空值与同名跨节键的配置，检查大小写查询和完整名称集合。不存在的分区首键为空。再次载入时旧键消失，空输入清空配置。Profile 容量设置不影响 SimpleIni。
2 文件增删改。创建临时文件，写入并覆盖键，新建对象应读回对应值（包括 2048 字节长值）。删除单键后其他键保留；删除分区后另一分区及其值保留。
3 Windows Profile。系统模式拒绝 loadData；设置四字节容量后 abcdef 读回 abc，扩容后恢复全值。检查默认节的枚举、首键、getAllKey 的空集合，以及删除键和整节后的文件状态，重开后核对另一分区的名称和同名键值。400 字节值使用 401 字节缓冲时完整返回，使用 400 字节缓冲时返回前 399 字节。
4 解析边界。BOM 后首节有效，大小写不同的重复键取最后值，缺右括号的节头被跳过，其后键仍属原节。空分区出现在枚举中，删除后只剩原节。
5 生命周期。复制构造与赋值不可用。在独占临时目录中的缺失路径首次写入，旧、新实例均可读回；删除最后一键仍保留空节，删除该节后磁盘文件存在且为空，新对象枚举为空。提前载入的快照仍返回原值。非 Windows 还核对默认构造参数选择 SimpleIni。
6 Windows 参数边界。仅有一个长分区时，小枚举预算应返回空集合；增加预算后应完整读回长、短两个名称。覆盖负数、一字节、零分区、单分区和两个 INT32_MAX 上限，校验读值、首键及分区数相互独立。
7 非 Windows 的通用 Unicode 转换器白盒测试。通过对应 SDK 的 ConvertUTF.h 和生产库验证四字节字符转为预期代理对并往返、分别从 UTF-8 和 UTF-16 解码到 UTF-32 U+1F600、空区间、截断及非法续字节。输出不足应返回 targetExhausted 且不消费输入；严格模式遇到超范围码点返回 sourceIllegal，并保持输入、输出指针位置。Windows Cini 使用原生转换后端，未链接这组通用 C 符号。
8 字段和路径边界。内嵌零字节不能命中前缀键、分区或文件；包含 INI 结构字符的非法写入不能更改原值和文件中的名称集合。Windows 两个后端各用独立文件，并比较非法操作前后的全部磁盘字节。
9 损坏输入与恢复。内存和文件中的内嵌零字节应使载入失败，禁止使用部分配置覆盖原文件；独立读取原始字节核对未被改写，重新载入合法文本后可以恢复。
10 全局分区与返回副本。先在带等号的 UTF-8 分区名下写入含右方括号的 UTF-8 键，再向全局分区写入同名键。重开时按分区分别读回，值边缘的空格经解析被修剪；删除全局分区后具名分区保留。读值及枚举副本在修改并销毁源对象后仍保存原内容。

构建与运行
分别使用 version_release.sh（需要调试时用 version_debug.sh）准备目标架构的 Cini、PlatformTest，再构建 CiniTest.sln 或 CiniTest/CMakeLists.txt。Windows Release 静态 SDK 使用默认配置。验证 DLL 时使用同配置的 DLL SDK，MSBuild 传 /p:CiniTestLinkage=dll，CMake 传 -DCiniTestLinkage=dll。Wasm 设置 Emscripten 工具链、BIT=wasm32 或 wasm64，并指定该位宽的 SDK_ROOT。Node 可运行 node CiniTest.js --instant。Linux ARM 选择目标交叉工具链；Android 和 iOS 通过 PlatformTest 设备入口部署，临时文件放在宿主提供的可写 TMPDIR。
无参数、--instant 或 --all 运行本平台全部功能用例；--cases 1,2,4-5 指定编号。Windows 登记 1–6、8–10，其余平台登记 1、2、4、5、7–10。--stress 无已登记项。PlatformTest 为每个执行项输出编号和结果，成功退出 0，功能失败退出 1，非法或空选择退出 2。CMake 的功能批次可用 ctest -R "^CiniTest$" 选择。文件用例退出时回收其临时文件。
VS2013 可检查复制构造的类型特征，其 is_copy_assignable 对已删除赋值运算符会误报。该工具链另以实际消费者语句 Cini copy(source) 和 destination = source 分别做负向编译检查，两者均应被拒绝。其他工具链同时检查两个标准类型特征。

2026-11-23 当前源码独立重建结果
Windows x64，VS2013 Release 静态库和 DLL 各通过 1–6、8–10。Windows Win32，VS2013 Debug 静态库通过同样 9 项。
Wasm32、Wasm64，Emscripten 3.1.74 Release，Node 24.0.0，分别从 CTest 功能入口通过 1、2、4、5、7–10。
共 10 个独立编号，5 种配置合计执行 43 项瞬时测试，通过 43，失败 0，取消 0。Linux、Linux ARM、macOS、Android、iOS 仅核查源码、构建和适用的设备入口。Wasm 浏览器宿主未实测。
另行运行 Windows x64、Node Wasm32 和 Wasm64 的 main 示例，均返回 0。VS2013 独立编译复制构造和赋值调用均报 C2248。指定编号成功去重，非法编号和空类型选择返回 2；两个依赖准备脚本按顺序调用，首个依赖失败时传回错误码。
判定自检使用临时测试副本。用例 3 在操作后额外删除应保留的分区，用例 5 在检查前移除应存在的文件，两个用例均准确返回 1。正式批次使用原始测试工程；这些辅助验证单列。

本批产物 SHA-256
Windows x64 Release 静态 1B5FCEF61AF596194635CC7EE33B837652530FE87D682374BDB528A575969A8E
Windows x64 Release 动态 204EBE7552CF9808466734D390E8A42F73BCC8B47E4E1FD26B7168E8D4D69407
对应 Cini.dll 07723C970520F9D57B182F7285F745154F2D3E408AD628B544F07740094649E1
Windows Win32 Debug 静态 8F1880AB4240FADC3E51AFEBB138D6CC31E6A269F5AB6D02ED6A5D37A31A6B56
Wasm32 模块 034C5566DC5B623223628C7922C34576C5F1A4EC6D0A45162E951F60B5BBDE66
Wasm64 模块 6D4CDC331DC32A4A08A7FF02331BFB6F4BBCAAEBA82DCFEEBF913E1050F8A20B

性能口径
当前构建缺少性能测试数据。本次仅做功能验证，没有执行 --benchmark。Windows、Linux、Linux ARM、macOS、Android、iOS、Wasm 均无可归属于本次代码的延迟、吞吐或波动区间。
CiniTest 的独立基准包含两个场景。第一个在同一对象上反复 loadData，输入为一个分区、64 个键、每个值均为 payload，包含清理上次解析结果的成本。第二个从已载入配置以固定键 k63 调用 readIni，包含查找、返回字符串副本和消费返回长度的成本。输入构造、文件读写、进程启动和首次预热在计时外。解析批次结束后在计时外核对 64 个键及全部值，再开始读取基准。
复测时按平台、位宽、编译器、Release 配置、链接方式、CPU 和宿主版本分别记录。固定输入、CPU 亲和性及电源模式，关闭其他高负载任务；Node 保持相同 JIT 参数。PlatformTest 预热后以二倍步长校准批量，两次确认批耗时达到约 20 ms，最多 2^26 次。每进程取九批的单次耗时中位数、Q1、Q3 和原始值。需跨进程比较时先预热一次，再独立运行五次，报告五个中位数的中位数、四分位数和范围。遇到宽分布应记录环境及离散程度；本次没有验证重复运行的波动幅度。
两个容量设置仅赋一个整数，无需单列性能数值。构造、枚举、文件写删和 Profile 查询依赖配置规模、路径及缓存状态，缺少分场景性能测试数据。
*/

/** 当前功能用例的稳定编号，各端只登记适用项
*/
enum CiniCaseId
{
    // 内存载入、替换与枚举
    kReadMemory = 1,
    // SimpleIni 文件增删改查
    kFileChanges = 2,
    // Windows 系统 Profile 文件操作
    kProfileFile = 3,
    // 解析边界与空分区
    kParserBoundaries = 4,
    // 首次创建文件与独立快照
    kFileLifecycle = 5,
    // Windows 系统缓冲区边界
    kProfileLimits = 6,
    // 通用 Unicode 转换器的区间及恢复位置
    kConversionBounds = 7,
    // 不可表示的名称、值和路径
    kFieldBoundaries = 8,
    // 损坏输入后的保护和重新载入
    kDamagedInput = 9,
    // 全局键、非 ASCII 名称及返回副本的寿命
    kGlobalSectionAndCopies = 10
};

/** 独占测试创建的路径，退出作用域时回收文件
*/
struct IniFile
{
public:
    // 系统分配的文件路径，创建失败时为空
    std::string m_path;

    /** 创建独占临时目录，供单个用例操作文件
    @param [in] createFile 为 true 时预建空文件，false 保留缺失路径
    */
    explicit IniFile(bool createFile = true);

    /** 回收本次测试文件及独占目录
    */
    ~IniFile();

private:
    /** 禁止复制文件归属
    @param [in] other 另一份临时文件
    */
    IniFile(const IniFile& other) = delete;

    /** 禁止覆盖文件归属
    @param [in] other 另一份临时文件
    @return 当前对象引用
    */
    IniFile& operator=(const IniFile& other) = delete;

private:
    // 只在本实例排他创建目录成功后设置
    std::string m_directory;
};