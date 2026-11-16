#include "LockFreeHashMapPerfRunner.h"
#include "LockFreeHashMapRunner.h"
#include "LockFreeHashMapPerfValidation.h"
#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <sstream>
#include <stdexcept>

bool LockFreeHashMapPerfRunner::aboveNormal()
{
    const char* option = std::getenv("LOCKFREEHASHMAP_PERF_ABOVE_NORMAL");
    if (option == nullptr || std::strcmp(option, "0") == 0)
    {
        return false;
    }
    if (std::strcmp(option, "1") == 0)
    {
        return true;
    }
    throw std::invalid_argument("LOCKFREEHASHMAP_PERF_ABOVE_NORMAL must be 0 or 1");
}

bool LockFreeHashMapPerfRunner::parse(const std::string& line, const LockFreeHashMapPerfSpec& spec,
    int32_t repeats, bool memory, std::map<std::string, double>& fields)
{
    if (repeats <= 0 || spec.m_threads <= 0 || spec.m_iterations == 0 ||
        spec.m_iterations % static_cast<size_t>(spec.m_threads) != 0)
    {
        return false;
    }
    std::istringstream input(line);
    std::string field;
    if (!(input >> field) || field != (memory ? "MEM" : "PERF"))
    {
        return false;
    }
    std::map<std::string, double> parsed;
    bool named = false;
    while (input >> field)
    {
        size_t separator = field.find('=');
        if (separator == std::string::npos || separator == 0 || separator + 1 == field.size())
        {
            return false;
        }
        std::string name = field.substr(0, separator);
        std::string value = field.substr(separator + 1);
        if (name == "name")
        {
            if (named || value != spec.m_name)
            {
                return false;
            }
            named = true;
            continue;
        }
        double number = -1;
        char tail = 0;
        std::istringstream numeric(value);
        if (!(numeric >> number) || (numeric >> tail) || !(number >= 0 && number < 1e100) || parsed.count(name))
        {
            return false;
        }
        parsed[name] = number;
    }
    if (!named || !parsed.count("id") || parsed["id"] != spec.m_id || !parsed.count("errors") || parsed["errors"] != 0)
    {
        return false;
    }
    const char* performance[] = { "repeats", "calls", "seconds", "ops", "p50_us", "p95_us", "p99_us", "samples", "batch_min_s", "batch_max_s", "extra_calls", "attempted_calls", "output_string_capacity", "state_tail_padding", "collection_validation", "conditional_interfaces" };
    const char* allocations[] = { "retained", "peak", "allocations", "work_allocations", "after_clear", "after_free", "net" };
    size_t count = memory ? sizeof(allocations) / sizeof(allocations[0]) : sizeof(performance) / sizeof(performance[0]);
    for (size_t i = 0; i < count; ++i)
    {
        if (!parsed.count(memory ? allocations[i] : performance[i]))
        {
            return false;
        }
    }
    if (memory)
    {
        for (size_t i = 0; i < sizeof(allocations) / sizeof(allocations[0]); ++i)
        {
            double value = parsed[allocations[i]];
            if (value != std::floor(value))
            {
                return false;
            }
        }
        if (parsed["net"] != 0 || parsed["retained"] > parsed["peak"] || parsed["after_clear"] > parsed["peak"] ||
            parsed["after_free"] > parsed["peak"] || parsed["work_allocations"] > parsed["allocations"])
        {
            return false;
        }
    }
    else
    {
        // 不能把输出准备不同的旧二进制与当前夹具混入同一性能对照。
        if (parsed["output_string_capacity"] != 512 || parsed["state_tail_padding"] != 64 || parsed["collection_validation"] != 2 ||
            (parsed["conditional_interfaces"] != 0 && parsed["conditional_interfaces"] != 1) ||
            (LockFreeHashMapPerfModels::currentOnly(spec) && parsed["conditional_interfaces"] != 1) ||
            (parsed.count("iteration_multiplier") && parsed["iteration_multiplier"] != spec.m_iterationMultiplier) ||
            (spec.m_iterationMultiplier != 1 && !parsed.count("iteration_multiplier")))
        {
            return false;
        }
        double calls = double(spec.m_iterations) * repeats * LockFreeHashMapPerfModels::callsPerIteration(spec);
        double measured = parsed["ops"] * parsed["seconds"];
        // 延迟独立采样批次只有一批；集合和clear逐次采样，其余按每线程的127间隔采样。
        size_t iterations = spec.m_iterations / spec.m_threads;
        size_t expectedSamples = spec.m_operation == LockFreeHashMapPerfClear ||
            (spec.m_operation >= LockFreeHashMapPerfKeys && spec.m_operation <= LockFreeHashMapPerfPairs) ?
            spec.m_iterations : size_t(spec.m_threads) * (iterations < 2048 ? iterations : (iterations - 1) / 127 + 1);
        if (parsed["repeats"] != repeats || parsed["calls"] != calls || parsed["attempted_calls"] != calls + parsed["extra_calls"] || parsed["seconds"] <= 0 || parsed["ops"] <= 0 ||
            measured < calls * 0.999999 || measured > calls * 1.000001 || parsed["samples"] != expectedSamples ||
            parsed["extra_calls"] != std::floor(parsed["extra_calls"]) ||
            parsed["p50_us"] > parsed["p95_us"] || parsed["p95_us"] > parsed["p99_us"] ||
            parsed["batch_min_s"] <= 0 || parsed["batch_min_s"] > parsed["batch_max_s"] ||
            parsed["seconds"] < parsed["batch_min_s"] * repeats * 0.999999 ||
            parsed["seconds"] > parsed["batch_max_s"] * repeats * 1.000001)
        {
            return false;
        }
    }
    fields.swap(parsed);
    return true;
}

