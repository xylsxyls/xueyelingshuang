#include "CookRecipeDelta.h"
#include <map>
#include <set>
#include <vector>

static const char *kRecipeDeltaFields[] = {"schemaVersion", "id", "creationId", "title", "category", "subtitle",
    "difficulty", "coverImageId", "mediaIds", "author", "authorUserId", "priceType", "priceCoins", "priceAmount",
    "defaultOwned", "systemRecipe", "customRecipe", "tags", "tools", "ingredients"};
static const char *kTaskDeltaFields[] = {"id", "title", "detail", "resource", "voiceText", "safetyLevel",
    "backgroundWaitMode", "durationSeconds", "active", "manualSkippable", "canPause", "continuesDuringPause",
    "canLeaveKitchen", "dependencies"};

bool CookRecipeDelta::knownField(const std::string &field, bool task)
{
    const size_t count = task ? sizeof(kTaskDeltaFields) / sizeof(kTaskDeltaFields[0])
                              : sizeof(kRecipeDeltaFields) / sizeof(kRecipeDeltaFields[0]);
    for (size_t i = 0; i < count; ++i)
    {
        if (field == (task ? kTaskDeltaFields[i] : kRecipeDeltaFields[i]))
        {
            return true;
        }
    }
    return false;
}

bool CookRecipeDelta::compare(const std::string &before, const std::string &after, RapidJsonDocument &delta)
{
    RapidJsonDocument oldDocument, newDocument;
    if (!oldDocument.parse(before) || !newDocument.parse(after) || !oldDocument.isObject() || !newDocument.isObject())
    {
        return false;
    }
    RapidJsonValue fields;
    fields.setArray();
    for (size_t i = 0; i < sizeof(kRecipeDeltaFields) / sizeof(kRecipeDeltaFields[0]); ++i)
    {
        const char *field = kRecipeDeltaFields[i];
        if (oldDocument.getValue(field).toString() != newDocument.getValue(field).toString())
        {
            RapidJsonValue item;
            item.setObject();
            item.addString("field", field);
            item.addValue("value", newDocument.getValue(field));
            fields.pushValue(item);
        }
    }
    const std::vector<RapidJsonValue> oldTasks = oldDocument.getArrayValueOrEmpty("tasks"),
                                      newTasks = newDocument.getArrayValueOrEmpty("tasks");
    std::map<std::string, RapidJsonValue> oldById;
    std::vector<std::string> oldOrder, newOrder;
    for (size_t i = 0; i < oldTasks.size(); ++i)
    {
        const std::string id = oldTasks[i].getStringOrDefault("id", "");
        if (id.empty() || oldById.count(id) != 0)
        {
            return false;
        }
        oldById[id] = oldTasks[i];
        oldOrder.push_back(id);
    }
    RapidJsonValue tasks;
    tasks.setArray();
    std::set<std::string> ids;
    for (size_t i = 0; i < newTasks.size(); ++i)
    {
        const std::string id = newTasks[i].getStringOrDefault("id", "");
        if (id.empty() || !ids.insert(id).second)
        {
            return false;
        }
        newOrder.push_back(id);
        RapidJsonValue changes;
        changes.setArray();
        for (size_t j = 0; j < sizeof(kTaskDeltaFields) / sizeof(kTaskDeltaFields[0]); ++j)
        {
            const char *field = kTaskDeltaFields[j];
            if (oldById.count(id) == 0 ||
                oldById[id].getValue(field).toString() != newTasks[i].getValue(field).toString())
            {
                RapidJsonValue item;
                item.setObject();
                item.addString("field", field);
                item.addValue("value", newTasks[i].getValue(field));
                changes.pushValue(item);
            }
        }
        if (changes.size() != 0)
        {
            RapidJsonValue item;
            item.setObject();
            item.addString("id", id);
            item.addValue("fields", changes);
            tasks.pushValue(item);
        }
    }
    delta.setObject();
    delta.addValue("fields", fields);
    delta.addValue("tasks", tasks);
    delta.addBool("orderChanged", oldOrder != newOrder);
    if (oldOrder != newOrder)
    {
        delta.addStringArray("order", newOrder);
    }
    return true;
}

bool CookRecipeDelta::apply(const std::string &before, const std::string &deltaJson, std::string &after)
{
    RapidJsonDocument result, delta;
    if (!result.parse(before) || !delta.parse(deltaJson) || !result.isObject() || !delta.isObject())
    {
        return false;
    }
    const std::vector<RapidJsonValue> changes = delta.getArrayValueOrEmpty("fields");
    for (size_t i = 0; i < changes.size(); ++i)
    {
        const std::string field = changes[i].getStringOrDefault("field", "");
        if (!knownField(field, false))
        {
            return false;
        }
        result.addValue(field.c_str(), changes[i].getValue("value"));
    }
    const std::vector<RapidJsonValue> oldTasks = result.getArrayValueOrEmpty("tasks"),
                                      patches = delta.getArrayValueOrEmpty("tasks");
    std::map<std::string, RapidJsonValue> tasks;
    std::vector<std::string> order;
    for (size_t i = 0; i < oldTasks.size(); ++i)
    {
        const std::string id = oldTasks[i].getStringOrDefault("id", "");
        if (id.empty() || tasks.count(id) != 0)
        {
            return false;
        }
        tasks[id] = oldTasks[i];
        order.push_back(id);
    }
    for (size_t i = 0; i < patches.size(); ++i)
    {
        const std::string id = patches[i].getStringOrDefault("id", "");
        if (id.empty())
        {
            return false;
        }
        if (tasks.count(id) == 0)
        {
            tasks[id].setObject();
        }
        const std::vector<RapidJsonValue> fields = patches[i].getArrayValueOrEmpty("fields");
        for (size_t j = 0; j < fields.size(); ++j)
        {
            const std::string field = fields[j].getStringOrDefault("field", "");
            if (!knownField(field, true))
            {
                return false;
            }
            tasks[id].addValue(field.c_str(), fields[j].getValue("value"));
        }
    }
    if (delta.getBoolOrDefault("orderChanged", false))
    {
        order = delta.getStringArrayOrEmpty("order");
    }
    RapidJsonValue restored;
    restored.setArray();
    std::set<std::string> used;
    if (order.empty() || order.size() > 256)
    {
        return false;
    }
    for (size_t i = 0; i < order.size(); ++i)
    {
        if (tasks.count(order[i]) == 0 || !used.insert(order[i]).second ||
            tasks[order[i]].getStringOrDefault("id", "") != order[i])
        {
            return false;
        }
        restored.pushValue(tasks[order[i]]);
    }
    result.addValue("tasks", restored);
    after = result.toString();
    return true;
}

bool CookRecipeDelta::empty(const RapidJsonDocument &delta)
{
    return delta.getArrayValueOrEmpty("fields").empty() && delta.getArrayValueOrEmpty("tasks").empty() &&
           !delta.getBoolOrDefault("orderChanged", false);
}