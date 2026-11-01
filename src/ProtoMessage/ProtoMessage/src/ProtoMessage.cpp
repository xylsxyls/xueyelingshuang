#include "ProtoMessage.h"
#include "ProtoMessage.pb.h"
#include "google/protobuf/io/coded_stream.h"
#include <utility>

#if defined(_MSC_VER) && !defined(PROTO_MESSAGE_NO_AUTOLINK)
#if defined _WIN64 && defined _DEBUG
#pragma comment(lib, "libprotobuf-litestatic64d.lib")
#elif defined _WIN64
#pragma comment(lib, "libprotobuf-litestatic64.lib")
#elif defined _DEBUG
#pragma comment(lib, "libprotobuf-litestatic32d.lib")
#else
#pragma comment(lib, "libprotobuf-litestatic32.lib")
#endif
#endif

const int32_t ProtoMessage::PROTO_ERROR;

ProtoMessage::ProtoMessage() :
m_code(0),
m_hasCode(false),
m_key(-1),
m_dataKey(-1)
{

}

ProtoMessage::ProtoMessage(const ProtoMessage& other) :
m_error(other.m_error),
m_keyMapData(other.m_keyMapData),
m_keyListData(other.m_keyListData),
m_keyTableData(other.m_keyTableData),
m_code(other.m_code),
m_hasCode(other.m_hasCode),
m_key(other.m_key),
m_dataKey(other.m_dataKey)
{
    // 成员各自拥有资源，后续成员复制失败时由C++回收已完成构造的成员。
}

ProtoMessage::~ProtoMessage()
{

}

void ProtoMessage::swap(ProtoMessage& other)
{
    m_error.swap(other.m_error);
    m_keyMapData.swap(other.m_keyMapData);
    m_keyListData.swap(other.m_keyListData);
    m_keyTableData.swap(other.m_keyTableData);
    std::swap(m_code, other.m_code);
    std::swap(m_hasCode, other.m_hasCode);
    std::swap(m_key, other.m_key);
    std::swap(m_dataKey, other.m_dataKey);
}

ProtoMessage& ProtoMessage::operator=(const ProtoMessage& other)
{
    if (this != &other)
    {
        ProtoMessage copy(other);
        swap(copy);
    }
    return *this;
}

ProtoMessage& ProtoMessage::operator[](const int32_t& key)
{
    if (m_dataKey != -1)
    {
        m_key = m_dataKey;
    }
    m_dataKey = key;
    return *this;
}

ProtoMessage ProtoMessage::operator=(const Variant& value)
{
    const int32_t key = m_key == -1 ? PROTO_GLOBAL : m_key;
    const int32_t dataKey = m_dataKey;
    m_key = -1;
    m_dataKey = -1;
    setKeyValue(dataKey, value, key);
    return *this;
}

void ProtoMessage::push_back(const Variant& value)
{
    const int32_t key = m_dataKey == -1 ? PROTO_GLOBAL : m_dataKey;
    m_key = -1;
    m_dataKey = -1;
    addListValue(value, key);
}

void ProtoMessage::push_front(const Variant& value)
{
    const int32_t key = m_dataKey == -1 ? PROTO_GLOBAL : m_dataKey;
    m_key = -1;
    m_dataKey = -1;
    m_keyListData[key].push_front(value);
}

void ProtoMessage::setTable(const std::vector<std::vector<Variant>>& table)
{
    const int32_t key = m_dataKey == -1 ? PROTO_GLOBAL : m_dataKey;
    m_key = -1;
    m_dataKey = -1;
    setKeyTable(table, key);
}

void ProtoMessage::setCode(int32_t code, const std::string& errorMsg)
{
    std::string copy(errorMsg);
    m_error.swap(copy);
    m_code = code;
    m_hasCode = true;
}

int32_t ProtoMessage::getCode() const
{
    return m_code;
}

std::string ProtoMessage::getErrorMessage() const
{
    return m_error;
}

void ProtoMessage::setKeyValue(const int32_t& dataKey, const Variant& dataValue, const int32_t& key)
{
    m_keyMapData[key][dataKey] = dataValue;
}

