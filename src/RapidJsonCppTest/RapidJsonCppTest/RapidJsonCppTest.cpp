#include "RapidJsonCppTest.h"
#include "RapidJsonCpp/RapidJsonCppAPI.h"
#include "PlatformTest/PlatformTestAPI.h"
#include "rapidjson/document.h"
#include <cstdio>
#include <cstring>
#include <limits>
#include <stdexcept>
#include <utility>
#ifndef __EMSCRIPTEN__
#include <atomic>
#include <sstream>
#include <thread>
#endif

/** 执行一组独立断言并输出失败身份 */
struct Checks
{
    // 当前组失败次数
    unsigned failures;
    /** 核对独立预期，失败时记录用例 */
    void operator()(bool ok, const char* label)
    {
        if (!ok)
        {
            ++failures;
            std::fprintf(stderr, "FAIL %s\n", label);
        }
    }
};

/** 文档、值、赋值和类型边界，失败返回1 */
static int ApiCases()
{
    Checks check = {};
    RapidJsonDocument document;
    check(document.isObject(), "RapidJsonDocument default object");
    check(!document.parse(""), "RapidJsonDocument empty parse failed");
    check(!document.parse("{bad json"), "RapidJsonDocument invalid parse failed");
    check(document.parse("{\"name\":\"cook\",\"count\":3,\"ok\":true,\"tags\":[\"a\",\"b\"]}"), "RapidJsonDocument parse object");
    check(document.getStringOrDefault("name", "") == "cook", "RapidJsonDocument get string");
    check(document.getIntOrDefault("count", 0) == 3, "RapidJsonDocument get int");
    check(document.getBoolOrDefault("ok", false), "RapidJsonDocument get bool");
    check(document.getStringArrayOrEmpty("tags").size() == 2, "RapidJsonDocument get string array");
    check(document.getValue("tags").isArray(), "RapidJsonDocument get value array");
    document.addString("name", "chef");
    document.addInt("count", 5);
    document.addBool("ok", false);
    document.addStringArray("tags", std::vector<std::string>{"x", "y", "z"});
    document.addString("literal", "char value");
    document.addString("emptyChar", static_cast<const char*>(nullptr));
    RapidJsonDocument verify;
    check(verify.parse(document.toString()), "RapidJsonDocument serialize parse");
    check(verify.getStringOrDefault("name", "") == "chef", "RapidJsonDocument replace string");
    check(verify.getIntOrDefault("count", 0) == 5, "RapidJsonDocument replace int");
    check(!verify.getBoolOrDefault("ok", true), "RapidJsonDocument replace bool");
    check(verify.getStringArrayOrEmpty("tags").size() == 3, "RapidJsonDocument replace array");
    check(verify.getStringOrDefault("literal", "") == "char value", "RapidJsonDocument add char string");
    check(verify.getStringOrDefault("emptyChar", "default") == "", "RapidJsonDocument add null char string");
    check(verify.getArrayValueOrEmpty("tags").size() == 3, "RapidJsonDocument get value array size");
    check(!verify.parse(""), "RapidJsonDocument empty parse clears old data");
    check(verify.getStringOrDefault("name", "default") == "default", "RapidJsonDocument empty parse no stale string");
    check(!verify.parse("{bad json"), "RapidJsonDocument invalid parse clears old data");
    check(verify.getIntOrDefault("count", 99) == 99, "RapidJsonDocument invalid parse no stale int");
    check(verify.parse("[1,2,3]") && verify.isArray(), "RapidJsonDocument parse root array");
    check(verify.getBoolOrDefault("ok", true), "RapidJsonDocument root array getter default");
    std::string dirtyJson("{\"name\":\"ok\"}");
    dirtyJson.push_back('\0');
    dirtyJson += "broken";
    check(!verify.parse(dirtyJson), "RapidJsonDocument dirty text after zero fails");
    check(verify.getStringOrDefault("name", "default") == "default", "RapidJsonDocument dirty text clears old data");

    RapidJsonDocument nullKeyDocument;
    nullKeyDocument.addString(nullptr, "bad");
    nullKeyDocument.addInt(nullptr, 3);
    nullKeyDocument.addBool(nullptr, true);
    check(nullKeyDocument.toString() == "{}", "RapidJsonDocument null key ignored");

    std::string binaryText("ab", 2);
    binaryText.push_back('\0');
    binaryText += "cd";
    RapidJsonDocument binaryDocument;
    binaryDocument.addString("binary", binaryText);
    RapidJsonDocument binaryParsed;
    check(binaryParsed.parse(binaryDocument.toString()), "RapidJsonDocument embedded zero parse");
    check(binaryParsed.getStringOrDefault("binary", "") == binaryText, "RapidJsonDocument embedded zero keep length");

    RapidJsonValue child;
    child.setObject();
    child.addString("childName", "small");
    child.addInt("childCount", 2);
    child.addBool("childOk", true);
    child.addStringArray("childTags", std::vector<std::string>{"m", "n"});
    RapidJsonDocument parent;
    parent.addValue("child", child);
    check(parent.toString().find("childName") != std::string::npos, "RapidJsonDocument add value");

    RapidJsonValue arrayValue;
    arrayValue.setArray();
    arrayValue.reserve(4);
    arrayValue.pushString("one");
    arrayValue.pushInt(2);
    arrayValue.pushBool(true);
    arrayValue.pushValue(child);
    check(arrayValue.isArray() && arrayValue.size() == 4, "RapidJsonValue array push");
    check(arrayValue.toString().find("childName") != std::string::npos, "RapidJsonValue nested serialize");
    RapidJsonValue childRead = parent.getValue("child");
    check(childRead.getStringOrDefault("childName", "") == "small", "RapidJsonValue get nested string");
    check(childRead.getIntOrDefault("childCount", 0) == 2, "RapidJsonValue get nested int");
    check(childRead.getBoolOrDefault("childOk", false), "RapidJsonValue get nested bool");
    check(childRead.getStringArrayOrEmpty("childTags").size() == 2, "RapidJsonValue get nested string array");
    check(childRead.getArrayValueOrEmpty("childTags").size() == 2, "RapidJsonValue get nested value array");

    RapidJsonValue nullKeyValue;
    nullKeyValue.addString(nullptr, "bad");
    nullKeyValue.addInt(nullptr, 1);
    nullKeyValue.addBool(nullptr, true);
    check(nullKeyValue.toString() == "{}", "RapidJsonValue null key ignored");
    RapidJsonValue scalarValue;
    scalarValue.setString(binaryText);
    check(scalarValue.toString().find("\\u0000") != std::string::npos, "RapidJsonValue string embedded zero serialize");
    scalarValue.setString("char scalar");
    check(scalarValue.toString() == "\"char scalar\"", "RapidJsonValue set char string root");
    scalarValue.setInt(42);
    check(scalarValue.toString() == "42", "RapidJsonValue set int root");
    scalarValue.setBool(true);
    check(scalarValue.toString() == "true", "RapidJsonValue set bool root");
    scalarValue.addString("autoObject", "yes");
    scalarValue.addString("emptyChar", static_cast<const char*>(nullptr));
    check(scalarValue.isObject() && scalarValue.toString().find("autoObject") != std::string::npos, "RapidJsonValue add converts to object");
    check(scalarValue.toString().find("\"emptyChar\":\"\"") != std::string::npos, "RapidJsonValue add null char string");
    scalarValue.setString("not array");
    scalarValue.pushString("char item");
    scalarValue.pushInt(8);
    check(scalarValue.isArray() && scalarValue.size() == 2, "RapidJsonValue push converts to array");

    RapidJsonValue copyValue = child;
    check(copyValue.isObject() && copyValue.size() == child.size(), "RapidJsonValue copy construct");
    copyValue = copyValue;
    check(copyValue.isObject() && copyValue.size() == child.size(), "RapidJsonValue self assign");
    RapidJsonValue assignedValue;
    assignedValue = child;
    check(assignedValue.isObject() && assignedValue.size() == child.size(), "RapidJsonValue copy assign");
    RapidJsonValue moveValueSource;
    moveValueSource.addString("name", "move");
    RapidJsonValue moveValueTarget(std::move(moveValueSource));
    check(moveValueTarget.toString().find("\"name\":\"move\"") != std::string::npos, "RapidJsonValue move construct");
    moveValueSource.addString("afterMove", "source");
    check(moveValueSource.toString().find("afterMove") != std::string::npos, "RapidJsonValue moved source usable");
    RapidJsonValue moveValueAssign;
    moveValueAssign = std::move(moveValueTarget);
    check(moveValueAssign.toString().find("\"name\":\"move\"") != std::string::npos, "RapidJsonValue move assign");
    moveValueTarget.addString("afterAssign", "target");
    check(moveValueTarget.toString().find("afterAssign") != std::string::npos, "RapidJsonValue move assigned source usable");
    RapidJsonDocument copyDocument = parent;
    check(copyDocument.toString() == parent.toString(), "RapidJsonDocument copy construct");
    copyDocument = copyDocument;
    check(copyDocument.toString() == parent.toString(), "RapidJsonDocument self assign");
    RapidJsonDocument moveSource;
    moveSource.addString("name", "move");
    RapidJsonDocument moveTarget(std::move(moveSource));
    check(moveTarget.getStringOrDefault("name", "") == "move", "RapidJsonDocument move construct");
    moveSource.addString("afterMove", "source");
    check(moveSource.getStringOrDefault("afterMove", "") == "source", "RapidJsonDocument moved source usable");
    RapidJsonDocument moveAssign;
    moveAssign = std::move(moveTarget);
    check(moveAssign.getStringOrDefault("name", "") == "move", "RapidJsonDocument move assign");
    moveTarget.addString("afterAssign", "target");
    check(moveTarget.getStringOrDefault("afterAssign", "") == "target", "RapidJsonDocument move assigned source usable");

    RapidJsonCpp legacy;
    legacy["id"] = 7;
    legacy["title"] = "legacy";
    RapidJsonDocument legacyDocument;
    check(legacyDocument.parse(legacy.toString()), "RapidJsonCpp legacy serialize parse");
    check(legacyDocument.getIntOrDefault("id", 0) == 7, "RapidJsonCpp legacy get int");
    check(legacyDocument.getStringOrDefault("title", "") == "legacy", "RapidJsonCpp legacy get string");
    RapidJsonCpp legacyEmptyKey;
    legacyEmptyKey[""] = "empty";
    RapidJsonDocument legacyEmptyKeyDocument;
    check(legacyEmptyKeyDocument.parse(legacyEmptyKey.toString()), "RapidJsonCpp legacy empty key parse");
    check(legacyEmptyKeyDocument.getStringOrDefault("", "missing") == "empty", "RapidJsonCpp legacy empty key get string");
    check(legacy["missing"].isNull(), "RapidJsonCpp legacy missing key null value");
    check(legacy["id"].isInt() && legacy.toIntOrDefault(-1) == 7, "RapidJsonCpp legacy key cache update");
    check(legacy.parse("[{\"v\":1},{\"v\":2}]"), "RapidJsonCpp legacy parse root array");
    check(!legacy.hasMember("v"), "RapidJsonCpp legacy hasMember root array false");
    check(legacy.toArray().size() == 2, "RapidJsonCpp legacy root array toArray");
    check(!legacy.parse(dirtyJson), "RapidJsonCpp legacy dirty text after zero fails");
    check(!legacy.hasMember("name"), "RapidJsonCpp legacy dirty text clears old data");
    RapidJsonCpp legacyRoot;
    legacyRoot = 9;
    check(legacyRoot.toString() == "9", "RapidJsonCpp legacy root int assign");
    legacyRoot = std::string("root");
    check(legacyRoot.toString() == "\"root\"", "RapidJsonCpp legacy root string assign");
    RapidJsonCpp legacyItemA;
    legacyItemA["v"] = 1;
    RapidJsonCpp legacyItemB;
    legacyItemB["v"] = 2;
    std::vector<RapidJsonCpp> legacyArray;
    legacyArray.push_back(legacyItemA);
    legacyArray.push_back(legacyItemB);
    RapidJsonCpp legacyArrayRoot;
    legacyArrayRoot = legacyArray;
    check(legacyArrayRoot.toArray().size() == 2, "RapidJsonCpp legacy root array assign");
    RapidJsonCpp legacyKeyClear;
    legacyKeyClear["items"] = legacyArray;
    legacyKeyClear["title"] = std::string("after array");
    RapidJsonDocument legacyKeyClearDocument;
    check(legacyKeyClearDocument.parse(legacyKeyClear.toString()), "RapidJsonCpp legacy key clear parse");
    check(legacyKeyClearDocument.getStringOrDefault("title", "") == "after array", "RapidJsonCpp legacy key clear after array assign");
    RapidJsonCpp legacyObjectRoot;
    legacyObjectRoot = legacyKeyClear;
    check(legacyObjectRoot.toString().find("after array") != std::string::npos, "RapidJsonCpp legacy root object assign");
    legacyObjectRoot = legacyObjectRoot;
    check(legacyObjectRoot.toString().find("after array") != std::string::npos, "RapidJsonCpp legacy self assign");
    legacyObjectRoot = legacyObjectRoot.toRapidJsonValue();
    check(legacyObjectRoot.toString().find("after array") != std::string::npos, "RapidJsonCpp legacy self document assign");
    legacyObjectRoot = legacyObjectRoot.toRapidJsonValue();
    check(legacyObjectRoot.toString().find("after array") != std::string::npos, "RapidJsonCpp legacy self value assign");

    RapidJsonCpp legacyNullConstruct(static_cast<const rapidjson::Value*>(nullptr));
    check(legacyNullConstruct.isNull() && legacyNullConstruct.toString() == "null", "RapidJsonCpp legacy null pointer construct");
    RapidJsonCpp legacyNullRoot;
    legacyNullRoot = static_cast<const rapidjson::Value*>(nullptr);
    check(legacyNullRoot.isNull() && legacyNullRoot.toString() == "null", "RapidJsonCpp legacy null pointer root assign");
    RapidJsonCpp legacyNullMember;
    legacyNullMember["empty"] = static_cast<const rapidjson::Value*>(nullptr);
    legacyNullMember["after"] = 12;
    RapidJsonDocument legacyNullMemberDocument;
    check(legacyNullMemberDocument.parse(legacyNullMember.toString()), "RapidJsonCpp legacy null pointer member parse");
    check(legacyNullMemberDocument.getIntOrDefault("after", -1) == 12, "RapidJsonCpp legacy null pointer member clears key");
    check(legacyNullMember.toString().find("\"empty\":null") != std::string::npos, "RapidJsonCpp legacy null pointer member kept");

    return check.failures ? 1 : 0;
}

