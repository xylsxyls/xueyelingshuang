#include "LockFreeQueueTestResults.h"
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <stdexcept>

Json::Value LockFreeQueueTestResults::fields(const std::string& line)
{
    Json::Value result;
    std::istringstream stream(line);
    std::string token;
    while (stream >> token)
    {
        size_t pos = token.find('=');
        if (pos == std::string::npos)
        {
            continue;
        }
        std::string key = token.substr(0, pos), value = token.substr(pos + 1);
        if (result.isMember(key))
        {
            throw std::runtime_error("Duplicate evidence field: " + key);
        }
        std::string special = value;
        for (size_t i = 0; i < special.size(); ++i)
        {
            if (special[i] >= 'A' && special[i] <= 'Z')
            {
                special[i] += 'a' - 'A';
            }
        }
        if (special == "nan" || special == "inf" || special == "+inf" || special == "-inf" || special == "infinity")
        {
            throw std::runtime_error("Nonfinite evidence number: " + line);
        }
        char* end = nullptr;
        double number = std::strtod(value.c_str(), &end);
        if (!value.empty() && end && !*end)
        {
            if (!std::isfinite(number))
            {
                throw std::runtime_error("Nonfinite evidence number: " + line);
            }
            result[key] = number;
        }
        else
        {
            result[key] = value;
        }
    }
    return result;
}

