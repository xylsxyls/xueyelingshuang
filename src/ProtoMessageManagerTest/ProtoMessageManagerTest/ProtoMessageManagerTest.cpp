#include "ProtoMessageManagerTest.h"
#include "PlatformTest/PlatformTestAPI.h"
#include "ProtoMessageManager/ProtoMessageManagerAPI.h"
#include "ProtoMessageManager/ProtoMessage.pb.h"

#include <google/protobuf/stubs/once.h>
#include <cstdio>
#include <cstring>
#include <limits>
#include <thread>
#include <atomic>
#include <utility>
#include <new>
#include <cstdlib>

#ifdef _MSC_VER
#ifdef _DEBUG
#pragma comment(lib, "libprotobuf-lited.lib")
#else
#pragma comment(lib, "libprotobuf-lite.lib")
#endif
#endif

// -1表示关闭注入，0使下一次分配失败；仅串行故障用例修改该值
static int32_t allocationBudget = -1;

/** 测试进程的单次分配故障注入；允许第零次失败后恢复
@param [in] size 申请的字节数，零仍分配可释放的地址
@return 有效内存地址，失败抛出bad_alloc
*/
void* operator new(size_t size)
{
    if (allocationBudget == 0)
    {
        allocationBudget = -1;
        throw std::bad_alloc();
    }
    if (allocationBudget > 0)
    {
        --allocationBudget;
    }
    void* result = std::malloc(size == 0 ? 1 : size);
    if (result == nullptr)
    {
        throw std::bad_alloc();
    }
    return result;
}

/** 释放测试分配器交付的单对象内存
@param [in] pointer 待释放地址，可为空
*/
void operator delete(void* pointer) throw()
{
    std::free(pointer);
}

/** 使数组分配参与同一个故障预算
@param [in] size 整个数组需要的字节数
@return 有效内存地址，失败抛出bad_alloc
*/
void* operator new[](size_t size)
{
    return ::operator new(size);
}

/** 释放测试进程的数组分配
@param [in] pointer 待释放地址，可为空
*/
void operator delete[](void* pointer) throw()
{
    ::operator delete(pointer);
}

// 本次进程已执行的断言数
static unsigned checks = 0;
// 本次进程未达到预期的断言数
static unsigned failures = 0;

/** 记录独立预期，失败不中断进程
@param [in] result 本次比较结果
@param [in] description 用例名称
*/
static void Check(bool result, const char* description)
{
    ++checks;
    if (!result)
    {
        ++failures;
        std::printf("FAILED: %s\n", description);
    }
}

/** 比较类型和完整字节，不能用Variant历史字节比较忽略类型
@param [in] left 被测借用，可空
@param [in] right 独立预期
@return 类型及全部字节相同
*/
static bool Same(const Variant* left, const Variant& right)
{
    return left != nullptr && left->type() == right.type() && left->size() == right.size() &&
        (right.size() == 0 || std::memcmp(left->data(), right.data(), right.size()) == 0);
}

/** 使用公开接口运行固定输入的性能测试
@return 正确性校验通过为0，失败返回非0
*/
static int RunBenchmarks()
{
    ProtoMessageRecord original;
    for (int32_t key = 0; key < 32; ++key)
    {
        if (!original.setValue(key, Variant(key + 17)))
        {
            return 1;
        }
    }
    std::string wire;
    ProtoMessageRecord output;
    // 每次编码32个整数字段，计入正文分配及完整序列化
    LibraryBenchmark::run("protobuf.record.encode.32_int32", [&](uint32_t count) -> uint64_t
    {
        uint64_t bytes = 0;
        for (uint32_t i = 0; i < count; ++i)
        {
            if (!original.toString(wire))
            {
                throw std::runtime_error("record encode failed");
            }
            bytes += wire.size();
        }
        return bytes;
    });
    // 从同一正文反复解析，计入事务提交以及旧消息的析构
    LibraryBenchmark::run("protobuf.record.decode.32_int32", [&](uint32_t count) -> uint64_t
    {
        uint64_t fields = 0;
        for (uint32_t i = 0; i < count; ++i)
        {
            if (!output.from(wire.data(), wire.size()))
            {
                throw std::runtime_error("record parse failed");
            }
            fields += output.size();
        }
        return fields;
    });
    for (int32_t key = 0; key < 32; ++key)
    {
        if (!Same(output.findValue(key), Variant(key + 17)))
        {
            return 1;
        }
    }
    return 0;
}

