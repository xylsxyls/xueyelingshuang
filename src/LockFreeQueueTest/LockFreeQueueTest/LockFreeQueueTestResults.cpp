#include "LockFreeQueueTestResults.h"
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <stdexcept>

double LockFreeQueueTestResults::whole(const Json::Value& values, const char* key, double maximum)
{
    const double value = number(values, key, false);
    if (value != std::floor(value) || value > maximum)
    {
        throw std::runtime_error(std::string("Invalid evidence integer: ") + key);
    }
    return value;
}

double LockFreeQueueTestResults::number(const Json::Value& values, const char* key, bool positive)
{
    if (!values[key].isNumeric() || !std::isfinite(values[key].asDouble()) ||
        (positive ? values[key].asDouble() <= 0 : values[key].asDouble() < 0))
    {
        throw std::runtime_error(std::string("Missing/invalid evidence number: ") + key);
    }
    return values[key].asDouble();
}

std::map<std::string, double> LockFreeQueueTestResults::scenarios(const std::string& suite, bool reference)
{
    typedef LockFreeQueueTestTool IO;
    std::map<std::string, double> expected;
    if (suite == "35" || suite == "40")
    {
        for (int32_t p = 1; p <= 16; p *= 2)
        {
            if (suite == "35")
            {
                expected["producer" + IO::integer(p)] = 4000000;
                expected["mpsc" + IO::integer(p)] = 1000000;
            }
            else
            {
                expected["wave750_t0_p" + IO::integer(p)] = 1182148;
                expected["wave750_t8_p" + IO::integer(p)] = 1182148;
            }
        }
    }
    else if (suite == "38")
    {
        const char* names[] = {"default_zero", "default_positive", "finite_fixed_zero",
            "finite_fixed_positive", "finite_proportional", "shrink_regrow_zero", "shrink_regrow_positive"};
        for (size_t i = 0; i < 7; ++i)
        {
            for (int32_t cycle = 0; cycle < (i >= 5 ? 8 : 4); ++cycle)
            {
                expected[std::string(names[i]) + (i >= 5 || cycle == 0 ? ".growth#" : ".reuse#") +
                    IO::integer(cycle)] = i < 2 ? 32768 : 4095;
            }
        }
        for (int32_t p = 1; p <= 16; p *= 4)
        {
            expected["bounded0_p" + IO::integer(p) + ".pressure#0"] = 200000;
            expected["bounded1_p" + IO::integer(p) + ".pressure#0"] = 200000;
        }
    }
    else if (suite == "extra")
    {
        expected["burst1"] = 2000000;
        expected["burst4096"] = 1048576;
        expected["burst65536"] = 1048576;
        const char* names[] = {"size_empty", "empty_empty", "pop_empty", "size_one", "push_pop_empty_cycle"};
        const int32_t records[] = {1, 16, 128};
        for (size_t i = 0; i < 5; ++i)
        {
            for (size_t r = 0; r < 3; ++r)
            {
                expected[std::string(names[i]) + "_r" + IO::integer(records[r])] = i == 4 ? 10000 : 20000000;
            }
        }
        expected["default_construct_destroy"] = 2000;
        expected["configured_destroy_init"] = 2000;
        expected["clear_65536"] = 65536;
        expected["destroy_65536"] = 65536;
        expected["payload_inline256"] = 262144;
        expected["payload_string64"] = 262144;
        expected["payload_string1024"] = 131072;
        expected["payload_string16384"] = 8192;
        expected["payload_shared256kb"] = 262144;
        if (!reference)
        {
            expected["full_pool_failure"] = 10000;
            expected["context_acquire_release"] = 5000000;
        }
    }
    else
    {
        throw std::runtime_error("Unknown performance suite");
    }
    return expected;
}

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
    bool pass = false, sawNotification = false;
    size_t completions = 0;
    std::map<std::string, std::set<size_t>> sharedLanes;
    std::set<int32_t> passed;
    bool reference = meta["reference"].asBool();
    std::string suite = meta["suite"].asString(), profile = meta["profile"].asString();
    bool sufficient = profile == "fixed" || profile == "dynamic" || profile == "grown_reuse";
    const bool backlog = kind == "core" && meta["ids"].size() == 1 && meta["ids"][0u].asInt() == 22;
    std::set<int32_t> remainingBacklogs;
    if (backlog)
    {
        remainingBacklogs = {1, 1024, 65536, 262144};
    }
    int32_t pendingCount = 0;
    double firstPop = 0;
    std::set<std::string> measured, verified, stages, shrunk, reclaimed;
    const std::map<std::string, double> expected = kind == "performance" ? scenarios(suite, reference) :
        std::map<std::string, double>();
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
        if (backlog && tag == "BACKLOG")
        {
            const int32_t count = static_cast<int32_t>(whole(values, "count", 262144));
            if (pendingCount || !remainingBacklogs.erase(count) || whole(values, "valid", 1) != 1)
            {
                throw std::runtime_error("Wrong, duplicate or unfinished backlog");
            }
            firstPop = whole(values, "first_pop_ns", 9007199254740991.0);
            pendingCount = count;
            row["case"] = "backlog" + IO::integer(pendingCount);
            output["samples"].append(row);
        }
        if (backlog && tag == "LATENCY")
        {
            if (!pendingCount || values["operation"].asString() != "pop_backlog" ||
                whole(values, "samples", 262144) != pendingCount)
            {
                throw std::runtime_error("Wrong backlog latency operation or sample count");
            }
            const char* quantiles[] = {"p50_ns", "p95_ns", "p99_ns", "max_ns"};
            double previous = 0;
            for (size_t q = 0; q < 4; ++q)
            {
                const double current = whole(values, quantiles[q], 9007199254740991.0);
                if (current < previous || (pendingCount == 1 && current != firstPop) || (q == 3 && current < firstPop))
                {
                    throw std::runtime_error("Inconsistent backlog latency distribution");
                }
                previous = current;
            }
            row["case"] = "backlog" + IO::integer(pendingCount);
            output["latencies"].append(row);
            pendingCount = 0;
        }
        if (kind == "core" && tag == "NOTIFICATION")
        {
            if (sawNotification || whole(values, "received", 6000) != 6000 || whole(values, "wake_permits", 191) != 191 ||
                whole(values, "coalesced_batch", 32) != 32 || whole(values, "finite_wait_ms", 5) != 5 ||
                whole(values, "valid", 1) != 1)
            {
                throw std::runtime_error("Wrong or duplicate notification evidence");
            }
            sawNotification = true;
        }
        if (kind == "core" && tag == "SUMMARY")
        {
            if (++completions != 1 || whole(values, "executed", 50) != meta["ids"].size() || whole(values, "failed", 0) != 0)
            {
                throw std::runtime_error("Wrong or duplicate core summary");
            }
        }
        if (kind == "demo" && line == "LockFreeQueue usage demo passed")
        {
            ++completions;
        }
        if (kind == "module" && tag == "PASS")
        {
            const int32_t width = meta["pointer_bytes"].asInt();
            if ((width != 4 && width != 8) || line.find("PASS module_consumer ") != 0 ||
                whole(values, "engines", 2) != 2 || whole(values, "cross_module_inline", 1) != 1 ||
                whole(values, "producer_context", 1) != 1 || whole(values, "pointer_bytes", 8) != width)
            {
                throw std::runtime_error("Wrong module completion evidence");
            }
            ++completions;
        }
        if (tag == "PASS")
        {
            int32_t id = 0;
            if (values.isMember("id"))
            {
                // Validate before narrowing so fractional and out-of-range IDs
                // follow the same evidence contract before JsonCpp conversion.
                id = static_cast<int32_t>(whole(values, "id", 50));
                if (id == 0)
                {
                    throw std::runtime_error("Zero PASS id");
                }
                if (!passed.insert(id).second)
                {
                    throw std::runtime_error("Duplicate PASS id");
                }
            }
            if (kind == "core" && id == 0)
            {
                throw std::runtime_error("Core PASS requires an ID");
            }
            if (kind == "performance" && (pass || (suite == "extra" ? values["suite"].asString() != "extra" :
                id != std::atoi(suite.c_str()))))
            {
                throw std::runtime_error("Wrong/duplicate benchmark PASS");
            }
            pass = true;
        }
        if (kind != "scheduling" && (tag == "FAIL" || (values.isMember("valid") && values["valid"].asDouble() != 1)))
        {
            throw std::runtime_error("Workload oracle failed: " + line);
        }
        if (kind == "shared-ptr" && tag == "SETUP")
        {
            const std::string mode = meta["mode"].asString();
            const bool prefix = mode == "prefix" || mode == "distributed" || mode == "reserved";
            const bool distributed = mode == "distributed" || mode == "distributed_pop" || mode == "reserved";
            const bool known = mode == "plain" || mode == "bound" || prefix || distributed;
            if (++completions != 1 || !known || whole(values, "case", 46) != 46 ||
                whole(values, "items", 20000000) != number(meta, "items", true) ||
                whole(values, "producers", 16) != number(meta, "producers", true) ||
                whole(values, "capacity", 9007199254740991.0) <= number(meta, "items", true) ||
                (whole(values, "bound", 1) != 0) != (mode != "plain") || (whole(values, "prefix", 1) != 0) != prefix ||
                (whole(values, "distributed", 1) != 0) != distributed || (whole(values, "reserved", 1) != 0) != (mode == "reserved"))
            {
                throw std::runtime_error("Shared-ptr setup differs from requested workload");
            }
        }
        if (kind == "shared-ptr" && tag == "LANE")
        {
            const std::string phase = values["phase"].asString();
            const size_t producers = static_cast<size_t>(whole(meta, "producers", 16));
            const size_t index = static_cast<size_t>(whole(values, "index", static_cast<double>(producers)));
            if ((phase != "separate" && phase != "concurrent") || !sharedLanes[phase].insert(index).second ||
                whole(values, "consumer", 1) != (index == producers ? 1 : 0))
            {
                throw std::runtime_error("Wrong or duplicate shared-ptr lane");
            }
            number(values, "cpu_milliseconds", false);
            output["lanes"].append(row);
        }
        if (kind == "shared-ptr" && tag == "RESULT")
        {
            const double items = number(values, "items", true);
            number(values, "producers", true);
            const char* seconds[] = {"producer_seconds", "consumer_seconds"};
            const char* speeds[] = {"producer_items_per_second", "consumer_items_per_second"};
            for (size_t metric = 0; metric < 2; ++metric)
            {
                const double elapsed = number(values, seconds[metric], true);
                const double speed = number(values, speeds[metric], true);
                // The workload writes twelve significant digits for both time and rate.
                if (std::fabs(speed * elapsed / items - 1) > 1e-9)
                {
                    throw std::runtime_error("Shared-ptr count/time/rate disagree");
                }
            }
            if (whole(values, "case", 46) != 46 || whole(values, "status", 0) != 0 || values["valid"].asDouble() != 1 ||
                values["items"] != meta["items"] || values["producers"] != meta["producers"] ||
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
                // Compare the emitted configuration before merging it with metadata.
                const char* settings[] = {"producers", "mode", "capacity", "cancelable"};
                for (size_t setting = 0; setting < 4; ++setting)
                {
                    const char* key = settings[setting];
                    if (meta.isMember(key) && (!values.isMember(key) ||
                        whole(values, key, 20000000) != number(meta, key, false)))
                    {
                        throw std::runtime_error(std::string("Scheduling configuration mismatch: ") + key);
                    }
                }
                output["samples"].append(row);
            }
        }
        if (kind != "performance")
        {
            continue;
        }
        if (tag == "BENCH")
        {
            if (++completions != 1 || whole(values, "variant", 4) != whole(meta, "expected_variant", 4) ||
                whole(values, "profile", 5) != whole(meta, "expected_profile", 5) ||
                whole(values, "pure_items", 4000000) != 4000000 || whole(values, "concurrent_items", 1000000) != 1000000)
            {
                throw std::runtime_error("Benchmark executable or pool profile differs from the request");
            }
        }
        if (tag == "WAVE")
        {
            currentCase = values["case"].asString();
        }
        else if (tag == "SCENARIO")
        {
            const double producers = number(values, "producers", true);
            if (producers != std::floor(producers) || producers > 16)
            {
                throw std::runtime_error("Invalid scenario producer count");
            }
            currentCase = values["phase"].asString() + IO::integer(values["producers"].asInt());
        }
        else if (tag == "OBSERVER")
        {
            high = static_cast<int32_t>(whole(values, "record_high_water", 128));
        }
        else if (tag == "POLICY")
        {
            const double round = number(values, "cycle", false);
            if (round != std::floor(round) || round > 7)
            {
                throw std::runtime_error("Invalid policy cycle");
            }
            currentCase = values["case"].asString() + "." + values["phase"].asString();
            cycle = values["cycle"];
        }
        else if (tag == "BACKLOG")
        {
            currentCase = "backlog" + IO::integer(values["count"].asInt());
        }
        if (tag == "BURST")
        {
            currentCase = "burst" + IO::integer(static_cast<int64_t>(whole(values, "width", 65536)));
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
        if (!values.isMember("cycle"))
        {
            row["cycle"] = cycle;
        }
        const std::string scenario = currentCase + (cycle.isNull() ? "" : "#" + IO::integer(cycle.asInt()));
        if (tag == "WAVE_STAGE")
        {
            const double phase = number(values, "phase", false), round = number(values, "cycle", false);
            if (phase > 2 || phase != std::floor(phase) || round > 3 || round != std::floor(round) ||
                !expected.count(currentCase) || !stages.insert(currentCase + "#" + IO::integer(values["cycle"].asInt()) +
                    ":" + IO::integer(values["phase"].asInt())).second)
            {
                throw std::runtime_error("Wrong/duplicate wave stage");
            }
            ++waveStages;
            if (whole(values, "backlog", 1000) != (phase == 0 ? 1000 : 500) ||
                (!reference && whole(values, "capacity", 1500) < 750))
            {
                throw std::runtime_error("Wrong wave backlog/capacity");
            }
            output["pools"].append(row);
        }
        if (tag == "REGISTERED_PRODUCERS" || tag == "REGISTERED_MPSC" || tag == "BURST" || tag == "EXTRA" || tag == "POLICY_RESULT" || tag == "WAVE_RESULT")
        {
            if (suite == "35")
            {
                const std::string phase = tag == "REGISTERED_PRODUCERS" ? "producer" : "mpsc";
                if ((tag != "REGISTERED_PRODUCERS" && tag != "REGISTERED_MPSC") ||
                    phase + IO::integer(static_cast<int64_t>(whole(values, "producers", 16))) != currentCase)
                {
                    throw std::runtime_error("Measured producer count or phase differs from the requested scenario");
                }
            }
            else if ((suite == "38" && tag != "POLICY_RESULT") || (suite == "40" && tag != "WAVE_RESULT") ||
                (suite == "extra" && tag != "BURST" && tag != "EXTRA"))
            {
                throw std::runtime_error("Result row belongs to a different benchmark suite");
            }
            const double count = number(values, values.isMember("items") ? "items" : "operations", true);
            number(values, "elapsed_ns", true);
            if (!expected.count(scenario) || expected.find(scenario)->second != count || !measured.insert(scenario).second)
            {
                throw std::runtime_error("Unexpected/duplicate scenario or wrong count: " + scenario);
            }
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
            if (suite == "35" && (!expected.count(scenario) || !verified.insert(scenario).second))
            {
                throw std::runtime_error("Wrong/duplicate pool verification scenario");
            }
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
            number(values, "elapsed_ns", false);
            if (values["phase"].asString() == "shrink")
            {
                if (currentCase.compare(0, 14, "shrink_regrow_") != 0 ||
                    !expected.count(scenario) || !shrunk.insert(scenario).second)
                {
                    throw std::runtime_error("Wrong/duplicate shrink cycle");
                }
                ++shrink;
            }
            else if (values["phase"].asString() == "reclaim")
            {
                if (!expected.count(scenario) || !reclaimed.insert(scenario).second)
                {
                    throw std::runtime_error("Wrong/duplicate reclamation cycle");
                }
            }
            else
            {
                throw std::runtime_error("Unknown maintenance phase");
            }
            output["maintenance"].append(row);
        }
    }
    if (kind == "performance")
    {
        if (completions != 1 || measured.size() != expected.size() || !pass || code != 0)
        {
            throw std::runtime_error("Missing benchmark rows/PASS; got " + IO::integer(output["samples"].size()));
        }
        if (suite == "35" && !reference && verification != 10)
        {
            throw std::runtime_error("Missing pool verification windows");
        }
        if (suite == "38" && !reference && (shrink != 16 || reclaimed.size() != expected.size()))
        {
            throw std::runtime_error("Missing shrink/regrow/reclamation cycles");
        }
        if (suite == "40" && waveStages != 120)
        {
            throw std::runtime_error("Missing wave stages");
        }
    }
    else if (kind == "shared-ptr")
    {
        if (code || completions != 1 || output["samples"].size() != 2)
        {
            throw std::runtime_error("Missing shared-ptr measurement phase");
        }
        std::set<std::string> phases;
        for (Json::Value::UInt i = 0; i < 2; ++i)
        {
            phases.insert(output["samples"][i]["phase"].asString());
        }
        const size_t laneCount = static_cast<size_t>(whole(meta, "producers", 16)) + 1;
        if (phases != std::set<std::string>({"separate", "concurrent"}) ||
            sharedLanes["separate"].size() != laneCount || sharedLanes["concurrent"].size() != laneCount)
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
        const bool control = meta.get("negative_control", false).asBool();
        bool negative = control && code == 3 && summary["progress_valid"].asDouble() == 0;
        if (control && (!negative || summary["negative_control"].asDouble() != 1))
        {
            throw std::runtime_error("Scheduling negative control did not detect the injected stall");
        }
        if (summary["data_valid"].asDouble() != 1 || summary["same_priority"].asDouble() != 1 ||
            (!negative && (code != 0 || summary["progress_valid"].asDouble() != 1)))
        {
            throw std::runtime_error("Scheduling oracle failed");
        }
        std::set<int32_t> indices;
        const double duration = number(meta, "duration_ms", true);
        const double gapLimit = (std::max)(100.0, (std::min)(1000.0, std::floor(duration / 4)));
        if (number(summary, "max_cpu_gap_ms", true) != gapLimit)
        {
            throw std::runtime_error("Scheduling CPU gap limit differs from requested window");
        }
        const double exactLimit = 9007199254740991.0;
        double produced = 0, consumed = 0;
        const Json::Value& priority = output["lanes"][0u]["priority"];
        bool observedProgress = true;
        for (Json::Value::UInt i = 0; i < output["lanes"].size(); ++i)
        {
            const Json::Value& lane = output["lanes"][i];
            const int32_t index = static_cast<int32_t>(whole(lane, "index", 16));
            if (!indices.insert(index).second || index < 0 || index > meta["producers"].asInt())
            {
                throw std::runtime_error("Duplicate/invalid scheduling lane");
            }
            if (whole(lane, "consumer", 1) != (index == meta["producers"].asInt() ? 1 : 0))
            {
                throw std::runtime_error("Wrong consumer lane");
            }
            if (!priority.isNumeric() || !lane["priority"].isNumeric() || lane["priority"] != priority ||
                priority.asDouble() != std::floor(priority.asDouble()))
            {
                throw std::runtime_error("Scheduling lanes disagree on thread priority");
            }
            const bool admission = summary["capacity"].asInt() != 256 || lane["consumer"].asInt() == 1;
            const int32_t mode = summary["mode"].asInt();
            const bool capacityWait = !admission && (mode == 0 || mode == 1 || mode == 5 || mode == 7);
            const double gap = number(lane, "sampled_cpu_gap_ms", false);
            const double attempts = whole(lane, "active_attempts", exactLimit), cycles = whole(lane, "cycles", exactLimit);
            const double active = whole(lane, "active_count", exactLimit), count = whole(lane, "count", exactLimit);
            if (lane["consumer"].asInt())
            {
                consumed = count;
            }
            else
            {
                if (count > exactLimit - produced)
                {
                    throw std::runtime_error("Scheduling producer sum exceeds exact integer range");
                }
                produced += count;
            }
            if (active > count)
            {
                throw std::runtime_error("Scheduling active count exceeds final count");
            }
            observedProgress = observedProgress && gap <= gapLimit && (capacityWait ? count > 0 : attempts > 0) && cycles > 0 &&
                (!admission || (active > 0 && count > 0));
        }
        if (produced != consumed || produced != whole(summary, "items", exactLimit) ||
            consumed != whole(summary, "consumed", exactLimit) || whole(summary, "valid", 1) != (negative ? 0 : 1))
        {
            throw std::runtime_error("Scheduling totals or final verdict disagree with lane evidence");
        }
        if (observedProgress != (summary["progress_valid"].asDouble() == 1))
        {
            throw std::runtime_error("Scheduling lane progress disagrees with summary");
        }
    }
    else if (kind == "core")
    {
        std::set<int32_t> expected;
        for (Json::Value::UInt i = 0; i < meta["ids"].size(); ++i)
        {
            expected.insert(meta["ids"][i].asInt());
        }
        if (code || expected.empty() || passed != expected || pendingCount || !remainingBacklogs.empty() ||
            sawNotification != (expected.count(9) != 0) ||
            (meta["require_summary"].asBool() && completions != 1))
        {
            throw std::runtime_error("Core selection and PASS IDs disagree");
        }
    }
    else if (kind == "module" || kind == "demo")
    {
        if (code != 0 || completions != 1)
        {
            throw std::runtime_error("Missing or repeated completion evidence");
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
    const double count = number(row, row.isMember("items") ? "items" : "operations", true);
    const double elapsed = number(row, "elapsed_ns", true);
    if (!std::isfinite(extra) || extra < 0 || !std::isfinite(elapsed + extra))
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
    const char* keyFields[] = {"arch", "configuration", "profile", "suite", "case", "cycle", "phase", "mode", "producers", "mask", "cancelable", "capacity", "call", "operation", "index", "consumer"};
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
        std::string candidate = medians.count("current") ? "current" : "push";
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