Json::Value LockFreeQueueTestResults::parse(const std::string& text, const Json::Value& meta, int32_t code)
{
    typedef LockFreeQueueTestTool IO;
    std::string kind = meta["kind"].asString();
    Json::Value output;
    const char* tables[] = {"samples", "latencies", "resources", "pools", "maintenance", "lanes"};
    for (size_t i = 0; i < 6; ++i)
    {
        output[tables[i]] = Json::Value(Json::arrayValue);
    }
    std::vector<std::string> lines = IO::split(IO::plain(text), '\n');
    std::string currentCase;
    Json::Value cycle;
    int32_t high = 0, verification = 0, waveStages = 0, shrink = 0;
    bool pass = false;
    std::set<int32_t> passed;
    bool reference = meta["reference"].asBool();
    std::string suite = meta["suite"].asString(), profile = meta["profile"].asString();
    bool sufficient = profile == "fixed" || profile == "dynamic" || profile == "grown_reuse";
    for (size_t i = 0; i < lines.size(); ++i)
    {
        const std::string& line = lines[i];
        std::string tag = line.substr(0, line.find(' '));
        Json::Value values = fields(line), row = meta;
        std::vector<std::string> keys = values.getMemberNames();
        for (size_t k = 0; k < keys.size(); ++k)
        {
            row[keys[k]] = values[keys[k]];
        }
        if (tag == "PASS")
        {
            pass = true;
            if (values.isMember("id"))
            {
                int32_t id = values["id"].asInt();
                if (!passed.insert(id).second)
                {
                    throw std::runtime_error("Duplicate PASS id");
                }
            }
        }
        if (kind != "scheduling" && (tag == "FAIL" || (values.isMember("valid") && values["valid"].asDouble() != 1)))
        {
            throw std::runtime_error("Workload oracle failed: " + line);
        }
        if (kind == "shared-ptr" && tag == "RESULT")
        {
            if (values["valid"].asDouble() != 1 || values["items"] != meta["items"] || values["producers"] != meta["producers"] ||
                values["pool_allocations"].asDouble() != 0 || values["pool_grows"].asDouble() != 0 ||
                !values.isMember("pool_allocations") || !values.isMember("pool_grows"))
            {
                throw std::runtime_error("Shared-ptr oracle/zero-allocation window failed");
            }
            output["samples"].append(row);
        }
        if (kind == "scheduling")
        {
            if (tag == "LANE")
            {
                output["lanes"].append(row);
            }
            if (tag == "SCHEDULE")
            {
                output["samples"].append(row);
            }
        }
        if (kind != "performance")
        {
            continue;
        }
        if (tag == "WAVE")
        {
            currentCase = values["case"].asString();
        }
        else if (tag == "SCENARIO")
        {
            currentCase = values["phase"].asString() + IO::integer(values["producers"].asInt());
        }
        else if (tag == "OBSERVER")
        {
            high = values["record_high_water"].asInt();
        }
        else if (tag == "POLICY")
        {
            currentCase = values["case"].asString() + "." + values["phase"].asString();
            cycle = values["cycle"];
        }
        else if (tag == "BACKLOG")
        {
            currentCase = "backlog" + IO::integer(values["count"].asInt());
        }
        if (tag == "BURST")
        {
            currentCase = "burst" + IO::integer(values["width"].asInt());
        }
        if (tag == "EXTRA")
        {
            currentCase = values["operation"].asString();
            if (currentCase == "size_empty" || currentCase == "empty_empty" || currentCase == "pop_empty" || currentCase == "size_one" || currentCase == "push_pop_empty_cycle")
            {
                currentCase += "_r" + IO::integer(high);
            }
        }
        row["case"] = currentCase;
        row["cycle"] = cycle;
        if (tag == "WAVE_STAGE")
        {
            ++waveStages;
            if (values["backlog"].asInt() != (values["phase"].asInt() == 0 ? 1000 : 500) ||
                (!reference && (values["capacity"].asInt() < 750 || values["capacity"].asInt() > 1500)))
            {
                throw std::runtime_error("Wrong wave backlog/capacity");
            }
            output["pools"].append(row);
        }
        if (tag == "REGISTERED_PRODUCERS" || tag == "REGISTERED_MPSC" || tag == "BURST" || tag == "EXTRA" || tag == "POLICY_RESULT" || tag == "WAVE_RESULT")
        {
            if (values["valid"].asDouble() != 1)
            {
                throw std::runtime_error("Missing explicit workload validation");
            }
            if (suite == "35" && sufficient)
            {
                if (values.get("rejected_retries", 0).asDouble() != 0 && meta["version"].asString().find("tryPush") == std::string::npos)
                {
                    throw std::runtime_error("Unexpected rejection in sufficient/reused capacity");
                }
                if (!reference && meta["counted"].asBool() && (!values.isMember("allocations") || values["allocations"].asDouble() != 0))
                {
                    throw std::runtime_error("Timed allocation in sufficient pool");
                }
            }
            if (suite == "40" && values["operations"].asDouble() != 1182148)
            {
                throw std::runtime_error("Wrong wave operation count");
            }
            output["samples"].append(row);
        }
        if (tag == "POOL_VERIFY")
        {
            ++verification;
            if (sufficient && (!values.isMember("allocation_delta") || !values.isMember("grow_delta") || values["allocation_delta"].asDouble() != 0 || values["grow_delta"].asDouble() != 0))
            {
                throw std::runtime_error("Sufficient pool allocated or grew");
            }
        }
        if (tag == "LATENCY")
        {
            std::string operation = values["operation"].asString();
            if (operation == "growth_allocate_publish" || operation == "full_pool_reject" || operation == "shrink_detach_free_256_visits" || operation == "direct_destroy_256kb_payload" || operation == "pop_destroy_256kb_payload")
            {
                row["case"] = operation;
            }
            output["latencies"].append(row);
        }
        if (tag == "RESOURCE")
        {
            if (line.find("NOT_RUN=") == std::string::npos && (values["memory_ok"].asDouble() != 1 || values["handles_ok"].asDouble() != 1))
            {
                throw std::runtime_error("Resource probe failed");
            }
            output["resources"].append(row);
        }
        if (tag == "POOL" || tag == "POLICY_POOL" || tag == "SHRINK")
        {
            if (tag == "SHRINK")
            {
                row["case"] = "supplement_shrink";
            }
            output["pools"].append(row);
        }
        if (tag == "MAINTENANCE" && values.isMember("phase"))
        {
            if (values["phase"].asString() == "shrink")
            {
                ++shrink;
            }
            output["maintenance"].append(row);
        }
    }
    if (kind == "performance")
    {
        size_t expected = suite == "35" || suite == "40" ? 10 : (suite == "38" ? 42 : (reference ? 27 : 29));
        if (output["samples"].size() != expected || !pass || code != 0)
        {
            throw std::runtime_error("Missing benchmark rows/PASS; got " + IO::integer(output["samples"].size()));
        }
        if (suite == "35" && !reference && verification != 10)
        {
            throw std::runtime_error("Missing pool verification windows");
        }
        if (suite == "38" && !reference && shrink != 16)
        {
            throw std::runtime_error("Missing shrink/regrow cycles");
        }
        if (suite == "40" && waveStages != 120)
        {
            throw std::runtime_error("Missing wave stages");
        }
    }
    else if (kind == "shared-ptr")
    {
        if (code || output["samples"].size() != 2)
        {
            throw std::runtime_error("Missing shared-ptr measurement phase");
        }
        std::set<std::string> phases;
        for (Json::Value::UInt i = 0; i < 2; ++i)
        {
            phases.insert(output["samples"][i]["phase"].asString());
        }
        if (phases != std::set<std::string>({"separate", "concurrent"}))
        {
            throw std::runtime_error("Invalid shared-ptr phases");
        }
    }
    else if (kind == "scheduling")
    {
        if (output["samples"].size() != 1 || output["lanes"].size() != meta["producers"].asUInt() + 1)
        {
            throw std::runtime_error("Missing scheduling lanes/summary");
        }
        Json::Value summary = output["samples"][0u];
        bool negative = meta["version"].asString() == "before" && code == 3 && summary["progress_valid"].asDouble() == 0;
        if (summary["data_valid"].asDouble() != 1 || summary["same_priority"].asDouble() != 1 ||
            (!negative && (code != 0 || summary["progress_valid"].asDouble() != 1)))
        {
            throw std::runtime_error("Scheduling oracle failed");
        }
        std::set<int32_t> indices;
        for (Json::Value::UInt i = 0; i < output["lanes"].size(); ++i)
        {
            const Json::Value& lane = output["lanes"][i];
            int32_t index = lane["index"].asInt();
            if (!indices.insert(index).second || index < 0 || index > meta["producers"].asInt())
            {
                throw std::runtime_error("Duplicate/invalid scheduling lane");
            }
            if (lane["consumer"].asInt() != (index == meta["producers"].asInt() ? 1 : 0))
            {
                throw std::runtime_error("Wrong consumer lane");
            }
        }
    }
    else if (kind == "core")
    {
        std::set<int32_t> expected;
        for (Json::Value::UInt i = 0; i < meta["ids"].size(); ++i)
        {
            expected.insert(meta["ids"][i].asInt());
        }
        if (code || passed != expected)
        {
            throw std::runtime_error("Core selection and PASS IDs disagree");
        }
    }
    else if (code != meta.get("expected_exit", 0).asInt())
    {
        throw std::runtime_error("Unexpected child exit");
    }
    return output;
}