/** 重复构造、拷贝、移动和释放，失败返回1 */
static int LifetimeCases()
{
    Checks check = {};
    int32_t lifecycleFailCount = 0;
    for (int32_t loopIndex = 0; loopIndex < 1000; ++loopIndex)
    {
        RapidJsonCpp loopJson;
        loopJson["index"] = loopIndex;
        loopJson["name"] = std::string("loop");
        RapidJsonCpp loopJsonCopy(loopJson);
        RapidJsonCpp loopJsonAssign;
        loopJsonAssign = loopJsonCopy;
        RapidJsonDocument loopDocument;
        loopDocument.parse(loopJsonAssign.toString());
        RapidJsonDocument loopDocumentCopy(loopDocument);
        RapidJsonDocument loopDocumentMove(std::move(loopDocumentCopy));
        RapidJsonDocument loopDocumentAssign;
        loopDocumentAssign = std::move(loopDocumentMove);
        RapidJsonValue loopValue;
        loopValue.addInt("index", loopIndex);
        loopValue.addString("name", "loop");
        RapidJsonValue loopValueCopy(loopValue);
        RapidJsonValue loopValueMove(std::move(loopValueCopy));
        RapidJsonValue loopValueAssign;
        loopValueAssign = std::move(loopValueMove);
        RapidJsonDocument loopValueDocument;
        loopValueDocument.addValue("value", loopValueAssign);
        if (loopDocumentAssign.getIntOrDefault("index", -1) != loopIndex ||
            loopJsonAssign.toString().find("\"name\":\"loop\"") == std::string::npos ||
            loopValueDocument.toString().find("\"index\"") == std::string::npos)
        {
            ++lifecycleFailCount;
        }
    }
    check(lifecycleFailCount == 0, "RapidJsonCpp pointer lifecycle pressure");

    RapidJsonCpp cursor;
    const std::string large(4096, 'z');
    const std::string tree = "{\"row\":{\"data\":\"" + large + "\"},\"rows\":[7]}";
    check(cursor.parse(tree), "prepare nested cursor");
    RapidJsonCpp independent(cursor["row"].toRapidJsonValue());
    bool intact = true;
    for (int32_t index = 0; index < 128; ++index)
    {
        const rapidjson::Value* row = cursor["row"].toRapidJsonValue();
        intact = intact && row->IsObject() && row->HasMember("data") && (*row)["data"].IsString() &&
            (*row)["data"].GetStringLength() == large.size() &&
            std::memcmp((*row)["data"].GetString(), large.data(), large.size()) == 0;
        const std::vector<RapidJsonCpp> rows = cursor["rows"].toArray();
        intact = intact && rows.size() == 1 && rows.front().toIntOrDefault(-1) == 7;
    }
    check(intact, "alternating large subtree and array selections retain their full values");
    const rapidjson::Value* current = cursor["row"].toRapidJsonValue();
    const std::string malformed[] = {"", std::string("null\0x", 6), "[1,2,"};
    for (size_t index = 0; index < sizeof(malformed) / sizeof(malformed[0]); ++index)
    {
        check(!cursor.parse(malformed[index]) && current->IsNull() && cursor.toString() == "{}",
            "failed parse resets selected value and root");
        cursor = 11;
        check(current->IsInt() && current->GetInt() == 11 && cursor.toString() == "11",
            "failed parse clears pending key before the next root assignment");
        check(cursor.parse(tree) && cursor["row"].toRapidJsonValue() == current, "cursor reused after failed parse");
    }
    check(independent["data"].toStringOrDefault("") == large, "copied subtree survives parent reset");

    return check.failures ? 1 : 0;
}

