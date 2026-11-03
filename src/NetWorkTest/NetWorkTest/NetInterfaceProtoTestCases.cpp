#include "NetInterfaceProtoTestCases.h"
#include "NetWorkTestCases.h"
#include "NetInterface/NetInterfaceAPI.h"
#include "ProtoMessage/ProtoMessageAPI.h"
#include <cstring>
#include <stdexcept>
#include <vector>

/** 用确定的前三次失败验证依赖初始化状态，测试不修改protobuf全局单例
@param [in,out] calls 当前独立初始化器调用次数
*/
static void ProtoRetryInitialization(int* calls)
{
    ++*calls;
    if (*calls <= 3)
    {
        throw *calls;
    }
}

/** 断言独立预期，失败立即由统一runner记录
@param [in] condition 必须成立的条件
@param [in] reason 失败步骤
*/
static void CheckProto(bool condition, const char* reason)
{
    netTestCheckpoint();
    if (!condition)
    {
        throw std::runtime_error(reason);
    }
}

void NetInterfaceProtoTestCases::initializationRecovery()
{
    google::protobuf::GoogleOnceDynamic once;
    int calls = 0;
    uint32_t failures = 0;
    for (uint32_t index = 0; index < 8; ++index)
    {
        try
        {
            once.Init(&ProtoRetryInitialization, &calls);
            CheckProto(index >= 3 && calls == 4, "initialization published partial state");
        }
        catch (int reason)
        {
            CheckProto(index < 3 && reason == static_cast<int>(index + 1), "wrong initializer failure");
            ++failures;
        }
    }
    CheckProto(failures == 3 && calls == 4, "initialization retry count");
}

void NetInterfaceProtoTestCases::wireCompatibility()
{
    CheckProto(GOOGLE_PROTOBUF_VERSION == 3003000, "protobuf runtime version");
    const char rawCode[] = {0x0a, 0x07, 0x08, 0x07, 0x12, 0x03, 'A', 0, 'Z'};
    const unsigned char rawMap[] = {0x12, 0x10, 0x12, 0x0e, 0x0a, 0x0c, 0x08, 0x01,
        0x12, 0x08, 0x0a, 0x04, 0xef, 0xbe, 0xad, 0xde, 0x10, 0x01};
    std::string expected(rawCode, sizeof(rawCode));
    expected.append(reinterpret_cast<const char*>(rawMap), sizeof(rawMap));
    ProtoMessage input;
    input.setCode(7, std::string("A\0Z", 3));
    input.setKeyValue(1, Variant(static_cast<int32_t>(-559038737)));
    CheckProto(input.toString() == expected, "handwritten legacy wire mismatch");
    CheckProto(input.toString() == expected, "serializing twice lost response code");
    ProtoMessage decoded;
    CheckProto(decoded.from(expected), "handwritten message parse");
    CheckProto(decoded.getCode() == 7 && decoded.getErrorMessage() == std::string("A\0Z", 3), "response code lost");
    Variant value;
    CheckProto(decoded.getKeyValue(1, value) && static_cast<int32_t>(value) == -559038737, "numeric bytes changed");
    CheckProto(decoded.toString() == expected, "round trip changed wire");
}

void NetInterfaceProtoTestCases::invalidValues()
{
    for (int location = 0; location < 3; ++location)
    {
        for (int type = -1; type <= 7; ++type)
        {
            for (int size = 0; size <= 9; ++size)
            {
                const bool valid = (type == 0 && size == 0) || (type == 1 && size == 4) ||
                    ((type == 2 || type == 3) && size == 8) || type == 4 || type == 5;
                pt::ProtoMessage wire;
                pt::VariantData* data = nullptr;
                if (location == 0)
                {
                    data = wire.add_keymapdata()->mutable_mapdata()->add_mapkeyvalue()->mutable_value();
                }
                else if (location == 1)
                {
                    data = wire.add_keylistdata()->mutable_listdata()->add_value();
                }
                else
                {
                    data = wire.add_keytabledata()->mutable_tabledata()->add_listdata()->add_value();
                }
                data->set_type(type);
                data->set_data(std::string(size, 'x'));
                ProtoMessage parsed;
                parsed.setKeyValue(91, Variant("old credential"));
                const bool result = parsed.from(wire.SerializeAsString());
                CheckProto(result == valid, "untrusted value accepted/rejected incorrectly");
                Variant old;
                CheckProto(!parsed.getKeyValue(91, old), "old field survived parse");
                if (!valid)
                {
                    CheckProto(parsed.getCode() == ProtoMessage::PROTO_ERROR &&
                        parsed.getList().empty() && parsed.getTable().empty(), "partial malformed state");
                }
            }
        }
    }
}

