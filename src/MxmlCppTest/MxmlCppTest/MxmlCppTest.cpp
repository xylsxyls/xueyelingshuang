#include "MxmlCppTest.h"
#include "MxmlCpp/MxmlCppAPI.h"
#include "PlatformTest/PlatformTestAPI.h"
#include <cstdio>
#include <cstring>
#include <stdexcept>

#define XML_CHECK(expression) do { if (!(expression)) { std::fprintf(stderr, \
    "MxmlCppTest line %d: %s\n", __LINE__, #expression); return 1; } } while (false)

/** 从空文档建立新树，独立验证写入和游标边界。
@return 通过返回0
*/
static int BuildTree()
{
    MxmlCpp xml;
    XML_CHECK(xml.element().empty() && xml.text().empty() && xml.attr("x").empty() && xml.toString().empty());
    XML_CHECK(!xml.parent() && !xml.next() && !xml.pre() && !xml.firstChild() && !xml.addChild("root"));
    XML_CHECK(xml.create() && !xml.addAttr("key", "value") && xml.toString().empty());
    XML_CHECK(xml.addChild("root") && !xml.addChild("secondRoot") && xml.firstChild());
    XML_CHECK(xml.element() == "root" && xml.addAttr("data", "") && xml.attr("data").empty());
    XML_CHECK(xml.addAttr("data", " changed ") && xml.attr("data") == " changed ");
    XML_CHECK(!xml.addChild("1bad") && !xml.addChild("bad name") && !xml.addChild("bad/") &&
        !xml.addAttr("", "") && !xml.addChild("", "x"));
    XML_CHECK(xml.addChild("\xe5\x90\x8d", "  a & b  ") && xml.addChild("empty", ""));
    XML_CHECK(xml.firstChild() && xml.element() == "\xe5\x90\x8d" && xml.text() == "  a & b  ");
    XML_CHECK(!xml.pre() && xml.next() && xml.element() == "empty" && xml.text().empty());
    XML_CHECK(!xml.next() && xml.pre() && xml.parent() && xml.element() == "root");
    XML_CHECK(xml.parent() && !xml.parent() && xml.element().empty());
    XML_CHECK(xml.create() && !xml.firstChild() && xml.toString().empty());
    return 0;
}

/** 读取独立编写的 XML 样本，核对筛选范围、字符解码和混合内容。
@return 通过返回0
*/
static int ReadTree()
{
    MxmlCpp xml;
    const std::string source = "\xef\xbb\xbf<?xml version=\"1.0\"?>\n<!--prolog--><r a=\"&quot;&amp;\">"
        " A\t<![CDATA[<literal>]]><branch><item id=\"9\"/></branch>\n"
        "<!--skip--><item id=\"\">&#65;&lt;&gt;</item><item id=\"2\"/> end </r>";
    XML_CHECK(xml.load(source) && xml.element().empty() && xml.firstChild("r"));
    const std::string roundTrip = xml.toString();
    XML_CHECK(roundTrip.find("<!--prolog-->") != std::string::npos &&
        roundTrip.find("<![CDATA[<literal>]]>") != std::string::npos);
    XML_CHECK(xml.attr("a") == "\"&" && xml.text() == " A\t<literal>\n end ");
    XML_CHECK(!xml.firstChild("", "", "9") && !xml.firstChild("item", "id", "9") && xml.element() == "r");
    XML_CHECK(xml.firstChild("", "id") && xml.attr("id").empty() && xml.text() == "A<>");
    XML_CHECK(xml.pre() && xml.element() == "branch" && xml.next() && xml.element() == "item");
    XML_CHECK(xml.next() && xml.attr("id") == "2" && !xml.next() && xml.parent());
    XML_CHECK(xml.firstChild("item", "id", "2") && !xml.firstChild());
    XML_CHECK(xml.load("<bare/>") && xml.element() == "bare" && xml.parent() && !xml.parent());
    return 0;
}

/** 解析或写入非法输入后校验此前成功的整棵树及游标。
@return 通过返回0
*/
static int RejectInvalid()
{
    MxmlCpp xml;
    XML_CHECK(xml.load("<r><keep key=\"v\"/></r>") && xml.firstChild());
    const std::string original = xml.toString();
    const std::string bad[] = {"", " ", "text", "<r>", "<r><a></r>", "<a/><b/>", "<?xml version=\"1.0\"?>",
        "garbage<r/>", "<r/>garbage", "<r>&notAnEntity;</r>", "<r>\xc0\x80</r>", std::string("<r/>\0junk", 9)};
    for (size_t i = 0; i < sizeof(bad) / sizeof(bad[0]); ++i)
    {
        XML_CHECK(!xml.load(bad[i]) && xml.toString() == original && xml.element() == "keep");
    }
    XML_CHECK(!xml.addChild("a", std::string("\0x", 2)) && !xml.addAttr("k", "\x01") &&
        !xml.addChild(std::string("a\0b", 3)) && !xml.addChild("\xed\xa0\x80"));
    XML_CHECK(!xml.firstChild("", std::string("key\0z", 5)) && xml.attr(std::string("key\0z", 5)).empty());
    XML_CHECK(xml.attr("key") == "v" && xml.toString() == original);
    return 0;
}