std::map<std::string, double> LockFreeHashMapPerfRunner::measure(const std::string& program,
    const LockFreeHashMapPerfSpec& spec, int32_t repeats, bool memory, const std::string& log, unsigned mask)
{
    unsigned multiplier = !memory && spec.m_iterationMultiplier != 1 ? static_cast<unsigned>(spec.m_iterationMultiplier) : 0;
    LockFreeHashMapRunResult run = LockFreeHashMapRunner::run(program, std::to_string(repeats * 1000 + spec.m_id), log, 120, mask, multiplier,
        aboveNormal());
    if (run.m_code != 0)
    {
        throw std::runtime_error("child failed: " + log + " exit=" + std::to_string(run.m_code));
    }
    std::ifstream input(log.c_str());
    std::string line;
    std::map<std::string, double> result;
    size_t matches = 0;
    while (std::getline(input, line))
    {
        if (line.compare(0, memory ? 4 : 5, memory ? "MEM " : "PERF ") == 0)
        {
            if (++matches != 1 || !parse(line, spec, repeats, memory, result))
            {
                throw std::runtime_error("invalid result: " + log);
            }
        }
    }
    if (!input.eof() || matches != 1)
    {
        throw std::runtime_error("missing result: " + log);
    }
    return result;
}

double LockFreeHashMapPerfRunner::median(std::vector<double> values)
{
    if (values.empty())
    {
        throw std::runtime_error("empty median");
    }
    std::sort(values.begin(), values.end());
    return (values[(values.size() - 1) / 2] + values[values.size() / 2]) / 2;
}

unsigned LockFreeHashMapPerfRunner::caseAffinity(int32_t threads, unsigned allowed)
{
    return threads == 1 ? allowed & (~allowed + 1U) : allowed;
}

