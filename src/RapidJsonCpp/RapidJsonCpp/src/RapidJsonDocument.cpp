#include "RapidJsonDocument.h"
#include "RapidJsonHelper.h"
#include <utility>

RapidJsonDocument::RapidJsonDocument() = default;
RapidJsonDocument::RapidJsonDocument(const RapidJsonDocument& document) : m_root(document.m_root) {}
RapidJsonDocument::RapidJsonDocument(RapidJsonDocument&& document) : m_root(std::move(document.m_root)) {}
RapidJsonDocument::~RapidJsonDocument() = default;

RapidJsonDocument& RapidJsonDocument::operator=(const RapidJsonDocument& document)
{
    m_root = document.m_root;
    return *this;
}

RapidJsonDocument& RapidJsonDocument::operator=(RapidJsonDocument&& document)
{
    m_root = std::move(document.m_root);
    return *this;
}

bool RapidJsonDocument::parse(const std::string& json)
{
    return RapidJsonHelper::parse(*m_root.m_document, json);
}

void RapidJsonDocument::setObject()
{
    m_root.setObject();
}

void RapidJsonDocument::setArray()
{
    m_root.setArray();
}

bool RapidJsonDocument::isObject() const
{
    return m_root.isObject();
}

bool RapidJsonDocument::isArray() const
{
    return m_root.isArray();
}

void RapidJsonDocument::addString(const char* key, const std::string& value)
{
    m_root.addString(key, value);
}

void RapidJsonDocument::addString(const char* key, const char* value)
{
    m_root.addString(key, value);
}

void RapidJsonDocument::addInt(const char* key, int32_t value)
{
    m_root.addInt(key, value);
}

void RapidJsonDocument::addBool(const char* key, bool value)
{
    m_root.addBool(key, value);
}

void RapidJsonDocument::addValue(const char* key, const RapidJsonValue& value)
{
    m_root.addValue(key, value);
}

void RapidJsonDocument::addStringArray(const char* key, const std::vector<std::string>& values)
{
    m_root.addStringArray(key, values);
}

std::string RapidJsonDocument::getStringOrDefault(const char* key, const std::string& defaultValue) const
{
    return m_root.getStringOrDefault(key, defaultValue);
}

int32_t RapidJsonDocument::getIntOrDefault(const char* key, int32_t defaultValue) const
{
    return m_root.getIntOrDefault(key, defaultValue);
}

bool RapidJsonDocument::getBoolOrDefault(const char* key, bool defaultValue) const
{
    return m_root.getBoolOrDefault(key, defaultValue);
}

RapidJsonValue RapidJsonDocument::getValue(const char* key) const
{
    return m_root.getValue(key);
}

std::vector<RapidJsonValue> RapidJsonDocument::getArrayValueOrEmpty(const char* key) const
{
    return m_root.getArrayValueOrEmpty(key);
}

std::vector<std::string> RapidJsonDocument::getStringArrayOrEmpty(const char* key) const
{
    return m_root.getStringArrayOrEmpty(key);
}

std::string RapidJsonDocument::toString() const
{
    return m_root.toString();
}