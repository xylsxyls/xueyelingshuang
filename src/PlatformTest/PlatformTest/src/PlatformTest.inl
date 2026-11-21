template<class Batch>
void LibraryBenchmark::run(const char* name, const Batch& batch, uint32_t operations)
{
    struct Work
    {
        // 同步借用调用方的批量工作负载，可为函数或带捕获的闭包
        decltype(batch) m_batch;
    };
    Work work = {batch};
    PlatformTest::runBenchmark(name, &work, [](const void* context, uint32_t count) -> uint64_t
    {
        return static_cast<const Work*>(context)->m_batch(count);
    }, operations);
}

namespace LibraryCaseSelection
{
/** 从当前位置读取一个正整数，最大值为本测试程序登记的最大编号
@param [in,out] cursor 当前字符位置，成功后指向首个非数字字符
@param [in] maximum 用例编号上界
@param [out] value 解析结果
@return 编号有效时返回 true
*/
inline bool number(const char*& cursor, unsigned maximum, unsigned& value)
{
    return PlatformTest::caseNumber(cursor, maximum, value);
}

/** 按登记顺序执行，无参数执行所有适用功能用例。无效选择不得触发测试
@param [in] argc 控制台参数数量
@param [in] argv 控制台参数数组
@param [in] cases 按稳定编号递增排列的本平台用例
@param [in] execute 执行一个选中用例，返回零表示通过
@return 0 表示已选用例通过，1 表示用例失败，2 表示参数或可执行范围无效
*/
template <typename Case, size_t Count, typename Execute>
int run(int argc, char** argv, const Case (&cases)[Count], Execute execute)
{
    struct Dispatch
    {
        // 实际登记的用例数组
        const Case* m_cases;
        // 同步借用本次调用的执行器
        Execute& m_execute;
    };
    Dispatch dispatch = {cases, execute};
    return PlatformTest::runCases(argc, argv, Count, &dispatch,
        [](void* context, size_t index, bool& stress) -> unsigned
        {
            const Case& item = static_cast<Dispatch*>(context)->m_cases[index];
            stress = item.stress;
            return item.id;
        },
        [](void* context, size_t index) -> int
        {
            Dispatch& data = *static_cast<Dispatch*>(context);
            return data.m_execute(data.m_cases[index]);
        });
}

/** 按登记的回调执行已选择的功能用例
@param [in] argc 控制台参数数量
@param [in] argv 控制台参数数组
@param [in] cases 当前平台的功能用例与回调
@return 0 表示通过，1 表示断言失败，2 表示选择无效
*/
template <size_t Count>
int run(int argc, char** argv, const LibraryTestCase (&cases)[Count])
{
    return run(argc, argv, cases, [](const LibraryTestCase& item) { return item.run(); });
}

/** 将数字和分类选择送入既有功能入口；也支持该工程原有的参数
@param [in] argc 参数数量
@param [in] argv 命令行参数数组
@param [in] cases 当前平台按 ID 递增登记的功能组
@param [in] suite 已有功能入口，接受单组参数或该工程的扩展选项
@return 0 表示通过，1 表示断言失败，2 表示选择无效
*/
template <size_t Count>
int runNamed(int argc, char** argv, const LibraryNamedTestCase (&cases)[Count], int (*suite)(int, char**))
{
    if (argc < 1 || argv == nullptr || argv[0] == nullptr || suite == nullptr || (argc > 1 && argv[1] == nullptr))
    {
        return 2;
    }
    if (argc < 2 || (std::strcmp(argv[1], "--all") != 0 && std::strcmp(argv[1], "--cases") != 0 &&
        std::strcmp(argv[1], "--instant") != 0 && std::strcmp(argv[1], "--stress") != 0))
    {
        return suite(argc, argv);
    }
    return run(argc, argv, cases, [&](const LibraryNamedTestCase& item)
    {
        std::string selection = std::string("--case=") + item.name;
        char* args[] = {argv[0], &selection[0], nullptr};
        return suite(2, args);
    });
}
}