bool LockFreeHashMapPerfRunner::compare(const std::vector<std::string>& programs, const std::string& prefix,
    int32_t rounds, unsigned affinityMask, int32_t first, int32_t last, bool revisions, int32_t fixedRepeats,
    size_t iterationMultiplier)
{
    std::vector<LockFreeHashMapPerfSpec> specs = LockFreeHashMapPerfModels::catalogue();
    if ((programs.size() != 5 && programs.size() != 7) || rounds < 1 || rounds > 9 || first < 1 || first > int32_t(specs.size()) ||
        last < 0 || last > int32_t(specs.size()) || (last != 0 && last < first) || fixedRepeats < 0 || fixedRepeats > 64)
    {
        return false;
    }
    int32_t end = last == 0 ? static_cast<int32_t>(specs.size()) : last;
    for (int32_t index = first - 1; index < end; ++index)
    {
        if (!LockFreeHashMapPerfModels::scaleIterations(specs[index], iterationMultiplier))
        {
            return false;
        }
    }
    int32_t versionCount = static_cast<int32_t>((programs.size() - 1) / 2);
    const char* versions[] = { "original", "current", "std_mutex" };
    std::ofstream raw, summary, catalogue;
    if (!LockFreeHashMapRunner::openReport(prefix + "-runs.csv", raw) ||
        !LockFreeHashMapRunner::openReport(prefix + "-comparison.csv", summary) ||
        !LockFreeHashMapRunner::openReport(prefix + "-catalogue.csv", catalogue))
    {
        throw std::runtime_error("cannot open performance reports: " + prefix);
    }
    raw << "id,name,phase,version,round,repeats,ops_s,seconds,p50_us,p95_us,p99_us,samples,extra_calls,attempted_calls,affinity_mask,log\n";
    summary << "id,name,original_ops_s,current_ops_s,current_over_original,original_p50_us,current_p50_us,original_p95_us,current_p95_us,original_p99_us,current_p99_us,original_min_ops_s,original_max_ops_s,current_min_ops_s,current_max_ops_s,original_median_seconds,current_median_seconds,samples,rounds,repeats,current_only,original_extra_calls,current_extra_calls,current_vs_original_percent";
    if (versionCount == 3)
    {
        summary << ",std_ops_s,current_over_std,current_vs_std_percent,std_p50_us,std_p95_us,std_p99_us,std_min_ops_s,std_max_ops_s,std_median_seconds,std_extra_calls";
    }
    summary << '\n';
    catalogue << "id,name,family,keys,buckets,threads,read_percent,iterations,operation,full_collision,shared_keys,history,calls_per_iteration,current_only,iteration_multiplier\n";
    for (int32_t index = first - 1; index < end; ++index)
    {
        const LockFreeHashMapPerfSpec& spec = specs[index];
        const unsigned effectiveAffinity = caseAffinity(spec.m_threads, affinityMask);
        bool only = LockFreeHashMapPerfModels::currentOnly(spec) && !revisions;
        catalogue << spec.m_id << ',' << spec.m_name << ',' << spec.m_family << ',' << spec.m_keys << ',' << spec.m_buckets
            << ',' << spec.m_threads << ',' << spec.m_readPercent << ',' << spec.m_iterations << ',' << spec.m_operation
            << ',' << spec.m_collision << ',' << spec.m_sharedKeys << ',' << spec.m_history << ','
            << LockFreeHashMapPerfModels::callsPerIteration(spec) << ',' << only << ',' << spec.m_iterationMultiplier << '\n';
        catalogue.flush();
        std::map<std::string, double> pilots[3];
        for (int32_t version = only ? 1 : 0; version < (only ? 2 : versionCount); ++version)
        {
            std::string log = prefix + "-pilot-id" + std::to_string(spec.m_id) + '-' + versions[version] + ".log";
            pilots[version] = measure(only ? programs.back() : programs[version], spec, 1, false, log, effectiveAffinity);
            if (revisions && pilots[version]["conditional_interfaces"] != 1)
            {
                // revision比较的两版都支持条件接口；首项就拒绝错误编译开关，不能混用不同循环代码。
                throw std::runtime_error("revision requires conditional interfaces in both binaries: " + programs[version]);
            }
            raw << std::setprecision(12) << spec.m_id << ',' << spec.m_name << ",pilot," << versions[version]
                << ",0,1," << pilots[version]["ops"] << ',' << pilots[version]["seconds"] << ','
                << pilots[version]["p50_us"] << ',' << pilots[version]["p95_us"] << ',' << pilots[version]["p99_us"]
                << ',' << pilots[version]["samples"] << ',' << pilots[version]["extra_calls"] << ',' << pilots[version]["attempted_calls"] << ',' << effectiveAffinity << ',' << log << '\n';
            raw.flush();
        }
        double fast = only ? pilots[1]["seconds"] : (std::min)(pilots[0]["seconds"], pilots[1]["seconds"]);
        double slow = only ? fast : (std::max)(pilots[0]["seconds"], pilots[1]["seconds"]);
        if (!only && versionCount == 3)
        {
            fast = (std::min)(fast, pilots[2]["seconds"]);
            slow = (std::max)(slow, pilots[2]["seconds"]);
        }
        int32_t repeats = static_cast<int32_t>((std::min)(64.0, 0.12 / fast + 1));
        if (spec.m_operation == LockFreeHashMapPerfClear)
        {
            // 每批已包含一千零二十四张满表，限制批次数，避免清空短耗时反推出过量建表。
            repeats = (std::min)(repeats, 4);
        }
        repeats = (std::max)(1, (std::min)(repeats, static_cast<int32_t>((std::min)(64.0, 0.5 / slow))));
        if (fixedRepeats != 0)
        {
            // 一次慢校准不能迫使后续复核一直使用短样本；两版接受完全相同的明确批次预算
            repeats = spec.m_operation == LockFreeHashMapPerfClear ? (std::min)(fixedRepeats, 4) : fixedRepeats;
        }
        std::map<std::string, std::vector<double>> values[3];
        size_t samples = 0;
        for (int32_t round = 0; round < rounds; ++round)
        {
            for (int32_t order = 0; order < (only ? 1 : versionCount); ++order)
            {
                int32_t version = only ? 1 : (round + order + spec.m_id) % versionCount;
                std::string log = prefix + "-id" + std::to_string(spec.m_id) + '-' + versions[version] + "-r" + std::to_string(round + 1) + ".log";
                std::map<std::string, double> measured = measure(only ? programs.back() : programs[version], spec, repeats, false, log, effectiveAffinity);
                for (std::map<std::string, double>::const_iterator item = measured.begin(); item != measured.end(); ++item)
                {
                    values[version][item->first].push_back(item->second);
                }
                samples = static_cast<size_t>(measured["samples"]);
                raw << std::setprecision(12) << spec.m_id << ',' << spec.m_name << ",measured," << versions[version] << ','
                    << round + 1 << ',' << repeats << ',' << measured["ops"] << ',' << measured["seconds"] << ','
                    << measured["p50_us"] << ',' << measured["p95_us"] << ',' << measured["p99_us"] << ',' << samples
                    << ',' << measured["extra_calls"] << ',' << measured["attempted_calls"] << ',' << effectiveAffinity << ',' << log << '\n';
                raw.flush();
            }
        }
        double rate[2] = { only ? 0 : median(values[0]["ops"]), median(values[1]["ops"]) };
        summary << std::setprecision(12) << spec.m_id << ',' << spec.m_name << ',';
        if (!only)
        {
            summary << rate[0];
        }
        summary << ',' << rate[1] << ',';
        if (!only)
        {
            summary << rate[1] / rate[0];
        }
        const char* percentiles[] = { "p50_us", "p95_us", "p99_us" };
        for (size_t i = 0; i < 3; ++i)
        {
            summary << ',';
            if (!only)
            {
                summary << median(values[0][percentiles[i]]);
            }
            summary << ',' << median(values[1][percentiles[i]]);
        }
        for (int32_t version = 0; version < 2; ++version)
        {
            summary << ',';
            if (!only || version == 1)
            {
                summary << *std::min_element(values[version]["ops"].begin(), values[version]["ops"].end());
            }
            summary << ',';
            if (!only || version == 1)
            {
                summary << *std::max_element(values[version]["ops"].begin(), values[version]["ops"].end());
            }
        }
        summary << ',';
        if (!only)
        {
            summary << median(values[0]["seconds"]);
        }
        summary << ',' << median(values[1]["seconds"]) << ',' << samples << ',' << rounds << ',' << repeats << ',' << only << ',';
        if (!only)
        {
            double total = 0;
            for (size_t i = 0; i < values[0]["extra_calls"].size(); ++i)
            {
                total += values[0]["extra_calls"][i];
            }
            summary << total;
        }
        double total = 0;
        for (size_t i = 0; i < values[1]["extra_calls"].size(); ++i)
        {
            total += values[1]["extra_calls"][i];
        }
        summary << ',' << total << ',';
        if (!only)
        {
            summary << (rate[1] / rate[0] - 1) * 100;
        }
        double standard = 0;
        if (versionCount == 3)
        {
            if (only)
            {
                // 原版和 std 都没有版本快照接口，不用普通 get 冒充同等语义
                summary << ",,,,,,,,,,";
            }
            else
            {
                standard = median(values[2]["ops"]);
                double extra = 0;
                for (size_t i = 0; i < values[2]["extra_calls"].size(); ++i)
                {
                    extra += values[2]["extra_calls"][i];
                }
                summary << ',' << standard << ',' << rate[1] / standard << ',' << (rate[1] / standard - 1) * 100;
                for (size_t i = 0; i < 3; ++i)
                {
                    summary << ',' << median(values[2][percentiles[i]]);
                }
                summary << ',' << *std::min_element(values[2]["ops"].begin(), values[2]["ops"].end())
                    << ',' << *std::max_element(values[2]["ops"].begin(), values[2]["ops"].end())
                    << ',' << median(values[2]["seconds"]) << ',' << extra;
            }
        }
        summary << '\n';
        summary.flush();
        if (!raw || !summary || !catalogue)
        {
            throw std::runtime_error("cannot write performance reports: " + prefix + " id=" + std::to_string(spec.m_id));
        }
        std::cout << "PERF_CASE " << spec.m_id << '/' << specs.size() << ' ' << spec.m_name
            << " original=" << rate[0] << " current=" << rate[1] << " ratio=" << (only ? 0 : rate[1] / rate[0])
            << " std=" << standard << " current_over_std=" << (standard == 0 ? 0 : rate[1] / standard) << std::endl;
    }
    if (first == 1 && last == 0)
    {
        std::vector<std::string> memoryPrograms(programs.begin() + versionCount, programs.begin() + versionCount * 2);
        if (!compareMemory(memoryPrograms, prefix, affinityMask))
        {
            return false;
        }
    }
    raw.close();
    summary.close();
    catalogue.close();
    if (!raw || !summary || !catalogue)
    {
        throw std::runtime_error("cannot close performance reports: " + prefix);
    }
    return true;
}