#ifndef __EMSCRIPTEN__
/** 独立写和共享只读并发，失败返回1 */
static int ThreadCases()
{
    Checks check = {};
    std::atomic<int32_t> threadFailCount(0);
    std::vector<std::thread> threads;
    for (int32_t threadIndex = 0; threadIndex < 8; ++threadIndex)
    {
        threads.push_back(std::thread([threadIndex, &threadFailCount]() -> void
        {
            for (int32_t loopIndex = 0; loopIndex < 500; ++loopIndex)
            {
                std::ostringstream oss;
                oss << "worker_" << threadIndex << "_" << loopIndex;
                RapidJsonDocument threadDocument;
                threadDocument.addString("name", oss.str());
                threadDocument.addInt("index", loopIndex);
                threadDocument.addBool("ok", true);
                RapidJsonDocument parsed;
                if (!parsed.parse(threadDocument.toString()) ||
                    parsed.getStringOrDefault("name", "") != oss.str() ||
                    parsed.getIntOrDefault("index", -1) != loopIndex ||
                    !parsed.getBoolOrDefault("ok", false))
                {
                    threadFailCount.fetch_add(1);
                }
            }
        }));
    }
    for (size_t i = 0; i < threads.size(); ++i)
    {
        threads[i].join();
    }
    check(threadFailCount.load() == 0, "RapidJsonCpp multithread pressure");

    RapidJsonDocument sharedDocument;
    sharedDocument.addString("name", "shared");
    sharedDocument.addInt("count", 11);
    sharedDocument.addBool("ok", true);
    const std::string sharedJson = sharedDocument.toString();
    std::atomic<int32_t> readFailCount(0);
    std::vector<std::thread> readThreads;
    for (int32_t threadIndex = 0; threadIndex < 8; ++threadIndex)
    {
        readThreads.push_back(std::thread([&sharedDocument, &sharedJson, &readFailCount]() -> void
        {
            for (int32_t loopIndex = 0; loopIndex < 500; ++loopIndex)
            {
                if (sharedDocument.toString() != sharedJson ||
                    sharedDocument.getStringOrDefault("name", "") != "shared" ||
                    sharedDocument.getIntOrDefault("count", -1) != 11 ||
                    !sharedDocument.getBoolOrDefault("ok", false))
                {
                    readFailCount.fetch_add(1);
                }
            }
        }));
    }
    for (size_t i = 0; i < readThreads.size(); ++i)
    {
        readThreads[i].join();
    }
    check(readFailCount.load() == 0, "RapidJsonCpp shared read multithread pressure");

    return check.failures ? 1 : 0;
}
#endif