bool ProtoMessage::getKeyValue(int32_t dataKey, Variant& value, int32_t key) const
{
    value.clear();
    const auto group = m_keyMapData.find(key);
    if (group == m_keyMapData.end())
    {
        return false;
    }
    const auto entry = group->second.find(dataKey);
    if (entry == group->second.end())
    {
        return false;
    }
    try
    {
        value = entry->second;
        return true;
    }
    catch (...)
    {
        value.clear();
        return false;
    }
}

void ProtoMessage::addListValue(const Variant& value, const int32_t& key)
{
    m_keyListData[key].push_back(value);
}

void ProtoMessage::setKeyMap(const std::map<int32_t, Variant>& mapData, const int32_t& key)
{
    std::map<int32_t, Variant> copy(mapData);
    m_keyMapData[key].swap(copy);
}

void ProtoMessage::setKeyList(const std::list<Variant>& listData, const int32_t& key)
{
    std::list<Variant> copy(listData);
    m_keyListData[key].swap(copy);
}

void ProtoMessage::setKeyTable(const std::vector<std::vector<Variant>>& tableData, const int32_t& key)
{
    std::vector<std::vector<Variant>> copy(tableData);
    m_keyTableData[key].swap(copy);
}

std::map<int32_t, Variant> ProtoMessage::getMap(const int32_t& key)
{
    std::map<int32_t, Variant> result;
    getMap(result, key);
    return result;
}

std::list<Variant> ProtoMessage::getList(const int32_t& key)
{
    std::list<Variant> result;
    getList(result, key);
    return result;
}

std::vector<std::vector<Variant>> ProtoMessage::getTable(const int32_t& key)
{
    std::vector<std::vector<Variant>> result;
    getTable(result, key);
    return result;
}

bool ProtoMessage::getMap(std::map<int32_t, Variant>& mapData, const int32_t& key)
{
    const auto group = m_keyMapData.find(key);
    if (group == m_keyMapData.end())
    {
        return false;
    }
    try
    {
        std::map<int32_t, Variant> copy(group->second);
        mapData.swap(copy);
        return true;
    }
    catch (...)
    {
        return false;
    }
}

bool ProtoMessage::getList(std::list<Variant>& listData, const int32_t& key)
{
    const auto group = m_keyListData.find(key);
    if (group == m_keyListData.end())
    {
        return false;
    }
    try
    {
        std::list<Variant> copy(group->second);
        listData.swap(copy);
        return true;
    }
    catch (...)
    {
        return false;
    }
}

bool ProtoMessage::getTable(std::vector<std::vector<Variant>>& tableData, const int32_t& key)
{
    const auto group = m_keyTableData.find(key);
    if (group == m_keyTableData.end())
    {
        return false;
    }
    try
    {
        std::vector<std::vector<Variant>> copy(group->second);
        tableData.swap(copy);
        return true;
    }
    catch (...)
    {
        return false;
    }
}

bool ProtoMessage::fail()
{
    clear();
    m_code = PROTO_ERROR;
    m_hasCode = true;
    return false;
}