/** 反复替换已有树，并在大型序列化缓冲边界检验字符及属性。
@return 通过返回0
*/
static int LargeRoundTrip()
{
    std::string data;
    for (size_t i = 0; i < 4096; ++i)
    {
        data.append("x&<> ");
    }
    MxmlCpp xml;
    MxmlCpp reader;
    XML_CHECK(xml.create() && xml.addChild("long", data) && xml.firstChild() && xml.addAttr("description", data));
    const std::string output = xml.toString();
    XML_CHECK(output.size() > data.size() * 2 && output.find("x&amp;&lt;&gt; ") != std::string::npos);
    for (int i = 0; i < 100; ++i)
    {
        XML_CHECK(reader.load(output) && reader.firstChild("long") && reader.text() == data &&
            reader.attr("description") == data);
        XML_CHECK(reader.load("<small/>") && reader.element() == "small" && reader.attr("description").empty());
    }
    std::string deep;
    for (int i = 0; i < 2048; ++i)
    {
        deep += "<n>";
    }
    deep += "bottom";
    for (int i = 0; i < 2048; ++i)
    {
        deep += "</n>";
    }
    XML_CHECK(reader.load(deep) && reader.element() == "n");
    for (int i = 1; i < 2048; ++i)
    {
        XML_CHECK(reader.firstChild("n"));
    }
    XML_CHECK(!reader.firstChild() && reader.text() == "bottom");
    for (int i = 0; i < 2048; ++i)
    {
        XML_CHECK(reader.parent());
    }
    XML_CHECK(!reader.parent() && reader.toString().find(deep) != std::string::npos);
    return 0;
}

/** 对同一棵宽树分别测量解析、保存和尾部查找。
@return 功能核验通过返回0
*/
static int Benchmark()
{
    std::string source("<root>");
    for (int i = 0; i < 64; ++i)
    {
        source += "<item id=\"" + std::to_string(i) + "\">sample &amp; text</item>";
    }
    source += "</root>";
    MxmlCpp xml;
    XML_CHECK(xml.load(source));
    const std::string saved = xml.toString();
    XML_CHECK(!saved.empty() && xml.firstChild("item", "id", "63") && xml.text() == "sample & text" && xml.parent());
    std::printf("source_bytes=%u items=64\n", static_cast<unsigned>(source.size()));
    // 批内重复替换同一文档，检查成功次数，包含旧树释放成本。
    LibraryBenchmark::run("xml.load.64items", [&](uint32_t count) -> uint64_t
    {
        uint64_t successes = 0;
        for (uint32_t i = 0; i < count; ++i)
        {
            successes += xml.load(source);
        }
        if (successes != count)
        {
            throw std::runtime_error("load");
        }
        return successes;
    });
    // 累加每次调用实际交付的字符串长度。
    LibraryBenchmark::run("xml.save.64items", [&](uint32_t count) -> uint64_t
    {
        uint64_t bytes = 0;
        for (uint32_t i = 0; i < count; ++i)
        {
            bytes += xml.toString().size();
        }
        if (bytes != uint64_t(count) * saved.size())
        {
            throw std::runtime_error("save");
        }
        return bytes;
    });
    // 每次查找固定的第64个元素，随后将游标移回同一起点。
    const std::string name("item"), attribute("id"), last("63");
    LibraryBenchmark::run("xml.findLastAndParent.64items", [&](uint32_t count) -> uint64_t
    {
        uint64_t successes = 0;
        for (uint32_t i = 0; i < count; ++i)
        {
            successes += xml.firstChild(name, attribute, last) && xml.parent();
        }
        if (successes != count)
        {
            throw std::runtime_error("lookup");
        }
        return successes;
    });
    return 0;
}

/** 调度可独立运行的功能和性能测试。
@param [in] argc 参数数量
@param [in] argv 参数数组
@return 通过返回0，失败返回1，无效选择返回2
*/
int main(int argc, char* argv[])
{
    if (argc == 2 && std::strcmp(argv[1], "--benchmark") == 0)
    {
        return Benchmark();
    }
    const LibraryTestCase cases[] = {{1, false, BuildTree}, {2, false, ReadTree},
        {3, false, RejectInvalid}, {4, true, LargeRoundTrip}};
    return LibraryCaseSelection::run(argc, argv, cases);
}