/** 检查UTF-8、整数边界、嵌套拷贝和按长度处理的字符串 */
static int BoundaryCases()
{
    Checks check = {};
    RapidJsonDocument doc;
    RapidJsonCpp selected;
    const char invalid[] = {'"', char(0xC0), char(0xAF), '"'};
    std::string bad(invalid, sizeof invalid);
    check(!doc.parse(bad) && doc.toString() == "{}", "document rejects malformed UTF-8");
    check(!selected.parse(bad) && selected.isNull(), "cursor rejects malformed UTF-8");
    const char* unpaired[] = {"\"\\udc00\"", "{\"\\uDFff\":1}", "\"\\\\\\uDd01\"", "\"\\uD814x\""};
    bool rejectedSurrogates = true;
    for (size_t i = 0; i < sizeof(unpaired) / sizeof(unpaired[0]); ++i)
    {
        const bool documentAccepted = doc.parse(unpaired[i]);
        const bool cursorAccepted = selected.parse(unpaired[i]);
        rejectedSurrogates = rejectedSurrogates && !documentAccepted && !cursorAccepted &&
            doc.toString() == "{}" && selected.isNull();
    }
    check(rejectedSurrogates, "unpaired surrogate escapes in string values and keys");
    const std::string paired = "{\"\\uD83c\\uDF4e\":\"\\ud83C\\udf4E\",\"literal\":\"\\\\uDC00\"}";
    const char* apple = "\xF0\x9F\x8D\x8E";
    check(doc.parse(paired) && doc.getStringOrDefault(apple, "") == apple &&
        doc.getStringOrDefault("literal", "") == "\\uDC00", "valid Unicode pair and escaped backslash in document");
    check(selected.parse(paired) && selected[apple].toStringOrDefault("") == apple &&
        selected["literal"].toStringOrDefault("") == "\\uDC00", "valid Unicode pair and escaped backslash in cursor");
    const double exact = 996.3723767827669;
    rapidjson::Document owner;
    rapidjson::Value parsedNumber;
    check(doc.parse("{\"n\":996.3723767827669}"), "document accepts a decimal near a rounding boundary");
    doc.getValue("n").copyToRapidJsonValue(&parsedNumber, &owner);
    check(parsedNumber.IsDouble() && parsedNumber.GetDouble() == exact, "document correctly rounds double");
    check(selected.parse("996.3723767827669") && selected.toRapidJsonValue()->IsDouble() &&
        selected.toRapidJsonValue()->GetDouble() == exact, "cursor correctly rounds double");
    const std::string truncated(1, static_cast<char>(0xf0));
    doc.addString("bad", truncated);
    check(doc.toString().empty(), "document reports invalid UTF-8 during serialization");
    doc.addString("bad", "\xF0\x9F\x8D\x8E");
    check(!doc.toString().empty(), "document serializes valid four-byte UTF-8");
    selected[truncated] = 1;
    check(selected.toString().empty(), "cursor reports invalid UTF-8 key during serialization");
    rapidjson::Value invalidNumbers(rapidjson::kArrayType);
    invalidNumbers.PushBack(1, owner.GetAllocator());
    invalidNumbers.PushBack((std::numeric_limits<double>::infinity)(), owner.GetAllocator());
    RapidJsonValue nonFinite(&invalidNumbers);
    check(nonFinite.toString().empty(), "nested non-finite number never returns partial JSON");
    check(doc.parse("{\"min\":-2147483648,\"max\":2147483647,\"big\":2147483648,\"mixed\":[1,\"one\",false]}"), "numeric input");
    check(doc.getIntOrDefault("min", 0) == (std::numeric_limits<int32_t>::min)() &&
        doc.getIntOrDefault("max", 0) == (std::numeric_limits<int32_t>::max)() &&
        doc.getIntOrDefault("big", -1) == -1, "32-bit limits");
    check(doc.getStringArrayOrEmpty("mixed") == std::vector<std::string>(1, "one"), "mixed string filtering");
    RapidJsonValue scalar;
    scalar.setString("seed");
    scalar.pushValue(scalar);
    check(scalar.toString() == "[\"seed\"]", "self append preserves scalar");
    scalar.setInt(17);
    scalar.addValue("previous", scalar);
    check(scalar.toString() == "{\"previous\":17}", "self member preserves scalar snapshot");
    selected["status"] = 12;
    check(selected.toIntOrDefault(-1) == 12, "cursor after member assignment");
    {
        bool rejected = false;
        scalar.setArray();
        try { scalar.reserve((std::numeric_limits<size_t>::max)()); }
        catch (const std::length_error&) { rejected = true; }
        check(rejected && scalar.size() == 0, "unrepresentable capacity");
    }
    char borrowed[] = "initial borrowed payload long enough to exceed inline string capacity";
    rapidjson::Value raw(rapidjson::StringRef(borrowed));
    RapidJsonValue value(&raw);
    RapidJsonCpp cursor(&raw);
    selected = &raw;
    borrowed[0] = 'X';
    check(value.toString()[1] == 'i' && cursor.toString()[1] == 'i' && selected.toString()[1] == 'i', "borrowed upstream strings owned");
    std::string key("a\0b", 3);
    selected[key] = -42;
    check(selected.hasMember(key) && !selected.hasMember("a") && selected[key].toIntOrDefault(0) == -42, "binary key");
    const std::string longText(4096, 'x');
    selected["first"] = longText;
    selected["second"] = 42;
    const rapidjson::Value* view = selected["first"].toRapidJsonValue();
    const char* payload = view->GetString();
    bool reused = true;
    for (int i = 0; i < 256; ++i)
    {
        const rapidjson::Value* same = selected["first"].toRapidJsonValue();
        reused = reused && same->GetString() == payload;
    }
    check(reused, "consecutive selections reuse the same long string");
    check(selected["second"].toRapidJsonValue() == view && view->IsInt() && view->GetInt() == 42,
        "borrowed cursor follows the newly selected value");
    view = selected["first"].toRapidJsonValue();
    selected = view;
    check(selected["first"].toStringOrDefault("") == longText, "assign borrowed selected value to its own member");
    return check.failures ? 1 : 0;
}