double LockFreeQueueTestResults::median(std::vector<double> values)
{
    if (values.empty())
    {
        throw std::runtime_error("Empty median");
    }
    for (size_t i = 0; i < values.size(); ++i)
    {
        if (!std::isfinite(values[i]))
        {
            throw std::runtime_error("Nonfinite median");
        }
    }
    std::sort(values.begin(), values.end());
    size_t middle = values.size() / 2;
    return values.size() % 2 ? values[middle] : values[middle - 1] / 2 + values[middle] / 2;
}

Json::Value LockFreeQueueTestResults::rates(const Json::Value& input, double extra)
{
    Json::Value row(input);
    if (!row.isMember("elapsed_ns"))
    {
        return row;
    }
    double count = row.get("items", row["operations"]).asDouble(), elapsed = row["elapsed_ns"].asDouble();
    if (count <= 0 || elapsed <= 0 || extra < 0)
    {
        throw std::runtime_error("Invalid timed operation count/duration");
    }
    row["rate_per_second"] = count * 1e9 / elapsed;
    row["complete_rate_per_second"] = count * 1e9 / (elapsed + extra);
    row["extra_maintenance_ns"] = extra;
    row["cpu_ns_per_operation"] = row.get("cpu_ns", 0).asDouble() / count;
    return row;
}

void LockFreeQueueTestResults::csv(const std::string& path, const Json::Value& rows)
{
    std::set<std::string> names;
    for (Json::Value::UInt i = 0; i < rows.size(); ++i)
    {
        std::vector<std::string> keys = rows[i].getMemberNames();
        names.insert(keys.begin(), keys.end());
    }
    std::ostringstream output;
    output << "\xef\xbb\xbf" << std::setprecision(17);
    for (std::set<std::string>::const_iterator key = names.begin(); key != names.end(); ++key)
    {
        if (key != names.begin())
        {
            output << ',';
        }
        output << *key;
    }
    output << "\r\n";
    for (Json::Value::UInt i = 0; i < rows.size(); ++i)
    {
        for (std::set<std::string>::const_iterator key = names.begin(); key != names.end(); ++key)
        {
            if (key != names.begin())
            {
                output << ',';
            }
            const Json::Value& value = rows[i][*key];
            if (value.isNull())
            {
                output << "N/A";
            }
            else if (value.isNumeric())
            {
                output << value.asDouble();
            }
            else if (value.isBool())
            {
                output << (value.asBool() ? 1 : 0);
            }
            else
            {
                std::string text = value.isString() ? value.asString() : Json::FastWriter().write(value);
                output << '"';
                for (size_t j = 0; j < text.size(); ++j)
                {
                    if (text[j] == '"')
                    {
                        output << '"';
                    }
                    output << text[j];
                }
                output << '"';
            }
        }
        output << "\r\n";
    }
    LockFreeQueueTestTool::write(path, output.str());
}

