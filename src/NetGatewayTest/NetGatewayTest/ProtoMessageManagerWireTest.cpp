#include "ProtoMessageManager/ProtoMessageManagerAPI.h"
#include <cstdio>
#include <memory>
#include <fstream>
#include <limits>
#include <iterator>
#include <algorithm>
#include <cstring>
#include <vector>

/** 读取样本中标签为 1 的长度分隔字段，只接受生成器使用的最短长度编码。
@param [in] bytes 当前消息
@param [in,out] pos 下一字段的起点，成功后指向字段末尾
@param [out] value 字段的原始正文
@return 标签、长度和边界均正确时返回 true
*/
static bool ReadField(const std::string& bytes, size_t& pos, std::string& value)
{
    if (pos == bytes.size() || bytes[pos++] != '\x0a') { return false; }
    uint32_t length = 0;
    for (unsigned shift = 0; shift <= 28 && pos < bytes.size(); shift += 7)
    {
        const unsigned byte = static_cast<unsigned char>(bytes[pos++]);
        if (shift == 28 && byte > 15) { return false; }
        length |= (byte & 127u) << shift;
        if (byte < 128)
        {
            if ((shift != 0 && byte == 0) || length > bytes.size() - pos) { return false; }
            value.assign(bytes, pos, length);
            pos += length;
            return true;
        }
    }
    return false;
}

/** 提取 Record 原始键值字段并排序，保留每个字段的字节和出现次数。
@param [in] bytes 单条 Record 的协议正文
@param [out] fields 排序后的原始字段
@return 只有 Record 外层和全部字段完整时返回 true
*/
static bool RecordFields(const std::string& bytes, std::vector<std::string>& fields)
{
    size_t pos = 0;
    std::string record, field;
    if (!ReadField(bytes, pos, record) || pos != bytes.size()) { return false; }
    pos = 0;
    while (pos != record.size())
    {
        if (!ReadField(record, pos, field)) { return false; }
        fields.push_back(field);
    }
    std::sort(fields.begin(), fields.end());
    return true;
}

/** 比较 1024 条带长度的样本。Record 的字段顺序未定义，其他类型仍逐字节核对。
@param [in] actual 本次输出
@param [in] expected 独立基准
@return 全部样本一致且无尾随数据时返回 true
*/
static bool SameSamples(const std::vector<char>& actual, const std::vector<char>& expected)
{
    if (actual.size() != expected.size()) { return false; }
    size_t pos = 0;
    for (int i = 0; i < 1024; ++i)
    {
        uint32_t length = 0;
        if (actual.size() - pos < sizeof(length) ||
            std::memcmp(actual.data() + pos, expected.data() + pos, sizeof(length)) != 0) { return false; }
        std::memcpy(&length, actual.data() + pos, sizeof(length));
        pos += sizeof(length);
        if (length > actual.size() - pos) { return false; }
        if (length && std::memcmp(actual.data() + pos, expected.data() + pos, length) != 0)
        {
            if (i / 256 != PROTO_MESSAGE_RECORD) { return false; }
            std::vector<std::string> left, right;
            if (!RecordFields(std::string(actual.data() + pos, length), left) ||
                !RecordFields(std::string(expected.data() + pos, length), right) || left != right) { return false; }
        }
        pos += length;
    }
    return pos == actual.size();
}