bool ProtoMessage::from(const std::string& data)
{
    try
    {
        if (data.size() > 1048576)
        {
            return fail();
        }
        ProtoMessage decoded;
        pt::ProtoMessage parsed;
        google::protobuf::io::CodedInputStream input(
            reinterpret_cast<const uint8_t*>(data.data()), static_cast<int32_t>(data.size()));
        input.SetTotalBytesLimit(1048576, 1048576);
        input.SetRecursionLimit(16);
        if (!parsed.ParseFromCodedStream(&input) || !input.ConsumedEntireMessage())
        {
            return fail();
        }
        size_t entries = 0;
        for (int32_t index = 0; index < parsed.keymapdata_size(); ++index)
        {
            const pt::KeyMapData& group = parsed.keymapdata(index);
            const pt::MapData& values = group.mapdata();
            entries += 1 + values.mapkeyvalue_size();
            if (entries > 65536)
            {
                return fail();
            }
            auto& target = decoded.m_keyMapData[group.key()];
            for (int32_t item = 0; item < values.mapkeyvalue_size(); ++item)
            {
                const pt::MapKeyValue& entry = values.mapkeyvalue(item);
                const pt::VariantData& value = entry.value();
                if (!Variant::isValidBlob(value.data(), value.type()))
                {
                    return fail();
                }
                target[entry.key()] = Variant(value.data(), static_cast<Variant::VariantType>(value.type()), true);
            }
        }
        for (int32_t index = 0; index < parsed.keylistdata_size(); ++index)
        {
            const pt::KeyListData& group = parsed.keylistdata(index);
            const pt::ListData& values = group.listdata();
            entries += 1 + values.value_size();
            if (entries > 65536)
            {
                return fail();
            }
            auto& target = decoded.m_keyListData[group.key()];
            for (int32_t item = 0; item < values.value_size(); ++item)
            {
                const pt::VariantData& value = values.value(item);
                if (!Variant::isValidBlob(value.data(), value.type()))
                {
                    return fail();
                }
                target.push_back(Variant(value.data(), static_cast<Variant::VariantType>(value.type()), true));
            }
        }
        for (int32_t index = 0; index < parsed.keytabledata_size(); ++index)
        {
            const pt::KeyTableData& group = parsed.keytabledata(index);
            const pt::TableData& table = group.tabledata();
            entries += 1 + table.listdata_size();
            if (entries > 65536)
            {
                return fail();
            }
            std::vector<std::vector<Variant>> rows(table.listdata_size());
            for (int32_t row = 0; row < table.listdata_size(); ++row)
            {
                const pt::ListData& values = table.listdata(row);
                entries += values.value_size();
                if (entries > 65536)
                {
                    return fail();
                }
                rows[row].reserve(values.value_size());
                for (int32_t item = 0; item < values.value_size(); ++item)
                {
                    const pt::VariantData& value = values.value(item);
                    if (!Variant::isValidBlob(value.data(), value.type()))
                    {
                        return fail();
                    }
                    rows[row].push_back(Variant(value.data(), static_cast<Variant::VariantType>(value.type()), true));
                }
            }
            decoded.m_keyTableData[group.key()].swap(rows);
        }
        // proto3.3既有生成代码跳过未知字段；仅保留已知字段，不长期持有protobuf载荷副本。
        decoded.m_hasCode = parsed.has_rspcode();
        decoded.m_code = parsed.rspcode().code();
        decoded.m_error = parsed.rspcode().errormessage();
        swap(decoded);
        return true;
    }
    catch (...)
    {
        return fail();
    }
}

std::string ProtoMessage::toString()
{
    std::string result;
    toString(result);
    return result;
}

bool ProtoMessage::toString(std::string& result)
{
    try
    {
        pt::ProtoMessage encoded;
        if (m_hasCode)
        {
            encoded.mutable_rspcode()->set_code(m_code);
            encoded.mutable_rspcode()->set_errormessage(m_error);
        }
        size_t entries = 0;
        for (auto group = m_keyMapData.begin(); group != m_keyMapData.end(); ++group)
        {
            entries += 1 + group->second.size();
            if (entries > 65536)
            {
                return false;
            }
            pt::KeyMapData* target = encoded.add_keymapdata();
            target->set_key(group->first);
            pt::MapData* values = target->mutable_mapdata();
            for (auto item = group->second.begin(); item != group->second.end(); ++item)
            {
                pt::MapKeyValue* entry = values->add_mapkeyvalue();
                entry->set_key(item->first);
                pt::VariantData* value = entry->mutable_value();
                item->second.toBlob(*value->mutable_data());
                value->set_type(item->second.type());
            }
        }
        for (auto group = m_keyListData.begin(); group != m_keyListData.end(); ++group)
        {
            entries += 1 + group->second.size();
            if (entries > 65536)
            {
                return false;
            }
            pt::KeyListData* target = encoded.add_keylistdata();
            target->set_key(group->first);
            pt::ListData* values = target->mutable_listdata();
            for (auto item = group->second.begin(); item != group->second.end(); ++item)
            {
                pt::VariantData* value = values->add_value();
                item->toBlob(*value->mutable_data());
                value->set_type(item->type());
            }
        }
        for (auto group = m_keyTableData.begin(); group != m_keyTableData.end(); ++group)
        {
            entries += 1 + group->second.size();
            if (entries > 65536)
            {
                return false;
            }
            pt::KeyTableData* target = encoded.add_keytabledata();
            target->set_key(group->first);
            pt::TableData* table = target->mutable_tabledata();
            for (size_t row = 0; row < group->second.size(); ++row)
            {
                const auto& source = group->second[row];
                entries += source.size();
                if (entries > 65536)
                {
                    return false;
                }
                pt::ListData* values = table->add_listdata();
                for (size_t item = 0; item < source.size(); ++item)
                {
                    pt::VariantData* value = values->add_value();
                    source[item].toBlob(*value->mutable_data());
                    value->set_type(source[item].type());
                }
            }
        }
        if (encoded.ByteSizeLong() > 1048576)
        {
            return false;
        }
        std::string bytes;
        if (!encoded.SerializeToString(&bytes))
        {
            return false;
        }
        result.swap(bytes);
        return true;
    }
    catch (...)
    {
        return false;
    }
}