bool LockFreeHashMapPerfRunner::compareMemory(const std::vector<std::string>& programs, const std::string& prefix, unsigned affinityMask)
{
    if (programs.size() != 2 && programs.size() != 3)
    {
        return false;
    }
    const char* versions[] = { "original", "current", "std_mutex" };
    std::ofstream memory;
    if (!LockFreeHashMapRunner::openReport(prefix + "-memory.csv", memory))
    {
        throw std::runtime_error("cannot open memory report: " + prefix);
    }
    memory << "id,name,version,retained,peak,allocations,work_allocations,after_clear,after_free,net,log\n";
    std::vector<LockFreeHashMapPerfSpec> specs = LockFreeHashMapPerfModels::memoryCatalogue();
    for (size_t i = 0; i < specs.size(); ++i)
    {
        for (size_t version = 0; version < programs.size(); ++version)
        {
            std::string log = prefix + "-memory-id" + std::to_string(specs[i].m_id) + '-' + versions[version] + ".log";
            std::map<std::string, double> result = measure(programs[version], specs[i], 1, true, log, affinityMask);
            memory << std::setprecision(12) << specs[i].m_id << ',' << specs[i].m_name << ',' << versions[version]
                << ',' << result["retained"] << ',' << result["peak"] << ',' << result["allocations"] << ',' << result["work_allocations"]
                << ',' << result["after_clear"] << ',' << result["after_free"] << ',' << result["net"] << ',' << log << '\n';
            memory.flush();
            if (!memory)
            {
                throw std::runtime_error("cannot write memory report: " + prefix + " id=" + std::to_string(specs[i].m_id));
            }
        }
    }
    memory.close();
    return bool(memory);
}