/** 验证通用消息的分组、链式写入、拷贝、清空以及专用容器容量边界
*/
static void CheckPublicContainers()
{
    ProtoMessage message;
    const Variant first(int32_t(11)), second(int32_t(22));
    message[10][7] = first;
    message[10][8] = Variant(int32_t(22));
    Variant copied;
    Check(message.getKeyValue(7, copied, 10) && Same(&copied, first) &&
        Same(message.findValue(8, 10), second), "chain assignment selects map and field");
    Check(!message.getKeyValue(100, copied, 10) && copied.type() == Variant::INIT,
        "absent field clears copied value");
    std::map<int32_t, Variant> fields = message.getMap(10);
    fields[7] = second;
    message.setKeyMap(fields, 20);
    Check(Same(message.findValue(7, 10), first) && Same(message.findValue(7, 20), second),
        "returned map owns independent values");
    Check(message.setBytes(1, "a\0b", 3, Variant::BLOB, 30) &&
        !message.setBytes(1, nullptr, 1, Variant::BLOB, 30) && message.findValue(1, 30)->size() == 3 &&
        std::memcmp(message.findValue(1, 30)->data(), "a\0b", 3) == 0,
        "grouped bytes keep previous field on invalid input");
    message[40].push_back(second);
    message[40].push_front(first);
    std::list<Variant> list;
    Check(message.getList(list, 40) && list.size() == 2 && Same(&list.front(), first) &&
        Same(&list.back(), second), "grouped list appends and prepends in order");
    message.setKeyList(list, 41);
    Check(message.findList(41) != nullptr && message.getList(41).size() == 2, "assigned list group");
    std::vector<std::vector<Variant>> rows(2);
    rows[1].push_back(first);
    message[50].setTable(rows);
    message.setKeyTable(rows, 51);
    std::vector<std::vector<Variant>> readRows;
    Check(message.getTable(readRows, 50) && readRows.size() == 2 && readRows[0].empty() &&
        readRows[1].size() == 1 && Same(&readRows[1][0], first) && message.getTable(51).size() == 2,
        "ragged table preserves empty row and selected group");
    message.setCode(7, "cause");
    ProtoMessage assigned;
    assigned = message;
    ProtoMessage moved;
    moved = std::move(assigned);
    Check(moved.getCode() == 7 && moved.getErrorMessage() == "cause" && assigned.getCode() == 0,
        "copy assignment and move assignment retain code and error");
    moved.clearMap();
    Check(moved.findMap(10) == nullptr && moved.findList(40) != nullptr && moved.findTable(50) != nullptr,
        "clearMap preserves other shapes");
    moved.clearList();
    Check(moved.findList(40) == nullptr && moved.findTable(50) != nullptr, "clearList preserves table");
    moved.clearTable();
    Check(moved.findTable(50) == nullptr && moved.getCode() == 7, "clearTable preserves response code");
    moved.clear();
    Check(moved.getCode() == 0 && moved.getErrorMessage().empty(), "clear resets response state");
    ProtoMessageArray array;
    ProtoMessageRecord record;
    ProtoMessageTable table;
    Check(array.reserve(4) && !array.reserve(65536) && array.at(0) == nullptr, "array capacity limit and bounds");
    Check(record.reserve(4) && !record.reserve(65536) && record.setBytes(42, "abc", 3) &&
        record.findValue(42)->type() == Variant::BLOB, "record capacity and default byte type");
    Check(!record.setBytes(42, nullptr, 4) && record.findValue(42)->size() == 3,
        "record rejects invalid byte replacement");
    Check(table.setColumns(2) && table.reserveRows(3) && !table.reserveRows(21846) &&
        !table.setColumns(65535) && table.columns() == 2 && table.at(0, 0) == nullptr,
        "table reserve and column boundaries preserve schema");
}