void ProtoMessage::clear()
{
    std::string empty;
    m_error.swap(empty);
    m_code = 0;
    m_hasCode = false;
    clearMap();
    clearList();
    clearTable();
    m_key = -1;
    m_dataKey = -1;
}

void ProtoMessage::clearMap()
{
    m_keyMapData.clear();
}

void ProtoMessage::clearList()
{
    m_keyListData.clear();
}

void ProtoMessage::clearTable()
{
    m_keyTableData.clear();
}

// PROTO_MESSAGE_OWNERSHIP_UNIT_BEGIN
//#include <cstdio>
//#include <cstring>
//#include <stdexcept>
//#if defined(_MSC_VER) && defined(_DEBUG) && defined(STATIC_LIB)
//#include <crtdbg.h>
//#endif
//
//int main()
//{
//    uint32_t checks = 0;
//    // 每个独立条件计数，失败交给测试runner记录；生产路径没有断言终止。
//    const auto Check = [&checks](bool condition, const char* reason)
//    {
//        ++checks;
//        if (!condition)
//        {
//            throw std::runtime_error(reason);
//        }
//    };
//    try
//    {
//#if !defined(STATIC_LIB)
//        // DLL边界只传借用的const输入和POD结果，不跨各自CRT释放可变STL输出。
//        for (uint32_t iteration = 0; iteration < 1000; ++iteration)
//        {
//            ProtoMessage original;
//            original.setCode(19, std::string(80, 'r'));
//            original.setKeyValue(1, Variant(int32_t(7)));
//            original.setKeyValue(2, Variant(std::string(1000, 'b')));
//            original.addListValue(Variant(int32_t(1)));
//            original[5].push_front(Variant(int32_t(2)));
//            std::vector<std::vector<Variant>> table(3, std::vector<Variant>(2, Variant(int32_t(3))));
//            original.setKeyTable(table);
//            ProtoMessage copy(original);
//            ProtoMessage assigned;
//            assigned = copy;
//            assigned = assigned;
//            original.clear();
//            Check(copy.getCode() == 19 && assigned.getCode() == 19, "DLL copy metadata");
//            copy.clearMap();
//            copy.clearList();
//            copy.clearTable();
//            Check(copy.getCode() == 19, "DLL selective clear");
//            Check(!copy.from(std::string("\x0a\x07\x08", 3)) && copy.getCode() == ProtoMessage::PROTO_ERROR,
//                "DLL failed parse state");
//            Check(copy.from("") && copy.getCode() == 0, "DLL empty recovery");
//        }
//#else
//
//        // STRING文本入口截断；原始STRING反序列化及BLOB保留完整零字节并保持类型号。
//        const std::string nulBytes("A\0B", 3);
//        ProtoMessage nulSource;
//        nulSource.setKeyValue(1, Variant(nulBytes.c_str()));
//        nulSource.setKeyValue(2, Variant(nulBytes));
//        nulSource.setKeyValue(3, Variant(nulBytes, Variant::STRING, true));
//        const std::string nulWire = nulSource.toString();
//        ProtoMessage nulDecoded;
//        Check(nulDecoded.from(nulWire), "NUL fixture parse");
//        Variant nulValue;
//        Check(nulDecoded.getKeyValue(1, nulValue) && nulValue.type() == Variant::STRING &&
//            nulValue.toBlob() == "A", "ordinary STRING round trip");
//        Check(nulDecoded.getKeyValue(2, nulValue) && nulValue.type() == Variant::BLOB &&
//            nulValue.toBlob() == nulBytes, "BLOB NUL round trip");
//        Check(nulDecoded.getKeyValue(3, nulValue) && nulValue.type() == Variant::STRING &&
//            nulValue.toBlob() == nulBytes && nulDecoded.toString() == nulWire, "serialized STRING NUL round trip");
//
//        const char* truncated[] = {"\x0a\x7f", "\x12\x7f", "\x12\x02\x12\x7f", "\x1a\x02\x12\x7f",
//            "\x22\x02\x12\x7f", "\x0a\x02\x12\x7f"};
//        const uint32_t truncatedSizes[] = {2, 2, 4, 4, 4, 4};
//        for (uint32_t index = 0; index < 6; ++index)
//        {
//            ProtoMessage rejected;
//            rejected.setKeyValue(91, Variant("old identity"));
//            Check(!rejected.from(std::string(truncated[index], truncatedSizes[index])) &&
//                rejected.getCode() == ProtoMessage::PROTO_ERROR && rejected.getMap().empty(), "truncated embedded message accepted");
//        }
//
//        const unsigned char raw[] = {0x0a, 0x07, 0x08, 0x07, 0x12, 0x03, 'A', 0, 'Z',
//            0x12, 0x10, 0x12, 0x0e, 0x0a, 0x0c, 0x08, 0x01,
//            0x12, 0x08, 0x0a, 0x04, 0xef, 0xbe, 0xad, 0xde, 0x10, 0x01};
//        const std::string legacy(reinterpret_cast<const char*>(raw), sizeof(raw));
//        ProtoMessage message;
//        message.setCode(7, std::string("A\0Z", 3));
//        message.setKeyValue(1, Variant(static_cast<int32_t>(-559038737)));
//        Check(message.toString() == legacy, "independent legacy byte fixture");
//        Check(message.toString() == legacy, "encoding changes message");
//        Check(message.from(legacy) && message.toString() == legacy, "legacy round trip");
//        const char unknownRaw[] = {0x0a, 0x04, 0x08, 0x09, 0x18, 0x07,
//            static_cast<char>(0xa0), 0x06, 0x01};
//        const std::string unknown(unknownRaw, sizeof(unknownRaw));
//        const std::string known("\x0a\x02\x08\x09", 4);
//        Check(message.from(unknown) && message.toString() == known, "legacy unknown skip rule");
//        ProtoMessage unknownCopy(message);
//        Check(unknownCopy.toString() == known, "known metadata copy");
//        unknownCopy.setCode(9);
//        Check(unknownCopy.toString() == known, "setCode metadata");
//
//        message.clear();
//        const std::map<int32_t, Variant> emptyMap;
//        const std::list<Variant> emptyList;
//        const std::vector<std::vector<Variant>> emptyTable;
//        message.setKeyMap(emptyMap, 11);
//        message.setKeyList(emptyList, 12);
//        message.setKeyTable(emptyTable, 13);
//        ProtoMessage parsed;
//        Check(parsed.from(message.toString()), "empty containers parse");
//        std::map<int32_t, Variant> mapOutput;
//        std::list<Variant> listOutput;
//        std::vector<std::vector<Variant>> tableOutput;
//        Check(parsed.getMap(mapOutput, 11) && mapOutput.empty(), "empty map existence");
//        Check(parsed.getList(listOutput, 12) && listOutput.empty(), "empty list existence");
//        Check(parsed.getTable(tableOutput, 13) && tableOutput.empty(), "empty table existence");
//        std::vector<std::vector<Variant>> table(3);
//        table[1].push_back(Variant(std::string("a\0b", 3)));
//        message.setKeyTable(table);
//        message[25][7] = Variant(int32_t(-7));
//        message[9] = Variant(static_cast<uint64_t>(UINT64_MAX));
//        message[6].push_back(Variant(int32_t(2)));
//        message[6].push_front(Variant(int32_t(1)));
//        message.push_back(Variant(int32_t(4)));
//        const std::string encoded = message.toString();
//        Check(parsed.from(encoded) && parsed.toString() == encoded, "all container round trip");
//        Variant value;
//        Check(parsed.getKeyValue(7, value, 25) && static_cast<int32_t>(value) == -7, "nested selection");
//        Check(parsed.getKeyValue(9, value) && static_cast<uint64_t>(value) == UINT64_MAX, "selection resets");
//        Check(parsed.getList(listOutput, 6) && listOutput.size() == 2 &&
//            static_cast<int32_t>(listOutput.front()) == 1 && static_cast<int32_t>(listOutput.back()) == 2,
//            "list insertion ordering");
//        Check(parsed.getList(listOutput) && listOutput.size() == 1 &&
//            static_cast<int32_t>(listOutput.front()) == 4, "global list selection");
//        Check(parsed.getTable(tableOutput) && tableOutput.size() == 3 && tableOutput[0].empty() &&
//            tableOutput[2].empty() && tableOutput[1][0].toBlob() == std::string("a\0b", 3), "empty table rows");
//        mapOutput[1] = Variant(int32_t(123));
//        Check(!parsed.getMap(mapOutput, -100) && mapOutput.size() == 1, "missing getter changed output");
//        Check(!parsed.getKeyValue(-123, value) && value.type() == Variant::INIT, "missing value stale data");
//
//        for (uint32_t iteration = 0; iteration < 1000; ++iteration)
//        {
//            ProtoMessage copy(parsed);
//            ProtoMessage assigned;
//            assigned = copy;
//            assigned = assigned;
//            copy.clear();
//            Check(assigned.toString() == encoded, "copy lifetime");
//            Check(assigned.from(encoded) && assigned.toString() == encoded, "repeat parse/serialize");
//            Check(!assigned.from(std::string("\x0a\x07\x08", 3)) &&
//                assigned.getCode() == ProtoMessage::PROTO_ERROR && assigned.getMap().empty() &&
//                assigned.getList().empty() && assigned.getTable().empty(), "failed parse old identity");
//            Check(assigned.from("") && assigned.getCode() == 0, "failure recovery");
//        }
//        for (int32_t type = -1; type <= 7; ++type)
//        {
//            for (int32_t length = 0; length <= 9; ++length)
//            {
//                const bool valid = (type == 0 && length == 0) || (type == 1 && length == 4) ||
//                    ((type == 2 || type == 3) && length == 8) || type == 4 || type == 5;
//                pt::ProtoMessage wire;
//                pt::VariantData* item = wire.add_keylistdata()->mutable_listdata()->add_value();
//                item->set_type(type);
//                item->set_data(std::string(length, 'x'));
//                Check(parsed.from(wire.SerializeAsString()) == valid, "type/width validation");
//            }
//        }
//        std::string groups;
//        for (size_t index = 0; index < 65536; ++index)
//        {
//            groups.append("\x12\x00", 2);
//        }
//        Check(parsed.from(groups), "65536 parsed node boundary");
//        groups.append("\x12\x00", 2);
//        Check(!parsed.from(groups) && parsed.getCode() == ProtoMessage::PROTO_ERROR, "65537 nodes accepted");
//        Check(!parsed.from(std::string(1048577, 'x')), "parse size limit");
//        message.clear();
//        message.setKeyValue(1, Variant(std::string(1048577, 'x')));
//        std::string unchanged("unchanged");
//        Check(!message.toString(unchanged) && unchanged == "unchanged", "encode size rollback");
//        message.clear();
//        message.setCode(15, "copy metadata allocation test text");
//        for (int32_t index = 0; index < 5; ++index)
//        {
//            message.setKeyValue(index, Variant(std::string(100, 'a')));
//            message.addListValue(Variant(std::string(80, 'b')));
//        }
//        message.setKeyTable(table);
//        const std::string faultBytes = message.toString();
//        Check(parsed.from(faultBytes), "fault fixture warmup");
//        parsed.clear();
//#if defined(_MSC_VER) && defined(_DEBUG)
//        static uint32_t s_position = 0;
//        static uint32_t s_seen = 0;
//        // 仅拒绝本轮第N次normal分配，避免测试自身记录和CRT元数据干扰。
//        const auto Hook = [](int operation, void*, size_t, int block, long, const unsigned char*, int) -> int
//        {
//            return operation == _HOOK_ALLOC && block == _NORMAL_BLOCK && ++s_seen == s_position ? 0 : 1;
//        };
//        uint32_t failures = 0;
//        _CrtMemState before;
//        _CrtMemState after;
//        _CrtMemCheckpoint(&before);
//        for (uint32_t mode = 0; mode < 4; ++mode)
//        {
//            for (s_position = 1; s_position <= 256; ++s_position)
//            {
//                ProtoMessage target;
//                target.setCode(99);
//                target.setKeyValue(91, Variant("old identity"));
//                std::string output("unchanged");
//                s_seen = 0;
//                bool failed = false;
//                bool escaped = false;
//                _CRT_ALLOC_HOOK previous = _CrtSetAllocHook(Hook);
//                try
//                {
//                    if (mode == 0)
//                    {
//                        ProtoMessage copy(message);
//                    }
//                    else if (mode == 1)
//                    {
//                        target = message;
//                    }
//                    else if (mode == 2)
//                    {
//                        failed = !target.from(faultBytes);
//                    }
//                    else
//                    {
//                        failed = !message.toString(output);
//                    }
//                }
//                catch (...)
//                {
//                    escaped = true;
//                    failed = true;
//                }
//                _CrtSetAllocHook(previous);
//                failures += failed ? 1 : 0;
//                Check(mode < 2 || !escaped, "codec exception escaped");
//                if (failed && mode == 1)
//                {
//                    Check(target.getCode() == 99 && target.getKeyValue(91, value), "assignment partial commit");
//                }
//                if (failed && mode == 2)
//                {
//                    Check(target.getCode() == ProtoMessage::PROTO_ERROR && !target.getKeyValue(91, value),
//                        "fault parse retained identity");
//                }
//                if (failed && mode == 3)
//                {
//                    Check(output == "unchanged", "failed encode touched output");
//                }
//            }
//        }
//        value.clear();
//        _CrtMemCheckpoint(&after);
//        Check(failures > 40, "failure injection did not reach allocation paths");
//        Check(after.lCounts[_NORMAL_BLOCK] == before.lCounts[_NORMAL_BLOCK] &&
//            after.lSizes[_NORMAL_BLOCK] == before.lSizes[_NORMAL_BLOCK], "codec/copy fault allocation leak");
//        Check(message.toString() == faultBytes, "fault source mutated");
//
//        {
//            google::protobuf::RepeatedPtrField<std::string> repeated;
//            for (uint32_t index = 0; index < 4; ++index)
//            {
//                *repeated.Add() = std::string(100, 'x');
//            }
//            uint32_t vendorFailures = 0;
//            _CrtMemCheckpoint(&before);
//            for (uint32_t mode = 0; mode < 4; ++mode)
//            {
//                for (s_position = 1; s_position <= 64; ++s_position)
//                {
//                    google::protobuf::RepeatedPtrField<std::string> target;
//                    *target.Add() = "original";
//                    if (mode == 3)
//                    {
//                        target.MergeFrom(repeated);
//                        target.Clear();
//                        *target.Add() = "original";
//                    }
//                    s_seen = 0;
//                    bool failed = false;
//                    _CRT_ALLOC_HOOK previous = _CrtSetAllocHook(Hook);
//                    try
//                    {
//                        if (mode == 0 || mode == 3)
//                        {
//                            target.MergeFrom(repeated);
//                        }
//                        else if (mode == 1)
//                        {
//                            google::protobuf::RepeatedPtrField<std::string> copy(repeated);
//                        }
//                        else
//                        {
//                            google::protobuf::RepeatedPtrField<std::string> copy(repeated.begin(), repeated.end());
//                        }
//                    }
//                    catch (...)
//                    {
//                        ++vendorFailures;
//                        failed = true;
//                    }
//                    _CrtSetAllocHook(previous);
//                    Check(target.Get(0) == "original", "vendor merge overwrote active element");
//                    if (failed && (mode == 0 || mode == 3))
//                    {
//                        Check(target.size() == 1 && target.Add()->empty(), "vendor failure left dirty reuse slot");
//                    }
//                    target.Clear();
//                    target.MergeFrom(repeated);
//                    Check(target.size() == 4 && target.Get(3) == repeated.Get(3), "vendor recovery after allocation fault");
//                }
//            }
//            _CrtMemCheckpoint(&after);
//            Check(vendorFailures >= 20 && after.lCounts[_NORMAL_BLOCK] == before.lCounts[_NORMAL_BLOCK] &&
//                after.lSizes[_NORMAL_BLOCK] == before.lSizes[_NORMAL_BLOCK], "vendor repeated ownership leaked");
//        }
//
//#endif
//#endif
//        std::printf("ProtoMessage ownership checks=%u PASS\n", checks);
//        return 0;
//    }
//    catch (const std::exception& error)
//    {
//        std::printf("ProtoMessage checks=%u FAIL %s\n", checks, error.what());
//        return 1;
//    }
//    catch (...)
//    {
//        std::printf("ProtoMessage checks=%u FAIL unexpected exception\n", checks);
//        return 1;
//    }
//}
// PROTO_MESSAGE_OWNERSHIP_UNIT_END