void NetInterfaceProtoTestCases::ownership()
{
    ProtoMessage original;
    original.setCode(19, "copy");
    original.setKeyValue(1, Variant(uint64_t(0xfedcba9876543210ULL)));
    original.addListValue(Variant(1.25));
    std::vector<std::vector<Variant>> table(1, std::vector<Variant>(1, Variant(std::string("x\0y", 3))));
    original.setKeyTable(table);
    const std::string expected = original.toString();
    ProtoMessage assigned;
    assigned = original;
    original.clear();
    assigned = assigned;
    CheckProto(assigned.toString() == expected, "message copy alias or self assignment");
    ProtoMessage copy(assigned);
    assigned.clear();
    CheckProto(copy.toString() == expected, "copy constructor alias");
    Variant number;
    CheckProto(copy.getKeyValue(1, number), "copied number missing");
    number = number;
    CheckProto(static_cast<uint64_t>(number) == 0xfedcba9876543210ULL, "uint64 precision or self assignment");
    CheckProto(number.toString() == "18364758544493064720", "uint64 decimal platform support");
    Variant floating(1.25);
    CheckProto(floating.toString() == "1.25", "double used integer formatting");
    Variant fromBytes(floating.toBlob(), Variant::DOUBLE, true);
    Variant copied(fromBytes);
    fromBytes.clear();
    CheckProto(static_cast<double>(copied) == 1.25, "typed double blob ownership");
    for (int type = 0; type <= 7; ++type)
    {
        Variant bad(std::string("x"), static_cast<Variant::VariantType>(type), true);
        if (type != 4 && type != 5)
        {
            CheckProto(bad.type() == Variant::INIT, "malformed variant constructed");
        }
    }
    Variant empty;
    std::string destination("stale");
    empty.toString(destination);
    CheckProto(destination.empty(), "empty string retained old output");
    destination = "stale";
    empty.toBlob(destination);
    CheckProto(destination.empty(), "empty blob retained old output");
}

void NetInterfaceProtoTestCases::cBoundary()
{
    NetInterfaceMessageHandle handle = nullptr;
    CheckProto(NetInterface::messageCreate(&handle) == NI_OK, "create C message");
    try
    {
        const uint8_t value[] = {1, 0, 2, 255};
        CheckProto(NetInterface::messageSetValue(handle, 2, 5, 5, value, sizeof(value)) == NI_OK, "set binary field");
        CheckProto(NetInterface::messageSetCode(handle, 123, value, 3) == NI_OK, "set response code");
        uint32_t required = 0;
        CheckProto(NetInterface::messageEncode(handle, nullptr, 0, &required) == NI_BUFFER_TOO_SMALL && required > 0, "query encoded size");
        std::vector<uint8_t> output(required, 0x5a);
        CheckProto(NetInterface::messageEncode(handle, &output[0], required - 1, &required) == NI_BUFFER_TOO_SMALL &&
            output[0] == 0x5a && output.back() == 0x5a, "short buffer modified");
        CheckProto(NetInterface::messageEncode(handle, &output[0], static_cast<uint32_t>(output.size()), &required) == NI_OK, "encode C message");
        CheckProto(NetInterface::messageParse(handle, &output[0], required) == NI_OK, "parse C message");
        int32_t type = 0;
        uint8_t read[4] = {};
        CheckProto(NetInterface::messageGetValue(handle, 2, 5, &type, read, 4, &required) == NI_OK &&
            type == 5 && required == 4 && std::memcmp(read, value, 4) == 0, "read C binary value");
        int32_t code = 0;
        CheckProto(NetInterface::messageGetCode(handle, &code, read, 4, &required) == NI_OK && code == 123 && required == 3, "read C code");
        CheckProto(NetInterface::messageSetValue(handle, 2, 5, 1, value, 3) == NI_INVALID_ARGUMENT, "bad numeric width accepted");
        CheckProto(NetInterface::messageGetValue(handle, 2, 5, &type, read, 4, &required) == NI_OK && type == 5, "bad set changed old value");
        CheckProto(NetInterface::messageParse(handle, nullptr, 1) == NI_INVALID_ARGUMENT, "null body accepted");
        CheckProto(NetInterface::messageGetValue(handle, 2, 5, &type, read, 4, &required) == NI_NOT_FOUND, "failed parse retained data");
        CheckProto(NetInterface::messageClear(handle) == NI_OK, "clear C message");
        int32_t result = NI_OK;
        for (int index = 0; index < 17000 && result == NI_OK; ++index)
        {
            result = NetInterface::messageSetValue(handle, 0, index, 0, nullptr, 0);
        }
        CheckProto(result == NI_LIMIT_EXCEEDED, "unbounded C field accumulation");
        CheckProto(NetInterface::messageCreate(nullptr) == NI_INVALID_ARGUMENT &&
            NetInterface::messageParse(nullptr, nullptr, 0) == NI_INVALID_ARGUMENT &&
            NetInterface::messageEncode(nullptr, nullptr, 0, &required) == NI_INVALID_ARGUMENT, "null C handles");
    }
    catch (...)
    {
        NetInterface::messageDestroy(handle);
        throw;
    }
    NetInterface::messageDestroy(handle);
    NetInterface::messageDestroy(nullptr);
}