/** 逐分配点检查编码失败保留输出、解析失败清空及重试
*/
static void CheckAllocationRollback()
{
    // 遍历分配失败点，覆盖protobuf临时树、输出串和业务容器恢复
    std::printf("phase: allocation\n");
    std::fflush(stdout);
    for (int32_t kind = 0; kind < 4; ++kind)
    {
        ProtoMessageDocument* source = ProtoMessageManager::create(static_cast<ProtoMessageKind>(kind));
        Variant content(std::string(8192, 'p'));
        if (source->general() != nullptr)
        {
            source->general()->setKeyValue(1, content);
        }
        if (source->record() != nullptr)
        {
            source->record()->setValue(1, content);
        }
        if (source->array() != nullptr)
        {
            source->array()->append(content);
        }
        if (source->table() != nullptr)
        {
            source->table()->setColumns(1);
            source->table()->appendRow(&content, 1);
        }
        std::string wire;
        Check(source->toString(wire), "fault fixture");
        for (int32_t budget = 0; budget < 120; ++budget)
        {
            std::string output("previous");
            allocationBudget = budget;
            const bool encoded = source->toString(output);
            allocationBudget = -1;
            Check(encoded ? output == wire : output == "previous", "encode allocation transaction");
            ProtoMessageDocument* target = ProtoMessageManager::create(static_cast<ProtoMessageKind>(kind));
            Check(target->from(wire.data(), wire.size()), "fault existing target");
            allocationBudget = budget;
            const bool decoded = target->from(wire.data(), wire.size());
            allocationBudget = -1;
            if (!decoded)
            {
                Check(kind == 0 ? target->general()->getCode() == ProtoMessage::PROTO_ERROR :
                    kind == 1 ? target->record()->size() == 0 :
                    kind == 2 ? target->array()->size() == 0 : target->table()->rows() == 0, "parse allocation fail closed");
            }
            Check(target->from(wire.data(), wire.size()), "recover from allocation failure");
            ProtoMessageManager::release(target);
        }
        ProtoMessageManager::release(source);
    }
}

#ifndef __EMSCRIPTEN__
/** 四个线程编码同一个稳定记录，各自持有输出和解析目标
*/
static void CheckConcurrentReads()
{
    ProtoMessageRecord shared;
    for (int32_t i = 0; i < 32; ++i)
    {
        shared.setValue(i, Variant(i));
    }
    std::atomic<int32_t> errors(0);
    std::vector<std::thread> threads;
    for (int32_t i = 0; i < 4; ++i)
    {
        // 每条线程只有自己的临时protobuf缓存、输出和解析对象
        threads.push_back(std::thread([&shared, &errors]()
        {
            for (int32_t n = 0; n < 1000; ++n)
            {
                std::string output;
                ProtoMessageRecord target;
                if (!shared.toString(output) || !target.from(output.data(), output.size()) || target.size() != 32)
                {
                    ++errors;
                }
            }
        }));
    }
    for (size_t i = 0; i < threads.size(); ++i)
    {
        threads[i].join();
    }
    Check(errors.load() == 0, "parallel const reads and encoding");
}
#endif

