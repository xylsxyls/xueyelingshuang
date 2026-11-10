#include "ProtoMessage.h"
#include "ProtoMessage.pb.h"
#include "ProtoMessageProtobuf.h"
#include "google/protobuf/io/coded_stream.h"
#include <utility>
#include <cstring>

#if defined(_MSC_VER)
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

const int32_t ProtoMessage::PROTO_ERROR = -2;

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

ProtoMessage& ProtoMessage::operator=(const Variant& value)
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
    Variant copy(dataValue);
    setKeyValue(dataKey, std::move(copy), key);
}

bool ProtoMessage::getKeyValue(int32_t dataKey, Variant& value, int32_t key) const
{
    const Variant* found = findValue(dataKey, key);
    if (found != nullptr)
    {
        try
        {
            value = *found;
            return true;
        }
        catch (...)
        {
            // 保留原接口失败清空输出的约定，不返回部分字段。
        }
    }
    value.clear();
    return false;
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
    return from(data.data(), data.size());
}

bool ProtoMessage::from(const void* data, size_t size)
{
    try
    {
        if (size > 1048576 || (data == nullptr && size != 0))
        {
            return fail();
        }
        ProtoMessage decoded;
        pt::ProtoMessage parsed;
        const uint8_t empty = 0;
        google::protobuf::io::CodedInputStream input(
            size == 0 ? &empty : static_cast<const uint8_t*>(data), static_cast<int32_t>(size));
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
                target.emplace_back(value.data(), static_cast<Variant::VariantType>(value.type()), true);
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
                    rows[row].emplace_back(value.data(), static_cast<Variant::VariantType>(value.type()), true);
                }
            }
            decoded.m_keyTableData[group.key()].swap(rows);
        }
        // 仅提交已知业务字段，protobuf未知字段随临时解析对象释放，不长期保留。
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

