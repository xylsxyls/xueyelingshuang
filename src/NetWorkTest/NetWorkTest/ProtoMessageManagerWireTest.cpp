#include "ProtoMessageManager/ProtoMessageManagerAPI.h"
#include <cstdio>
#include <memory>
#include <fstream>
#include <limits>
#include <iterator>

/** 生成确定性消息样本，可与独立版本生成的基准逐字节比较
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
            if (!input || input.bad() || actual != expected)
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