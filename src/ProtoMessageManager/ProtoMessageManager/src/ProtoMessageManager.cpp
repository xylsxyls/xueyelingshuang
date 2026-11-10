#include "ProtoMessageManager.h"
#include "ProtoMessage.h"
#include "ProtoMessageRecord.h"
#include "ProtoMessageArray.h"
#include "ProtoMessageTable.h"

ProtoMessageDocument* ProtoMessageManager::create(ProtoMessageKind kind)
{
    try
    {
        switch (kind)
        {
        case PROTO_MESSAGE_GENERAL:
            return new ProtoMessage();
        case PROTO_MESSAGE_RECORD:
            return new ProtoMessageRecord();
        case PROTO_MESSAGE_ARRAY:
            return new ProtoMessageArray();
        case PROTO_MESSAGE_TABLE:
            return new ProtoMessageTable();
        default:
            return nullptr;
        }
    }
    catch (...)
    {
        return nullptr;
    }
}

void ProtoMessageManager::release(ProtoMessageDocument* message)
{
    delete message;
}

// 使用示例：将下面注释展开为独立消费程序的main；正式库不编译此段
//#include "ProtoMessageManager/ProtoMessageManagerAPI.h"
//#include <cstdio>
//#include <memory>
//
///** 演示工厂创建四种消息、写字段、序列化、读取及自动释放
//@return 0表示演示完成；1表示创建/编解码失败，2表示异常
//*/
//int main()
//{
//    try
//    {
//        // 工厂对象在创建库内释放，所有借用指针在owner销毁前使用
//        typedef std::unique_ptr<ProtoMessageDocument, void (*)(ProtoMessageDocument*)> MessageOwner;
//        MessageOwner request(ProtoMessageManager::create(PROTO_MESSAGE_RECORD), &ProtoMessageManager::release);
//        MessageOwner received(ProtoMessageManager::create(PROTO_MESSAGE_RECORD), &ProtoMessageManager::release);
//        if (!request || !received)
//        {
//            return 1;
//        }
//
//        // 单组动态字段：字段号由业务双方约定，不需要修改proto
//        ProtoMessageRecord* fields = request->record();
//        const char product[] = "LumaPlayer";
//        if (!fields->reserve(3) || !fields->setValue(1, Variant(uint64_t(10001))) ||
//            !fields->setBytes(2, product, sizeof(product) - 1, Variant::STRING) ||
//            !fields->setValue(3, Variant(int32_t(2))))
//        {
//            return 1;
//        }
//
//        // 生产代码在这里通过NetSender发送实际written字节；本例直接交给接收对象
//        char output[2048];
//        size_t written = 0;
//        if (!request->serializeTo(output, sizeof(output), written) || !received->from(output, written))
//        {
//            return 1;
//        }
//        const Variant* userId = received->record()->findValue(1);
//        if (userId == nullptr || userId->type() != Variant::UINT64)
//        {
//            return 1;
//        }
//        std::printf("Record: user=%llu, bytes=%u\n", static_cast<unsigned long long>(static_cast<uint64_t>(*userId)),
//            static_cast<unsigned>(written));
//
//        // 顺序数组：同类型数字自动使用protobuf packed，保持插入顺序
//        MessageOwner pageList(ProtoMessageManager::create(PROTO_MESSAGE_ARRAY), &ProtoMessageManager::release);
//        if (!pageList || !pageList->array()->reserve(3) || !pageList->array()->append(Variant(int32_t(1))) ||
//            !pageList->array()->append(Variant(int32_t(4))) || !pageList->array()->append(Variant(int32_t(7))) ||
//            !pageList->serializeTo(output, sizeof(output), written))
//        {
//            return 1;
//        }
//        std::printf("Array: pages=%u, bytes=%u\n", static_cast<unsigned>(pageList->array()->size()),
//            static_cast<unsigned>(written));
//
//        // 固定两列的表：第一列资源ID，第二列名称；一行的值类型允许不同
//        MessageOwner resources(ProtoMessageManager::create(PROTO_MESSAGE_TABLE), &ProtoMessageManager::release);
//        if (!resources || !resources->table()->setColumns(2) || !resources->table()->reserveRows(2))
//        {
//            return 1;
//        }
//        Variant firstRow[] = {Variant(uint64_t(501)), Variant("document-a.pdf")};
//        Variant secondRow[] = {Variant(uint64_t(502)), Variant("document-b.pdf")};
//        if (!resources->table()->appendRow(firstRow, 2) || !resources->table()->appendRow(secondRow, 2) ||
//            !resources->serializeTo(output, sizeof(output), written))
//        {
//            return 1;
//        }
//        const Variant* title = resources->table()->at(0, 1);
//        if (title == nullptr || title->type() != Variant::STRING)
//        {
//            return 1;
//        }
//        std::printf("Table: first=%.*s, rows=%u, bytes=%u\n", static_cast<int>(title->size()), title->data(),
//            static_cast<unsigned>(resources->table()->rows()), static_cast<unsigned>(written));
//
//        // 通用消息：同一消息中组合字段组、列表与响应码，保留旧protobuf格式
//        MessageOwner response(ProtoMessageManager::create(PROTO_MESSAGE_GENERAL), &ProtoMessageManager::release);
//        if (!response)
//        {
//            return 1;
//        }
//        ProtoMessage* general = response->general();
//        general->setKeyValue(1, Variant(uint64_t(10001)));
//        general->addListValue(Variant(int32_t(501)));
//        general->addListValue(Variant(int32_t(502)));
//        general->setCode(0);
//        if (!response->serializeTo(output, sizeof(output), written))
//        {
//            return 1;
//        }
//        std::printf("General: code=%d, bytes=%u\n", general->getCode(), static_cast<unsigned>(written));
//        return 0;
//    }
//    catch (...)
//    {
//        // 通用类的历史void写入接口及值构造可能分配；智能指针仍负责释放
//        std::fprintf(stderr, "Message construction failed\n");
//        return 2;
//    }
//}