bool LockFreeHashMapPerfRunner::selftest()
{
    if (caseAffinity(1, 0) != 0 || caseAffinity(1, 0x7fffffcfU) != 1 ||
        caseAffinity(2, 0x7fffffcfU) != 0x7fffffcfU || caseAffinity(32, 0) != 0)
    {
        return false;
    }
    for (unsigned bit = 0; bit < std::numeric_limits<unsigned>::digits; ++bit)
    {
        const unsigned selected = 1U << bit;
        const unsigned allowed = ~0U << bit;
        if (caseAffinity(1, selected) != selected || caseAffinity(1, allowed) != selected ||
            caseAffinity(16, allowed) != allowed)
        {
            return false;
        }
    }
    if (!LockFreeHashMapPerfValidation::selftest())
    {
        return false;
    }
    std::cout << "PERF_PRIORITY above_normal=" << aboveNormal() << std::endl;
    std::vector<LockFreeHashMapPerfSpec> specs = LockFreeHashMapPerfModels::catalogue();
    std::map<std::string, bool> names;
    for (size_t i = 0; i < specs.size(); ++i)
    {
        if (specs[i].m_id != int32_t(i + 1) || names[specs[i].m_name])
        {
            return false;
        }
        names[specs[i].m_name] = true;
    }
    std::string valid = "PERF id=1 name=" + specs[0].m_name + " repeats=1 calls=1048576 seconds=1 ops=1048576 p50_us=1 p95_us=2 p99_us=3 samples=8257 batch_min_s=1 batch_max_s=1 extra_calls=0 attempted_calls=1048576 output_string_capacity=512 state_tail_padding=64 collection_validation=2 conditional_interfaces=1 errors=0";
    std::map<std::string, double> result;
    std::vector<double> samples;
    samples.push_back(3);
    samples.push_back(1);
    samples.push_back(2);
    LockFreeHashMapPerfSpec scaled = specs[0];
    LockFreeHashMapPerfSpec overflow = specs[0];
    overflow.m_iterations = (std::numeric_limits<size_t>::max)();
    LockFreeHashMapPerfSpec finite = specs[0];
    finite.m_operation = LockFreeHashMapPerfErase;
    std::string longValid = "PERF id=1 name=" + specs[0].m_name + " repeats=1 calls=268435456 seconds=1 ops=268435456 p50_us=1 p95_us=2 p99_us=3 samples=" +
        std::to_string((size_t(268435456) - 1) / 127 + 1) + " batch_min_s=1 batch_max_s=1 extra_calls=0 attempted_calls=268435456 iteration_multiplier=256 output_string_capacity=512 state_tail_padding=64 collection_validation=2 conditional_interfaces=1 errors=0";
    std::string missingCapability(valid), wrongCapability(valid), legacyCapability(valid);
    missingCapability.erase(missingCapability.find(" conditional_interfaces=1"), std::strlen(" conditional_interfaces=1"));
    wrongCapability.replace(wrongCapability.find("conditional_interfaces=1"), std::strlen("conditional_interfaces=1"), "conditional_interfaces=2");
    legacyCapability.replace(legacyCapability.find("conditional_interfaces=1"), std::strlen("conditional_interfaces=1"), "conditional_interfaces=0");
    if (parse(missingCapability, specs[0], 1, false, result) || parse(wrongCapability, specs[0], 1, false, result) ||
        !parse(legacyCapability, specs[0], 1, false, result))
    {
        return false;
    }
    LockFreeHashMapPerfSpec conditionalSpec = specs[0];
    conditionalSpec.m_operation = LockFreeHashMapPerfFind;
    if (!parse(valid, conditionalSpec, 1, false, result) || parse(legacyCapability, conditionalSpec, 1, false, result))
    {
        return false;
    }
    std::string missingPreparation(valid), wrongPreparation(valid);
    missingPreparation.erase(missingPreparation.find(" output_string_capacity=512"), std::strlen(" output_string_capacity=512"));
    wrongPreparation.replace(wrongPreparation.find("output_string_capacity=512"), std::strlen("output_string_capacity=512"), "output_string_capacity=0");
    if (parse(missingPreparation, specs[0], 1, false, result) || parse(wrongPreparation, specs[0], 1, false, result))
    {
        return false;
    }
    std::string missingCollection(valid), wrongCollection(valid);
    missingCollection.erase(missingCollection.find(" collection_validation=2"), std::strlen(" collection_validation=2"));
    wrongCollection.replace(wrongCollection.find("collection_validation=2"), std::strlen("collection_validation=2"), "collection_validation=1");
    if (parse(missingCollection, specs[0], 1, false, result) || parse(wrongCollection, specs[0], 1, false, result))
    {
        return false;
    }
    std::string missingIsolation(valid), wrongIsolation(valid);
    missingIsolation.erase(missingIsolation.find(" state_tail_padding=64"), std::strlen(" state_tail_padding=64"));
    wrongIsolation.replace(wrongIsolation.find("state_tail_padding=64"), std::strlen("state_tail_padding=64"), "state_tail_padding=0");
    if (parse(missingIsolation, specs[0], 1, false, result) || parse(wrongIsolation, specs[0], 1, false, result))
    {
        return false;
    }
    if (!LockFreeHashMapPerfModels::scaleIterations(scaled, 256) || scaled.m_iterations != 268435456 ||
        scaled.m_keys != specs[0].m_keys || scaled.m_threads != specs[0].m_threads ||
        !parse(longValid, scaled, 1, false, result) || parse(valid, scaled, 1, false, result) ||
        LockFreeHashMapPerfModels::scaleIterations(overflow, 2) || overflow.m_iterations != (std::numeric_limits<size_t>::max)() ||
        LockFreeHashMapPerfModels::scaleIterations(finite, 2) || finite.m_iterations != specs[0].m_iterations ||
        LockFreeHashMapPerfModels::scaleIterations(scaled, 2) ||
        LockFreeHashMapPerfModels::scaleIterations(finite, 0) || LockFreeHashMapPerfModels::scaleIterations(finite, 257))
    {
        return false;
    }
    std::string missingSamples = valid;
    missingSamples.replace(missingSamples.find("samples=8257"), std::strlen("samples=8257"), "samples=1");
    std::string fractionalCalls = valid;
    std::string wrongBatch = valid;
    wrongBatch.replace(wrongBatch.find("batch_min_s=1 batch_max_s=1"), std::strlen("batch_min_s=1 batch_max_s=1"), "batch_min_s=2 batch_max_s=2");
    fractionalCalls.replace(fractionalCalls.find("extra_calls=0"), std::strlen("extra_calls=0"), "extra_calls=0.5");
    fractionalCalls.replace(fractionalCalls.find("attempted_calls=1048576"), std::strlen("attempted_calls=1048576"), "attempted_calls=1048576.5");
    const LockFreeHashMapPerfSpec memorySpec = LockFreeHashMapPerfModels::memoryCatalogue()[0];
    const std::string memory = "MEM id=" + std::to_string(memorySpec.m_id) + " name=" + memorySpec.m_name +
        " retained=1 peak=1 allocations=1 work_allocations=1 after_clear=0 after_free=0 net=0 errors=0";
    std::string fractionalBytes = memory;
    fractionalBytes.replace(fractionalBytes.find("retained=1"), std::strlen("retained=1"), "retained=0.5");
    return parse(valid, specs[0], 1, false, result) && !parse(missingSamples, specs[0], 1, false, result) &&
        parse(memory, memorySpec, 1, true, result) && !parse(fractionalBytes, memorySpec, 1, true, result) &&
        !parse(fractionalCalls, specs[0], 1, false, result) && !parse(wrongBatch, specs[0], 1, false, result) &&
        !parse(valid + " =0", specs[0], 1, false, result) && !parse(valid + " invalid=", specs[0], 1, false, result) &&
        !parse(valid + " errors=0", specs[0], 1, false, result) &&
        !parse(valid.substr(0, valid.find(" errors=")), specs[0], 1, false, result) &&
        !parse(valid + " invalid=nan", specs[0], 1, false, result) && !parse(valid, specs[0], 2, false, result) &&
        LockFreeHashMapPerfModels::percentile(samples, 50) == 2 && median(samples) == 2 &&
        LockFreeHashMapPerfModels::memoryCatalogue().size() == 24;
}