void LockFreeQueueTestResults::summarize(const std::string& batch)
{
    typedef LockFreeQueueTestTool IO;
    Json::Value plan = IO::load(IO::join(batch, "plan.json"));
    Json::Value commands = IO::load(IO::join(batch, "commands.json")), data;
    if (commands.size() != plan.size())
    {
        throw std::runtime_error("Incomplete matrix; cannot summarize as complete");
    }
    Json::Value identities = IO::load(IO::join(batch, "frozen-hashes.json"));
    if (identities.isMember("host/LockFreeQueueTestTool.exe") &&
        IO::digest(IO::read(IO::executable())) != identities["host/LockFreeQueueTestTool.exe"].asString())
    {
        throw std::runtime_error("Parser identity differs; run the frozen host/LockFreeQueueTestTool.exe summarize");
    }
    std::vector<std::string> files = identities.getMemberNames();
    for (size_t i = 0; i < files.size(); ++i)
    {
        if (IO::digest(IO::read(IO::join(batch, files[i]))) != identities[files[i]].asString())
        {
            throw std::runtime_error("Frozen source/binary changed: " + files[i]);
        }
    }
    std::set<std::string> labels;
    for (Json::Value::UInt i = 0; i < commands.size(); ++i)
    {
        const Json::Value& record = commands[i];
        if (record["validated_exit"].asInt() != 0 || !record.isMember("validated_exit"))
        {
            throw std::runtime_error("Failed evidence cannot be summarized");
        }
        std::string label = record["label"].asString();
        if (!labels.insert(label).second)
        {
            throw std::runtime_error("Duplicate completed command");
        }
        const Json::Value* expected = nullptr;
        for (Json::Value::UInt j = 0; j < plan.size(); ++j)
        {
            if (plan[j]["label"].asString() == label)
            {
                expected = &plan[j];
            }
        }
        if (!expected || record["meta"] != (*expected)["meta"] || record["command"] != (*expected)["command"])
        {
            throw std::runtime_error("Recorded command differs from matrix");
        }
        std::string log = IO::join(batch, record["relative_log"].asString());
        IO::inside(log, batch);
        std::string raw = IO::read(log);
        if (IO::digest(raw) != record["sha256"].asString())
        {
            throw std::runtime_error("Raw log identity changed");
        }
        Json::Value parsed = parse(raw, record["meta"], record["exit"].asInt());
        // Preserve the exact canonical text: this repository's legacy JSON writer uses 16
        // significant digits, so loading/re-serializing a double may change its last bit.
        if (!record["parsed"].isString() || Json::FastWriter().write(parsed) != record["parsed"].asString())
        {
            throw std::runtime_error("Raw log and stored parsed evidence disagree");
        }
        std::vector<std::string> tables = parsed.getMemberNames();
        for (size_t j = 0; j < tables.size(); ++j)
        {
            for (Json::Value::UInt k = 0; k < parsed[tables[j]].size(); ++k)
            {
                data[tables[j]].append(parsed[tables[j]][k]);
            }
        }
    }
    std::string reports = IO::join(batch, "reports");
    std::vector<std::string> tables = data.getMemberNames();
    for (size_t i = 0; i < tables.size(); ++i)
    {
        csv(IO::join(reports, tables[i] + "-raw.csv"), data[tables[i]]);
    }
    // Key by semantic scenario, including cycle/phase, never by version or round.
    std::map<std::string, std::map<std::string, std::vector<Json::Value>>> groups;
    std::map<std::string, Json::Value> keys;
    const char* keyFields[] = {"arch", "profile", "suite", "case", "cycle", "phase", "mode", "producers", "mask", "cancelable", "capacity", "call", "operation", "index", "consumer"};
    for (size_t t = 0; t < tables.size(); ++t)
    {
        if (tables[t] == "maintenance" || tables[t] == "pools")
        {
            continue;
        }
        for (Json::Value::UInt i = 0; i < data[tables[t]].size(); ++i)
        {
            Json::Value row = data[tables[t]][i], key;
            if (row["phase_group"].asString() == "preflight")
            {
                continue;
            }
            key["table"] = tables[t];
            key["counted"] = row["counted"];
            for (size_t k = 0; k < sizeof(keyFields) / sizeof(keyFields[0]); ++k)
            {
                key[keyFields[k]] = row[keyFields[k]];
            }
            if (tables[t] == "samples")
            {
                double extra = 0;
                for (Json::Value::UInt j = 0; j < data["maintenance"].size(); ++j)
                {
                    const Json::Value& m = data["maintenance"][j];
                    if (m["arch"] == row["arch"] && m["version"] == row["version"] && m["profile"] == row["profile"] && m["suite"] == row["suite"] &&
                        m["case"] == row["case"] && m["cycle"] == row["cycle"] && m["round"] == row["round"] && m["counted"] == row["counted"] && m["phase_group"] == row["phase_group"])
                    {
                        extra += m.get("elapsed_ns", 0).asDouble();
                    }
                }
                row = rates(row, extra);
            }
            std::string encoded = Json::FastWriter().write(key), version = row["version"].asString();
            // Revision call variants share the same comparison group.
            size_t hyphen = version.find('-');
            if (hyphen != std::string::npos && (version.compare(0, 7, "before-") == 0 || version.compare(0, 6, "after-") == 0))
            {
                key["call"] = version.substr(hyphen + 1);
                version = version.substr(0, hyphen);
                encoded = Json::FastWriter().write(key);
            }
            keys[encoded] = key;
            groups[encoded][version].push_back(row);
        }
    }
    Json::Value summary(Json::arrayValue);
    for (std::map<std::string, std::map<std::string, std::vector<Json::Value>>>::const_iterator group = groups.begin(); group != groups.end(); ++group)
    {
        Json::Value out = keys[group->first];
        std::set<std::string> metrics;
        std::map<std::string, std::map<std::string, double>> medians;
        for (std::map<std::string, std::vector<Json::Value>>::const_iterator version = group->second.begin(); version != group->second.end(); ++version)
        {
            std::vector<std::string> names = version->second[0].getMemberNames();
            out[version->first + "_samples"] = static_cast<int32_t>(version->second.size());
            for (size_t i = 0; i < names.size(); ++i)
            {
                const std::string& metric = names[i];
                if (!version->second[0][metric].isNumeric() || out.isMember(metric) || metric == "round")
                {
                    continue;
                }
                std::vector<double> values;
                for (size_t r = 0; r < version->second.size(); ++r)
                {
                    if (!version->second[r][metric].isNumeric())
                    {
                        throw std::runtime_error("Metric missing in one round");
                    }
                    values.push_back(version->second[r][metric].asDouble());
                }
                double value = median(values);
                medians[version->first][metric] = value;
                metrics.insert(metric);
                out[version->first + "_" + metric] = value;
                out[version->first + "_" + metric + "_min"] = *std::min_element(values.begin(), values.end());
                out[version->first + "_" + metric + "_max"] = *std::max_element(values.begin(), values.end());
            }
        }
        std::string candidate = medians.count("after") ? "after" : (medians.count("candidate") ? "candidate" : (medians.count("current") ? "current" : "push"));
        if (medians.count(candidate))
        {
            for (std::map<std::string, std::map<std::string, double>>::const_iterator reference = medians.begin(); reference != medians.end(); ++reference)
            {
                if (reference->first == candidate)
                {
                    continue;
                }
                for (std::set<std::string>::const_iterator metric = metrics.begin(); metric != metrics.end(); ++metric)
                {
                    std::string label = *metric + "_" + candidate + "_vs_" + reference->first + "_percent";
                    if (medians[candidate].count(*metric) && reference->second.count(*metric) && reference->second.find(*metric)->second != 0)
                    {
                        out[label] = (medians[candidate][*metric] / reference->second.find(*metric)->second - 1) * 100;
                    }
                    else
                    {
                        out[label] = Json::Value();
                    }
                }
            }
        }
        summary.append(out);
    }
    IO::save(IO::join(batch, "comparison.json"), summary);
    csv(IO::join(reports, "comparison.csv"), summary);
    Json::Value wave(Json::arrayValue);
    for (Json::Value::UInt i = 0; i < summary.size(); ++i)
    {
        if (summary[i]["suite"].asString() == "40")
        {
            wave.append(summary[i]);
        }
    }
    csv(IO::join(reports, "wave750-1000-500.csv"), wave);
    std::cout << "EXPORTED " << summary.size() << " comparison rows; positive percent means larger metric (cost increases are worse)." << std::endl;
}