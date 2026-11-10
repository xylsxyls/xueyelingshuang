#include "ProtoMessageDocument.h"

ProtoMessageDocument::ProtoMessageDocument()
{

}

ProtoMessageDocument::~ProtoMessageDocument()
{

}

ProtoMessage* ProtoMessageDocument::general()
{
    return nullptr;
}

ProtoMessageRecord* ProtoMessageDocument::record()
{
    return nullptr;
}

ProtoMessageArray* ProtoMessageDocument::array()
{
    return nullptr;
}

ProtoMessageTable* ProtoMessageDocument::table()
{
    return nullptr;
}