/** 生成四种消息的固定样本，与独立基准核对格式、字段字节及有序容器的顺序
@param [in] argc 参数个数，要求输出文件名，可另传只读基准文件
@param [in] argv 测试输出路径和可选基准路径，不使用正式业务文件
@return 0表示生成或比较成功，非0表示构造、编码、文件或比较失败；只生成不证明兼容
*/
int main(int argc, char** argv)
{
    if (argc != 2 && argc != 3)
    {
        return 1;
    }
    try
    {
        std::vector<char> expected;
        if (argc == 3)
        {
            std::ifstream baseline(argv[2], std::ios::binary);
            if (!baseline)
            {
                return 12;
            }
            expected.assign(std::istreambuf_iterator<char>(baseline), std::istreambuf_iterator<char>());
            if (baseline.bad() || expected.empty())
            {
                return 12;
            }
        }
        std::ofstream output(argv[1], std::ios::binary | std::ios::trunc);
        if (!output)
        {
            return 2;
        }
        typedef std::unique_ptr<ProtoMessageDocument, void (*)(ProtoMessageDocument*)> MessageOwner;
        for (int32_t kind = 0; kind < 4; ++kind)
        {
            for (int32_t sample = 0; sample < 256; ++sample)
            {
                MessageOwner message(ProtoMessageManager::create(static_cast<ProtoMessageKind>(kind)), &ProtoMessageManager::release);
                if (!message)
                {
                    return 3;
                }
                std::vector<Variant> values;
                values.push_back(Variant());
                values.push_back(Variant(int32_t(0)));
                values.push_back(Variant((std::numeric_limits<int32_t>::min)()));
                values.push_back(Variant((std::numeric_limits<uint64_t>::max)()));
                values.push_back(Variant(sample));
                values.push_back(Variant(""));
                values.push_back(Variant(std::string()));
                Variant text;
                const char data[] = {'a', '\0', 'b'};
                if (!text.assignBytes(data, sizeof(data), Variant::STRING))
                {
                    return 4;
                }
                values.push_back(text);
                if (message->table() && (!message->table()->setColumns(values.size()) || !message->table()->appendRow(values.data(), values.size())))
                {
                    return 5;
                }
                for (size_t i = 0; i < values.size(); ++i)
                {
                    const int32_t key = static_cast<int32_t>(i) - sample % 3;
                    if (message->general())
                    {
                        message->general()->setKeyValue(key, values[i], sample % 5 - 2);
                        message->general()->addListValue(values[i], sample % 3 - 1);
                    }
                    if (message->record() && !message->record()->setValue(key, values[i]))
                    {
                        return 6;
                    }
                    if (message->array() && !message->array()->append(values[i]))
                    {
                        return 7;
                    }
                }
                if (message->general())
                {
                    // 覆盖未设置响应、显式零响应、空容器、零键与非零键
                    if (sample % 4 != 0)
                    {
                        message->general()->setCode(sample % 3 - 1, sample % 2 == 0 ? "" : "status");
                    }
                    message->general()->setKeyMap(std::map<int32_t, Variant>(), 9);
                    message->general()->setKeyList(std::list<Variant>(), 9);
                    std::vector<std::vector<Variant>> rows;
                    rows.push_back(std::vector<Variant>());
                    rows.push_back(values);
                    message->general()->setKeyTable(rows, sample % 2);
                }
                std::string bytes;
                if (!message->toString(bytes))
                {
                    return 8;
                }
                const uint32_t length = static_cast<uint32_t>(bytes.size());
                // 长度仅是测试样本文件的分隔，不属于消息协议
                output.write(reinterpret_cast<const char*>(&length), sizeof(length));
                output.write(bytes.data(), bytes.size());
                if (!output)
                {
                    return 9;
                }
            }
        }
        output.close();
        if (!output)
        {
            return 10;
        }
        if (argc == 3)
        {
            std::ifstream input(argv[1], std::ios::binary);
            const std::vector<char> actual((std::istreambuf_iterator<char>(input)), std::istreambuf_iterator<char>());
            if (!input || input.bad() || !SameSamples(actual, expected))
            {
                return 13;
            }
            std::printf("WIRE_COMPATIBILITY_PASS 1024 samples, %llu bytes\n", static_cast<unsigned long long>(actual.size()));
        }
        else
        {
            std::printf("WIRE_SAMPLES_GENERATED 1024; compatibility comparison not performed\n");
        }
        return 0;
    }
    catch (...)
    {
        return 11;
    }
}