/** 控制器入口，全部进程操作、校准和统计由 C++ 完成
@param [in] argc 参数数量
@param [in] argv selftest；两版参数保持原协议；three 为三方吞吐与内存；memory 为两版或三版内存程序、前缀、掩码
@note 比较命令可在 first last 后追加 1 至 64，固定每轮批次数；省略时保持原校准方式
@note 固定批次数后可再追加 1 至 256 的 get/set 连续循环倍率，原有短批次结果另行保留
@return 0 完成且通过，1 数据失败，2 参数错误，3 设施异常
*/
int main(int argc, char** argv)
{
    try
    {
        if (argc == 2 && std::string(argv[1]) == "selftest")
        {
            bool passed = LockFreeHashMapPerfRunner::selftest();
            std::cout << "PERF_RUNNER_SELFTEST passed=" << passed << " cases=" << LockFreeHashMapPerfModels::catalogue().size() << std::endl;
            return passed ? 0 : 1;
        }
        if ((argc == 6 || argc == 7) && std::string(argv[1]) == "memory")
        {
            int mask = 0;
            if (std::string(argv[argc - 1]) != "0" &&
                !LockFreeHashMapRunner::number(argv[argc - 1], (std::numeric_limits<int>::max)(), mask))
            {
                return 2;
            }
            std::vector<std::string> programs;
            for (int i = 2; i < argc - 2; ++i)
            {
                programs.push_back(argv[i]);
            }
            return LockFreeHashMapPerfRunner::compareMemory(programs, argv[argc - 2], static_cast<unsigned>(mask)) ? 0 : 1;
        }
        bool revisions = argc > 1 && std::string(argv[1]) == "revision";
        if (revisions)
        {
            // 保持两版原参数布局，只剥离命令前缀；两份性能程序都须启用新增接口
            --argc;
            ++argv;
        }
        bool three = argc > 1 && std::string(argv[1]) == "three";
        if (three ? (argc != 12 && argc != 14 && argc != 15 && argc != 16) : (argc != 9 && argc != 11 && argc != 12 && argc != 13))
        {
            return 2;
        }
        int rounds = 0, mask = 0, first = 1, last = 0, fixedRepeats = 0, iterationMultiplier = 1;
        int offset = three ? 3 : 0;
        if (!LockFreeHashMapRunner::number(argv[7 + offset], 9, rounds) ||
            (std::string(argv[8 + offset]) != "0" && !LockFreeHashMapRunner::number(argv[8 + offset], (std::numeric_limits<int>::max)(), mask)) ||
            (argc >= 11 + offset && (!LockFreeHashMapRunner::number(argv[9 + offset], 999, first) || !LockFreeHashMapRunner::number(argv[10 + offset], 999, last))) ||
            (argc >= 12 + offset && !LockFreeHashMapRunner::number(argv[11 + offset], 64, fixedRepeats)) ||
            (argc == 13 + offset && !LockFreeHashMapRunner::number(argv[12 + offset], 256, iterationMultiplier)))
        {
            return 2;
        }
        std::vector<std::string> programs;
        for (int i = three ? 2 : 1; i <= (three ? 8 : 5); ++i)
        {
            programs.push_back(argv[i]);
        }
        return LockFreeHashMapPerfRunner::compare(programs, argv[6 + offset], rounds, static_cast<unsigned>(mask), first, last, revisions, fixedRepeats,
            static_cast<size_t>(iterationMultiplier)) ? 0 : 1;
    }
    catch (const std::exception& error)
    {
        std::cerr << "PERF_CONTROLLER failed: " << error.what() << std::endl;
    }
    catch (...)
    {
        std::cerr << "PERF_CONTROLLER unexpected exception" << std::endl;
    }
    return 3;
}