void ProtoMessage::clear()
{
    // VS2013 Debug的空string也可能为迭代器代理分配；清理路径不能再次抛出
    m_error.clear();
    try
    {
        std::string empty;
        m_error.swap(empty);
    }
    catch (...)
    {
        // 持续内存不足时保留空字符串容量，内容已清空，析构仍负责最终释放
    }
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

ProtoMessage::ProtoMessage(ProtoMessage&& other) :
ProtoMessage()
{
    swap(other);
}

ProtoMessage& ProtoMessage::operator=(ProtoMessage&& other)
{
    if (this != &other)
    {
        clear();
        swap(other);
    }
    return *this;
}

ProtoMessage& ProtoMessage::operator=(Variant&& value)
{
    const int32_t key = m_key == -1 ? PROTO_GLOBAL : m_key;
    const int32_t dataKey = m_dataKey;
    m_key = -1;
    m_dataKey = -1;
    setKeyValue(dataKey, std::move(value), key);
    return *this;
}

void ProtoMessage::setKeyValue(const int32_t& dataKey, Variant&& dataValue, const int32_t& key)
{
    auto group = m_keyMapData.find(key);
    if (group == m_keyMapData.end())
    {
        // 独立准备整组再提交，外层节点分配失败不遗留空组。
        std::map<int32_t, Variant> values;
        values.emplace(dataKey, std::move(dataValue));
        m_keyMapData.emplace(key, std::move(values));
        return;
    }
    auto item = group->second.find(dataKey);
    if (item == group->second.end())
    {
        group->second.emplace(dataKey, std::move(dataValue));
    }
    else
    {
        item->second = std::move(dataValue);
    }
}

bool ProtoMessage::setBytes(int32_t dataKey, const void* data, size_t size, Variant::VariantType type, int32_t key)
{
    if (size > 1048576)
    {
        return false;
    }
    Variant value;
    if (!value.assignBytes(data, size, type))
    {
        return false;
    }
    try
    {
        setKeyValue(dataKey, std::move(value), key);
        return true;
    }
    catch (...)
    {
        return false;
    }
}

const Variant* ProtoMessage::findValue(int32_t dataKey, int32_t key) const
{
    const std::map<int32_t, Variant>* values = findMap(key);
    if (values == nullptr)
    {
        return nullptr;
    }
    const auto item = values->find(dataKey);
    return item == values->end() ? nullptr : &item->second;
}

const std::map<int32_t, Variant>* ProtoMessage::findMap(int32_t key) const
{
    const auto group = m_keyMapData.find(key);
    return group == m_keyMapData.end() ? nullptr : &group->second;
}

const std::list<Variant>* ProtoMessage::findList(int32_t key) const
{
    const auto group = m_keyListData.find(key);
    return group == m_keyListData.end() ? nullptr : &group->second;
}

const std::vector<std::vector<Variant>>* ProtoMessage::findTable(int32_t key) const
{
    const auto group = m_keyTableData.find(key);
    return group == m_keyTableData.end() ? nullptr : &group->second;
}

/** 将原始Variant填入历史protobuf值，保留原有字节约定
@param [in] value 稳定借用
@param [out] output 新建的生成消息
@param [in,out] bytes 累计原始正文预算
@return 不超过1MiB且字段合法
*/
static bool PutLegacyValue(const Variant& value, pt::VariantData& output, size_t& bytes)
{
    if (!ProtoMessageProtobuf::validValue(value) || value.size() > 1048576 - bytes)
    {
        return false;
    }
    bytes += value.size();
    if (value.type() != Variant::INIT)
    {
        output.set_type(value.type());
    }
    if (value.size() != 0)
    {
        output.mutable_data()->assign(value.data(), value.size());
    }
    return true;
}

/** 构建通用消息的临时protobuf对象，不实现编码算法
@param [in] maps 有序字段组
@param [in] lists 顺序列表组
@param [in] tables 按行表格组
@param [in] hasCode 是否显式设置响应码
@param [in] code 响应码
@param [in] error 响应文本
@param [out] output 新建的生成消息
@return 节点及原始字节预算满足要求；分配异常由调用入口捕获
*/
static bool BuildGeneral(const std::map<int32_t, std::map<int32_t, Variant>>& maps,
    const std::map<int32_t, std::list<Variant>>& lists,
    const std::map<int32_t, std::vector<std::vector<Variant>>>& tables,
    bool hasCode, int32_t code, const std::string& error, pt::ProtoMessage& output)
{
    if (error.size() > 1048576 || maps.size() > 65536 || lists.size() > 65536 || tables.size() > 65536)
    {
        return false;
    }
    size_t nodes = maps.size() + lists.size() + tables.size();
    size_t bytes = error.size();
    if (nodes > 65536)
    {
        return false;
    }
    if (hasCode)
    {
        pt::RspCode* response = output.mutable_rspcode();
        if (code != 0)
        {
            response->set_code(code);
        }
        if (!error.empty())
        {
            response->set_errormessage(error);
        }
    }
    for (auto group = maps.begin(); group != maps.end(); ++group)
    {
        if (group->second.size() > 65536 - nodes)
        {
            return false;
        }
        nodes += group->second.size();
        pt::KeyMapData* target = output.add_keymapdata();
        if (group->first != 0)
        {
            target->set_key(group->first);
        }
        pt::MapData* data = target->mutable_mapdata();
        data->mutable_mapkeyvalue()->Reserve(static_cast<int32_t>(group->second.size()));
        for (auto value = group->second.begin(); value != group->second.end(); ++value)
        {
            pt::MapKeyValue* field = data->add_mapkeyvalue();
            if (value->first != 0)
            {
                field->set_key(value->first);
            }
            if (!PutLegacyValue(value->second, *field->mutable_value(), bytes))
            {
                return false;
            }
        }
    }
    for (auto group = lists.begin(); group != lists.end(); ++group)
    {
        if (group->second.size() > 65536 - nodes)
        {
            return false;
        }
        nodes += group->second.size();
        pt::KeyListData* target = output.add_keylistdata();
        if (group->first != 0)
        {
            target->set_key(group->first);
        }
        pt::ListData* data = target->mutable_listdata();
        data->mutable_value()->Reserve(static_cast<int32_t>(group->second.size()));
        for (auto value = group->second.begin(); value != group->second.end(); ++value)
        {
            if (!PutLegacyValue(*value, *data->add_value(), bytes))
            {
                return false;
            }
        }
    }
    for (auto group = tables.begin(); group != tables.end(); ++group)
    {
        if (group->second.size() > 65536 - nodes)
        {
            return false;
        }
        nodes += group->second.size();
        pt::KeyTableData* target = output.add_keytabledata();
        if (group->first != 0)
        {
            target->set_key(group->first);
        }
        pt::TableData* data = target->mutable_tabledata();
        data->mutable_listdata()->Reserve(static_cast<int32_t>(group->second.size()));
        for (size_t row = 0; row < group->second.size(); ++row)
        {
            const std::vector<Variant>& values = group->second[row];
            if (values.size() > 65536 - nodes)
            {
                return false;
            }
            nodes += values.size();
            pt::ListData* list = data->add_listdata();
            list->mutable_value()->Reserve(static_cast<int32_t>(values.size()));
            for (size_t column = 0; column < values.size(); ++column)
            {
                if (!PutLegacyValue(values[column], *list->add_value(), bytes))
                {
                    return false;
                }
            }
        }
    }
    return true;
}

ProtoMessageKind ProtoMessage::kind() const
{
    return PROTO_MESSAGE_GENERAL;
}

ProtoMessage* ProtoMessage::general()
{
    return this;
}

std::string ProtoMessage::toString() const
{
    std::string result;
    toString(result);
    return result;
}

bool ProtoMessage::serializedSize(size_t& size) const
{
    size = 0;
    try
    {
        pt::ProtoMessage message;
        return BuildGeneral(m_keyMapData, m_keyListData, m_keyTableData, m_hasCode, m_code, m_error, message) && ProtoMessageProtobuf::size(message, size);
    }
    catch (...)
    {
        return false;
    }
}

bool ProtoMessage::serializeTo(void* output, size_t capacity, size_t& written) const
{
    written = 0;
    try
    {
        pt::ProtoMessage message;
        return BuildGeneral(m_keyMapData, m_keyListData, m_keyTableData, m_hasCode, m_code, m_error, message) && ProtoMessageProtobuf::write(message, output, capacity, written);
    }
    catch (...)
    {
        return false;
    }
}

bool ProtoMessage::toString(std::string& output) const
{
    try
    {
        pt::ProtoMessage message;
        return BuildGeneral(m_keyMapData, m_keyListData, m_keyTableData, m_hasCode, m_code, m_error, message) && ProtoMessageProtobuf::string(message, output);
    }
    catch (...)
    {
        return false;
    }
}