/** 对同一输入测量解析、序列化和末字段查找，未计入构造测试文本的时间 */
static int Benchmark()
{
    RapidJsonDocument doc;
    for (unsigned i = 0; i < 32; ++i)
    {
        const std::string key = "field" + std::to_string(i);
        doc.addInt(key.c_str(), static_cast<int>(i));
    }
    const std::string json = doc.toString();
    if (!doc.parse(json) || doc.getIntOrDefault("field31", -1) != 31) return 1;
    std::printf("json_bytes=%u fields=32\n", static_cast<unsigned>(json.size()));
    LibraryBenchmark::run("document.parse32", [&](uint32_t count) -> uint64_t
    {
        uint64_t valid = 0;
        for (uint32_t i = 0; i < count; ++i) valid += doc.parse(json);
        if (valid != count) throw std::runtime_error("parse failed");
        return valid;
    });
    LibraryBenchmark::run("document.serialize32", [&](uint32_t count) -> uint64_t
    {
        uint64_t bytes = 0;
        for (uint32_t i = 0; i < count; ++i) bytes += doc.toString().size();
        if (bytes != uint64_t(count) * json.size()) throw std::runtime_error("wrong size");
        return bytes;
    });
    LibraryBenchmark::run("document.findLast32", [&](uint32_t count) -> uint64_t
    {
        uint64_t sum = 0;
        for (uint32_t i = 0; i < count; ++i) sum += doc.getIntOrDefault("field31", -1);
        if (sum != uint64_t(count) * 31) throw std::runtime_error("lookup failed");
        return sum;
    });
    return doc.toString() == json ? 0 : 1;
}

/** 执行所选用例，0为通过，1为失败，2为非法选择 */
int main(int argc, char** argv)
{
    if (argc == 2 && !std::strcmp(argv[1], "--benchmark")) return Benchmark();
    const LibraryTestCase cases[] = {
        {1, false, ApiCases}, {2, false, BoundaryCases}, {3, true, LifetimeCases},
#ifndef __EMSCRIPTEN__
        {4, true, ThreadCases},
#endif
    };
    return LibraryCaseSelection::run(argc, argv, cases, [](const LibraryTestCase& test) { return test.run(); });
}