/** 运行所选功能或性能批次
@param [in] argc 命令行参数数量
@param [in] argv 可选择容器操作、线格式或性能测试
@return 全部选中断言通过返回0，错误输入返回2
*/
static int RunSuite(int argc, char* argv[])
{
    if (argc == 2 && std::strcmp(argv[1], "--benchmark") == 0)
    {
        return RunBenchmarks();
    }

    if (argc == 2 && std::strcmp(argv[1], "--list") == 0)
    {
#ifdef __EMSCRIPTEN__
        std::puts("containers wire allocation");
#else
        std::puts("containers wire allocation concurrent");
#endif
        return 0;
    }
    const bool all = argc <= 1;
    const bool allocationOnly = argc == 2 && std::strcmp(argv[1], "--case=allocation") == 0;
    bool concurrentOnly = false;
#ifndef __EMSCRIPTEN__
    concurrentOnly = argc == 2 && std::strcmp(argv[1], "--case=concurrent") == 0;
#endif
    const bool containersOnly = argc == 2 && std::strcmp(argv[1], "--case=containers") == 0;
    const bool wireOnly = argc == 2 && std::strcmp(argv[1], "--case=wire") == 0;
    if (!all && !containersOnly && !wireOnly && !allocationOnly && !concurrentOnly)
    {
        return 2;
    }
    try
    {
        if (allocationOnly)
        {
            CheckAllocationRollback();
        }
#ifndef __EMSCRIPTEN__
        if (concurrentOnly)
        {
            CheckConcurrentReads();
        }
#endif
        if (all || containersOnly)
        {
            CheckPublicContainers();
        }
        if (!all && !wireOnly)
        {
            return failures == 0 ? 0 : 1;
        }
        std::printf("phase: values\n");
        std::fflush(stdout);
        std::vector<Variant> values;
        values.push_back(Variant());
        values.push_back(Variant(int32_t(0)));
        values.push_back(Variant(int32_t(-1)));
        values.push_back(Variant((std::numeric_limits<int32_t>::min)()));
        values.push_back(Variant((std::numeric_limits<int32_t>::max)()));
        values.push_back(Variant(uint64_t(0)));
        values.push_back(Variant((std::numeric_limits<uint64_t>::max)()));
        const uint64_t doubles[] = {0, 0x8000000000000000ULL, 0x7ff0000000000000ULL, 0x7ff8000000001234ULL};
        for (size_t i = 0; i < sizeof(doubles) / sizeof(doubles[0]); ++i)
        {
            Variant v;
            Check(v.assignBytes(&doubles[i], 8, Variant::DOUBLE), "double source");
            values.push_back(v);
        }
        const size_t lengths[] = {0, 1, 15, 16, 17, 128, 4096};
        for (int32_t type = Variant::STRING; type <= Variant::BLOB; ++type)
        {
            for (size_t i = 0; i < sizeof(lengths) / sizeof(lengths[0]); ++i)
            {
                const std::string bytes(lengths[i], '\0');
                Variant v;
                Check(v.assignBytes(bytes.data(), bytes.size(), static_cast<Variant::VariantType>(type)), "raw string/blob");
                values.push_back(v);
            }
        }
        std::printf("phase: factory\n");
        std::fflush(stdout);
        for (int32_t kind = PROTO_MESSAGE_GENERAL; kind <= PROTO_MESSAGE_TABLE; ++kind)
        {
            ProtoMessageDocument* source = ProtoMessageManager::create(static_cast<ProtoMessageKind>(kind));
            ProtoMessageDocument* target = ProtoMessageManager::create(static_cast<ProtoMessageKind>(kind));
            Check(source != nullptr && target != nullptr && source->kind() == kind, "factory actual class");
            if (source == nullptr || target == nullptr)
            {
                ProtoMessageManager::release(source);
                ProtoMessageManager::release(target);
                return 1;
            }
            if (kind == PROTO_MESSAGE_GENERAL)
            {
                Check(dynamic_cast<ProtoMessage*>(source) == source->general(), "factory creates ProtoMessage itself");
            }
            if (source->table() != nullptr)
            {
                Check(source->table()->setColumns(values.size()), "table columns");
                Check(source->table()->appendRow(values.data(), values.size()), "mixed table");
            }
            for (size_t i = 0; i < values.size(); ++i)
            {
                if (source->general() != nullptr)
                {
                    source->general()->setKeyValue(static_cast<int32_t>(i), values[i]);
                }
                if (source->record() != nullptr)
                {
                    Check(source->record()->setValue(static_cast<int32_t>(i), values[i]), "mixed record");
                }
                if (source->array() != nullptr)
                {
                    Check(source->array()->append(values[i]), "mixed array");
                }
            }
            std::string wire;
            size_t size = 0;
            Check(source->toString(wire) && source->serializedSize(size) && size == wire.size(), "protobuf encode size");
            Check(target->from(wire.data(), wire.size()), "protobuf parse");
            if (kind != PROTO_MESSAGE_GENERAL)
            {
                pt::ProtoSpecializedMessage independent;
                Check(independent.ParseFromString(wire), "independent generated protobuf parse");
                Check((kind == PROTO_MESSAGE_RECORD && independent.has_record()) ||
                    (kind == PROTO_MESSAGE_ARRAY && independent.has_array()) ||
                    (kind == PROTO_MESSAGE_TABLE && independent.has_table()), "independent shape");
                for (size_t cut = 0; cut < wire.size(); ++cut)
                {
                    Check(!target->from(wire.data(), cut), "all-byte truncation rejected");
                }
                Check(target->from(wire.data(), wire.size()), "recovery after truncation");
            }
            for (size_t i = 0; i < values.size(); ++i)
            {
                const Variant* v = target->general() != nullptr ? target->general()->findValue(static_cast<int32_t>(i)) :
                    target->record() != nullptr ? target->record()->findValue(static_cast<int32_t>(i)) :
                    target->array() != nullptr ? target->array()->at(i) : target->table()->at(0, i);
                Check(Same(v, values[i]), "type and exact byte roundtrip");
            }
            std::vector<char> buffer(size + 16, 'x');
            size_t written = 99;
            Check(!source->serializeTo(buffer.data() + 8, size - 1, written) && written == 0 &&
                buffer == std::vector<char>(size + 16, 'x'), "short buffer untouched");
            Check(source->serializeTo(buffer.data() + 8, size, written) && written == size &&
                std::memcmp(buffer.data() + 8, wire.data(), size) == 0 && buffer[0] == 'x' && buffer.back() == 'x', "bounded direct output");
            Check(!target->from(nullptr, 1), "invalid input clears previous state");
            source->clear();
            Check(source->toString(wire) && target->from(wire.data(), wire.size()), "empty representation");
            ProtoMessageManager::release(source);
            ProtoMessageManager::release(target);
        }
        Check(ProtoMessageManager::create(static_cast<ProtoMessageKind>(99)) == nullptr, "unknown factory kind");
        ProtoMessageManager::release(nullptr);

        // 直接生成protobuf输入，验证适配层不依赖自己的编码输出
        std::printf("phase: fixture\n");
        std::fflush(stdout);
        pt::ProtoSpecializedMessage fixture;
        pt::ProtoField* field = fixture.mutable_record()->add_fields();
        field->set_key(-1);
        field->mutable_value()->set_signedvalue(-128);
        ProtoMessageRecord record;
        std::string bytes = fixture.SerializeAsString();
        Check(record.from(bytes.data(), bytes.size()) && Same(record.findValue(-1), Variant(int32_t(-128))), "independent protobuf record input");
        field = fixture.mutable_record()->add_fields();
        field->set_key(-1);
        field->mutable_value()->set_blobvalue("a\0b", 3);
        bytes = fixture.SerializeAsString();
        Check(record.from(bytes.data(), bytes.size()) && Same(record.findValue(-1), Variant(std::string("a\0b", 3))), "duplicate fields last wins");
        ProtoMessageArray array;
        Check(!array.from(bytes.data(), bytes.size()), "wrong shape rejected");
        fixture.mutable_array()->mutable_values()->mutable_mixed();
        bytes = fixture.SerializeAsString();
        Check(!record.from(bytes.data(), bytes.size()), "conflicting protobuf shapes rejected");
        fixture.clear_array();
        fixture.mutable_record()->mutable_fields(0)->mutable_value()->set_blobvalue("x");
        bytes = fixture.SerializeAsString();
        Check(!record.from(bytes.data(), bytes.size()), "conflicting value types rejected");
        fixture.Clear();

        // 相同类型数值数组验证packed及整数边界；混合追加后切换回来
        std::printf("phase: packed\n");
        std::fflush(stdout);
        for (int32_t type = Variant::INIT; type <= Variant::DOUBLE; ++type)
        {
            ProtoMessageArray numbers;
            for (int32_t i = 0; i < 512; ++i)
            {
                const Variant value = type == Variant::INIT ? Variant() : type == Variant::INT32 ? Variant(int32_t(i - 256)) :
                    type == Variant::UINT64 ? Variant(static_cast<uint64_t>(i)) : Variant(static_cast<double>(i));
                Check(numbers.append(value), "numeric append");
            }
            Check(numbers.toString(bytes) && array.from(bytes.data(), bytes.size()), "numeric packed roundtrip");
            Check(fixture.ParseFromString(bytes) && fixture.has_array() && !fixture.array().values().has_mixed(), "packed protobuf branch");
            for (size_t i = 0; i < numbers.size(); ++i)
            {
                Check(Same(array.at(i), *numbers.at(i)), "packed exact values");
            }
            Check(numbers.append(Variant("text")) && numbers.toString(bytes) && array.from(bytes.data(), bytes.size()), "packed to mixed");
        }
        std::printf("phase: limits\n");
        std::fflush(stdout);
        fixture.Clear();
        fixture.mutable_array()->mutable_values()->mutable_emptyvalues()->set_count(65536);
        bytes = fixture.SerializeAsString();
        Check(!array.from(bytes.data(), bytes.size()) && array.size() == 0, "compact INIT amplification bounded");
        fixture.mutable_array()->mutable_values()->mutable_emptyvalues()->set_count(65535);
        bytes = fixture.SerializeAsString();
        Check(array.from(bytes.data(), bytes.size()) && array.size() == 65535 && !array.append(Variant()), "maximum array nodes");
        array.clear();
        Check(array.append(Variant(std::string(1048576, 'b'))) && !array.toString(bytes), "total protobuf overhead budget");
        Check(!array.append(Variant(std::string(1048577, 'b'))), "oversized value rejected");

        // 构造当前标准库实际使用的碰撞桶，验证外部字段不能触发无界平方级遍历
        std::unordered_map<int32_t, Variant> bucketModel;
        bucketModel.reserve(1500);
        fixture.Clear();
        pt::ProtoMessageRecord* attack = fixture.mutable_record();
        for (int32_t key = 0; key < 10000000 && attack->fields_size() < 1500; ++key)
        {
            if (bucketModel.bucket(key) == 0)
            {
                pt::ProtoField* item = attack->add_fields();
                item->set_key(key);
                item->mutable_value()->set_signedvalue(key);
            }
        }
        Check(attack->fields_size() == 1500, "construct real hash collision input");
        bytes = fixture.SerializeAsString();
        record.setValue(-1, Variant("old identity"));
        Check(!record.from(bytes.data(), bytes.size()) && record.size() == 0, "hash collision work budget and clear old identity");

        ProtoMessageTable table;
        Check(table.setColumns(0) && table.appendRow(nullptr, 0) && table.toString(bytes), "zero columns nonzero rows");
        ProtoMessageTable parsedTable;
        Check(parsedTable.from(bytes.data(), bytes.size()) && parsedTable.rows() == 1 && parsedTable.columns() == 0, "zero column schema retained");
        fixture.Clear();
        fixture.mutable_table()->set_columns(65534);
        fixture.mutable_table()->set_rows(65535);
        fixture.mutable_table()->mutable_values()->mutable_mixed();
        bytes = fixture.SerializeAsString();
        Check(!parsedTable.from(bytes.data(), bytes.size()) && parsedTable.rows() == 0, "table multiplication budget");
        table.clear();
        Check(table.setColumns(2), "set columns on empty table");
        Variant row[] = {Variant("a"), Variant(int32_t(42))};
        Check(table.appendRow(row, 2), "table append");
        Check(table.appendRow(table.at(0, 0), 2) && Same(table.at(1, 0), row[0]), "table own borrow across growth");
        Check(!table.appendRow(row, 1) && table.rows() == 2, "partial row rejected");

        // 通用旧protobuf字节兼容：用第三方生成代码独立构造相同内容
        std::printf("phase: general\n");
        std::fflush(stdout);
        ProtoMessage message;
        pt::ProtoMessage legacy;
        for (int32_t i = 0; i < 32; ++i)
        {
            message.setKeyValue(i, Variant(i));
            pt::KeyMapData* group = i == 0 ? legacy.add_keymapdata() : legacy.mutable_keymapdata(0);
            pt::MapKeyValue* item = group->mutable_mapdata()->add_mapkeyvalue();
            if (i != 0)
            {
                item->set_key(i);
            }
            item->mutable_value()->set_type(Variant::INT32);
            item->mutable_value()->set_data(&i, sizeof(i));
        }
        Check(message.toString(bytes) && bytes == legacy.SerializeAsString(), "old protobuf exact bytes");
        ProtoMessage copied(message);
        ProtoMessage moved(std::move(copied));
        Check(moved.toString() == bytes && copied.toString().empty(), "copy move ownership");
        std::string original = bytes;
        message.setKeyValue(100, Variant(bytes));
        const Variant* borrowed = message.findValue(100);
        Check(message.from(borrowed->data(), borrowed->size()) && message.toString() == original, "own borrowed parse transaction");
        Check(!message.from(nullptr, 2) && message.getCode() == ProtoMessage::PROTO_ERROR && message.findMap() == nullptr, "general parse fail closed");
        Check(message.from(nullptr, 0) && message.getCode() == 0, "general empty parse");

        // 有效protobuf也可能含无效Variant宽度，拒绝借用旧身份字段
        legacy.mutable_keymapdata(0)->mutable_mapdata()->mutable_mapkeyvalue(0)->mutable_value()->set_data("x");
        Check(!message.from(legacy.SerializeAsString()) && message.getCode() == ProtoMessage::PROTO_ERROR, "legacy invalid numeric width");

        if (all)
        {
            CheckAllocationRollback();
        }
        // 变异测试允许合法protobuf未知字段；成功后必须可稳定再解析
        std::printf("phase: mutations\n");
        std::fflush(stdout);
        table.toString(original);
        uint32_t random = 1;
        for (int32_t i = 0; i < 10000; ++i)
        {
            bytes = original;
            random = random * 1664525U + 1013904223U;
            bytes[random % bytes.size()] ^= static_cast<char>((random >> 16) | 1);
            if (table.from(bytes.data(), bytes.size()))
            {
                std::string stable;
                Check(table.toString(stable) && parsedTable.from(stable.data(), stable.size()) &&
                    parsedTable.columns() == table.columns() && parsedTable.rows() == table.rows(), "mutation accepted semantic stability");
            }
            else
            {
                Check(table.rows() == 0, "mutation fail clears rows");
            }
        }
#ifndef __EMSCRIPTEN__
        if (all)
        {
            CheckConcurrentReads();
        }
#endif
    }
    catch (...)
    {
        Check(false, "unexpected exception");
    }
    std::printf("ProtoMessageManager checks=%u failures=%u\n", checks, failures);
    return failures == 0 ? 0 : 1;
}

/** 执行所选功能组，默认运行完整套件。
@param [in] argc 命令行参数数量
@param [in] argv 命令行参数数组
@return 0 表示通过，1 表示断言失败，2 表示参数无效
*/
int main(int argc, char* argv[])
{
    const LibraryNamedTestCase cases[] =
    {
        {ProtoMessageManagerTestContainers, false, "containers"},
        {ProtoMessageManagerTestWire, true, "wire"},
        {ProtoMessageManagerTestAllocation, true, "allocation"},
#ifndef __EMSCRIPTEN__
        {ProtoMessageManagerTestConcurrent, true, "concurrent"},
#endif
    };
    return LibraryCaseSelection::runNamed(argc, argv, cases, RunSuite);
}