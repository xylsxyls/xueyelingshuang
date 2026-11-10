#include "ProtoMessageTable.h"
#include "ProtoMessageProtobuf.h"
#include <utility>

ProtoMessageTable::ProtoMessageTable() :
m_columns(0), m_rows(0)
{

}

ProtoMessageTable::ProtoMessageTable(const ProtoMessageTable& other) :
m_columns(other.m_columns), m_rows(other.m_rows), m_values(other.m_values)
{

}

ProtoMessageTable& ProtoMessageTable::operator=(const ProtoMessageTable& other)
{
    if (this != &other)
    {
        ProtoMessageTable copy(other);
        m_values.swap(copy.m_values);
        std::swap(m_columns, copy.m_columns);
        std::swap(m_rows, copy.m_rows);
    }
    return *this;
}

ProtoMessageTable::~ProtoMessageTable()
{

}

ProtoMessageKind ProtoMessageTable::kind() const
{
    return PROTO_MESSAGE_TABLE;
}

void ProtoMessageTable::clear()
{
    m_values.clear();
    m_rows = 0;
}

bool ProtoMessageTable::setColumns(size_t columns)
{
    if (m_rows != 0 || columns > 65534)
    {
        return false;
    }
    m_columns = columns;
    return true;
}

bool ProtoMessageTable::reserveRows(size_t rows)
{
    if (rows > 65535 / (m_columns + 1))
    {
        return false;
    }
    try
    {
        m_values.reserve(rows * m_columns);
        return true;
    }
    catch (...)
    {
        return false;
    }
}

bool ProtoMessageTable::appendRow(const Variant* values, size_t count)
{
    if (count != m_columns || (values == nullptr && count != 0) || m_rows >= 65535 / (m_columns + 1))
    {
        return false;
    }
    for (size_t i = 0; i < count; ++i)
    {
        if (!ProtoMessageProtobuf::validValue(values[i]))
        {
            return false;
        }
    }
    const size_t previous = m_values.size();
    try
    {
        // 已预留容量时直接逐值复制；只有扩容时临时保留输入行，保护本表借用
        const size_t required = previous + count;
        const bool grows = required > m_values.capacity();
        std::vector<Variant> row;
        if (grows)
        {
            row.reserve(count);
            for (size_t i = 0; i < count; ++i)
            {
                row.push_back(values[i]);
            }
            size_t capacity = m_values.capacity() + m_values.capacity() / 2;
            if (capacity < required)
            {
                capacity = required;
            }
            if (capacity > 65535)
            {
                capacity = 65535;
            }
            m_values.reserve(capacity);
        }
        for (size_t i = 0; i < count; ++i)
        {
            if (grows)
            {
                m_values.push_back(std::move(row[i]));
            }
            else
            {
                m_values.push_back(values[i]);
            }
        }
        ++m_rows;
        return true;
    }
    catch (...)
    {
        while (m_values.size() > previous)
        {
            m_values.pop_back();
        }
        return false;
    }
}

const Variant* ProtoMessageTable::at(size_t row, size_t column) const
{
    return row < m_rows && column < m_columns ? &m_values[row * m_columns + column] : nullptr;
}

size_t ProtoMessageTable::columns() const
{
    return m_columns;
}

size_t ProtoMessageTable::rows() const
{
    return m_rows;
}

/** 构建规则表，省去逐行子消息；行列乘法先受节点预算约束
@param [in] columns 固定列数
@param [in] rows 完整行数量
@param [in] values 行优先的稳定单元格
@param [out] output 临时protobuf信封
@return 表格完整且预算满足要求
*/
static bool BuildTable(size_t columns, size_t rows, const std::vector<Variant>& values, pt::ProtoSpecializedMessage& output)
{
    if (columns > 65534 || rows > 65535 / (columns + 1) || values.size() != rows * columns)
    {
        return false;
    }
    pt::ProtoMessageTable* target = output.mutable_table();
    target->set_columns(static_cast<uint32_t>(columns));
    target->set_rows(static_cast<uint32_t>(rows));
    return ProtoMessageProtobuf::putValues(values, *target->mutable_values());
}

bool ProtoMessageTable::from(const void* data, size_t size)
{
    try
    {
        pt::ProtoSpecializedMessage input;
        if (!ProtoMessageProtobuf::parse(data, size, input) || !ProtoMessageProtobuf::validShape(input) || !input.has_table())
        {
            clear();
            return false;
        }
        const pt::ProtoMessageTable& table = input.table();
        if (table.columns() > 65534 || table.rows() > 65535 / (table.columns() + 1))
        {
            clear();
            return false;
        }
        std::vector<Variant> values;
        if (!ProtoMessageProtobuf::getValues(table.values(), values) || values.size() != table.rows() * table.columns())
        {
            clear();
            return false;
        }
        m_values.swap(values);
        m_rows = table.rows();
        m_columns = table.columns();
        return true;
    }
    catch (...)
    {
        clear();
        return false;
    }
}

ProtoMessageTable* ProtoMessageTable::table()
{
    return this;
}

bool ProtoMessageTable::serializedSize(size_t& size) const
{
    size = 0;
    try
    {
        pt::ProtoSpecializedMessage message;
        return BuildTable(m_columns, m_rows, m_values, message) && ProtoMessageProtobuf::size(message, size);
    }
    catch (...)
    {
        return false;
    }
}

bool ProtoMessageTable::serializeTo(void* output, size_t capacity, size_t& written) const
{
    written = 0;
    try
    {
        pt::ProtoSpecializedMessage message;
        return BuildTable(m_columns, m_rows, m_values, message) && ProtoMessageProtobuf::write(message, output, capacity, written);
    }
    catch (...)
    {
        return false;
    }
}

bool ProtoMessageTable::toString(std::string& output) const
{
    try
    {
        pt::ProtoSpecializedMessage message;
        return BuildTable(m_columns, m_rows, m_values, message) && ProtoMessageProtobuf::string(message, output);
    }
    catch (...)
    {
        return false;
    }
}