// PROTO_MESSAGE_COLD_INIT_UNIT_BEGIN
// 独立Debug main逐次注入冷启动失败；每个分配位置使用新进程，不能提前初始化protobuf。
// #include <google/protobuf/generated_message_util.h>
// #include <crtdbg.h>
// #include <cstdio>
// #include <cstdlib>
// static int remaining = -1;
// static int __cdecl reject(int operation, void*, size_t, int block, long, const unsigned char*, int)
// {
//     if (block == _NORMAL_BLOCK && (operation == _HOOK_ALLOC || operation == _HOOK_REALLOC))
//     {
//         if (remaining == 0) return 0;
//         if (remaining > 0) --remaining;
//     }
//     return 1;
// }
// int main(int argc, char** argv)
// {
//     if (argc != 2) return 2;
//     const int step = std::atoi(argv[1]);
//     _CrtMemState before, after;
//     _CrtMemCheckpoint(&before);
//     remaining = step;
//     bool caught = false;
//     const _CRT_ALLOC_HOOK previous = _CrtSetAllocHook(reject);
//     try { google::protobuf::internal::GetEmptyString(); }
//     catch (...) { caught = true; }
//     _CrtSetAllocHook(previous);
//     remaining = -1;
//     if (!google::protobuf::internal::GetEmptyString().empty()) return 3;
//     google::protobuf::ShutdownProtobufLibrary();
//     _CrtMemCheckpoint(&after);
//     const long long blocks = static_cast<long long>(after.lCounts[_NORMAL_BLOCK]) - before.lCounts[_NORMAL_BLOCK];
//     const long long bytes = static_cast<long long>(after.lSizes[_NORMAL_BLOCK]) - before.lSizes[_NORMAL_BLOCK];
//     const bool pass = blocks == 0 && bytes == 0;
//     std::printf("cold defaults step=%d caught=%d blocks=%lld bytes=%lld %s\n", step, caught, blocks, bytes, pass ? "PASS" : "FAIL");
//     return pass ? 0 : 1;
// }
// PROTO_MESSAGE_COLD_INIT_UNIT_END