void NetInterfaceProtoTestCases::limits()
{
    ProtoMessage message;
    CheckProto(!message.from(std::string(1048577, 'x')), "oversize protobuf accepted");
    std::string groups;
    for (int index = 0; index < 65536; ++index)
    {
        groups.append("\x12\x00", 2);
    }
    CheckProto(message.from(groups), "entry boundary rejected");
    groups.append("\x12\x00", 2);
    CheckProto(!message.from(groups), "entry bound not enforced");
    std::string recursion(17, '\x4b');
    recursion.append(17, '\x4c');
    CheckProto(!message.from(recursion), "unknown group recursion not bounded");
    CheckProto(!message.from(std::string("\x0a\x07\x08", 3)), "truncated message accepted");
    CheckProto(message.from("") && message.getCode() == 0, "empty message compatibility");
}

void NetInterfaceProtoTestCases::atomicContainers()
{
    ProtoMessage message;
    std::map<int32_t, Variant> emptyMap;
    std::list<Variant> emptyList;
    std::vector<std::vector<Variant>> emptyTable;
    message.setKeyMap(emptyMap, 11);
    message.setKeyList(emptyList, 12);
    message.setKeyTable(emptyTable, 13);
    std::vector<std::vector<Variant>> rows(3);
    rows[1].push_back(Variant(std::string("a\0b", 3)));
    message.setKeyTable(rows);
    message.setCode(71, "status");
    const std::string encoded = message.toString();
    ProtoMessage decoded;
    CheckProto(decoded.from(encoded), "empty container decode");
    CheckProto(decoded.getMap(emptyMap, 11) && emptyMap.empty(), "empty map existence lost");
    CheckProto(decoded.getList(emptyList, 12) && emptyList.empty(), "empty list existence lost");
    CheckProto(decoded.getTable(emptyTable, 13) && emptyTable.empty(), "empty table existence lost");
    CheckProto(decoded.getTable(emptyTable) && emptyTable.size() == 3 && emptyTable[0].empty() &&
        emptyTable[2].empty() && emptyTable[1][0].toBlob() == std::string("a\0b", 3), "empty rows lost");
    for (uint32_t index = 0; index < 256; ++index)
    {
        ProtoMessage copy(decoded);
        copy = copy;
        CheckProto(copy.toString() == encoded, "repeat encoding changed owned message");
        copy.clearMap();
        copy.clearList();
        copy.clearTable();
        CheckProto(copy.getCode() == 71 && copy.getMap().empty() && copy.getList().empty() &&
            copy.getTable().empty(), "selective clear retained duplicate protobuf payload");
    }
    std::string unchanged("unchanged");
    message.setKeyValue(1, Variant(std::string(1048577, 'x')));
    CheckProto(!message.toString(unchanged) && unchanged == "unchanged", "oversize encoding partially committed");
    CheckProto(!decoded.from(std::string("\x0a\x7f", 2)) && decoded.getCode() == ProtoMessage::PROTO_ERROR,
        "decode failure status");
    CheckProto(!decoded.getMap(emptyMap, 11), "decode failure retained previous empty container");

    const char* truncated[] = {"\x0a\x7f", "\x12\x7f", "\x12\x02\x12\x7f", "\x1a\x02\x12\x7f",
        "\x22\x02\x12\x7f", "\x0a\x02\x12\x7f"};
    const uint32_t lengths[] = {2, 2, 4, 4, 4, 4};
    for (uint32_t index = 0; index < 6; ++index)
    {
        decoded.setKeyValue(91, Variant("old identity"));
        CheckProto(!decoded.from(std::string(truncated[index], lengths[index])) &&
            decoded.getCode() == ProtoMessage::PROTO_ERROR && decoded.getMap().empty(), "truncated nested protobuf accepted");
    }

    const char unknown[] = {0x0a, 0x04, 0x08, 0x09, 0x18, 0x07, static_cast<char>(0xa0), 0x06, 0x01};
    CheckProto(decoded.from(std::string(unknown, sizeof(unknown))) &&
        decoded.toString() == std::string("\x0a\x02\x08\x09", 4), "protobuf3.3 unknown skip behavior changed");
}