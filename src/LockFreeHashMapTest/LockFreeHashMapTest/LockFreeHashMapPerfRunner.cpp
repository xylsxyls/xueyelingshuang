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
    const LockFreeHashMapPerfSpec& spec, int32_t repeats, bool memory, bool standard, const std::string& log, unsigned mask)
{
    unsigned multiplier = !memory && spec.m_iterationMultiplier != 1 ? static_cast<unsigned>(spec.m_iterationMultiplier) : 0;
    LockFreeHashMapRunResult run = LockFreeHashMapRunner::run(program, std::to_string(repeats * 1000 + spec.m_id), log, 120, mask, multiplier,
        aboveNormal());
    if (run.m_code != 0)
    {
        throw std::runtime_error("child failed: " + log + " exit=" + std::to_string(run.m_code));
    }
    std::ifstream input(log.c_str());
    std::map<std::string, double> result;
    if (!verifyObservation(input, spec, repeats, memory, standard, result))
    {
        throw std::runtime_error("invalid backend or result: " + log);
    }
    return result;
}

bool LockFreeHashMapPerfRunner::verifyObservation(std::istream& input, const LockFreeHashMapPerfSpec& spec,
    int32_t repeats, bool memory, bool standard, std::map<std::string, double>& result)
{
    bool identified = false, measured = false;
    std::string line;
    while (std::getline(input, line))
    {
        if (line.compare(0, 8, "BACKEND ") == 0)
        {
            const bool current = line == "BACKEND LockFreeHashMap";
            const bool locked = line == "BACKEND std_unordered_map_mutex max_load_factor=1 automatic_rehash=1";
            // 内存命令接收两种独立程序；吞吐命令必须与所选的 matrix 或 std 完全对应。
            if (identified || (!current && !locked) || (!memory && locked != standard))
            {
                return false;
            }
            identified = true;
        }
        else if (line.compare(0, 4, "MEM ") == 0 || line.compare(0, 5, "PERF ") == 0)
        {
            if (!identified || measured || !parse(line, spec, repeats, memory, result) ||
                (!memory && result["conditional_interfaces"] != (standard ? 0 : 1)))
            {
                return false;
            }
            measured = true;
        }
    }
    return input.eof() && measured;
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

bool LockFreeHashMapPerfRunner::matrix(const std::string& program, const std::string& prefix, int32_t rounds,
    unsigned affinityMask, int32_t first, int32_t last, bool standard, int32_t fixedRepeats, size_t iterationMultiplier)
{
    std::vector<LockFreeHashMapPerfSpec> specs = LockFreeHashMapPerfModels::catalogue();
    const int32_t end = last == 0 ? static_cast<int32_t>(specs.size()) : last;
    if (first < 1 || end < first || end > static_cast<int32_t>(specs.size()) || rounds < 1 || rounds > 9 ||
        fixedRepeats < 0 || fixedRepeats > 64)
    {
        return false;
    }
    // 标准表没有条件接口；空选择必须在创建证据文件或启动子进程之前拒绝。
    if (standard && std::find_if_not(specs.begin() + first - 1, specs.begin() + end,
        LockFreeHashMapPerfModels::currentOnly) == specs.begin() + end)
    {
        return false;
    }
    std::ofstream raw, summary, catalogue;
    if (!LockFreeHashMapRunner::openReport(prefix + "-runs.csv", raw) ||
        !LockFreeHashMapRunner::openReport(prefix + "-summary.csv", summary) ||
        !LockFreeHashMapRunner::openReport(prefix + "-catalogue.csv", catalogue))
    {
        return false;
    }
    raw << "id,name,round,repeats,ops_s,seconds,p50_us,p95_us,p99_us,samples,extra_calls,attempted_calls,affinity_mask,log\n";
    summary << "id,name,median_ops_s,min_ops_s,max_ops_s,rounds,repeats\n";
    catalogue << "id,name,family,keys,buckets,threads,read_percent,iterations,operation,full_collision,shared_keys,history,calls_per_iteration,current_only,iteration_multiplier\n";
    for (int32_t index = first - 1; index < end; ++index)
    {
        LockFreeHashMapPerfSpec& spec = specs[index];
        if (!LockFreeHashMapPerfModels::scaleIterations(spec, iterationMultiplier))
        {
            return false;
        }
        const bool conditional = LockFreeHashMapPerfModels::currentOnly(spec);
        catalogue << spec.m_id << ',' << spec.m_name << ',' << spec.m_family << ',' << spec.m_keys << ',' << spec.m_buckets
            << ',' << spec.m_threads << ',' << spec.m_readPercent << ',' << spec.m_iterations << ',' << spec.m_operation
            << ',' << spec.m_collision << ',' << spec.m_sharedKeys << ',' << spec.m_history << ','
            << LockFreeHashMapPerfModels::callsPerIteration(spec) << ',' << conditional << ',' << spec.m_iterationMultiplier << '\n';
        if (standard && conditional)
        {
            continue;
        }
        const unsigned mask = caseAffinity(spec.m_threads, affinityMask);
        int32_t repeats = 1;
        std::vector<double> rates;
        for (int32_t round = 0; round <= rounds; ++round)
        {
            const std::string log = prefix + "-id" + std::to_string(spec.m_id) + "-r" + std::to_string(round) + ".log";
            auto result = measure(program, spec, repeats, false, standard, log, mask);
            if (result["conditional_interfaces"] != (standard ? 0 : 1))
            {
                return false;
            }
            raw << std::setprecision(12) << spec.m_id << ',' << spec.m_name << ',' << round << ',' << repeats
                << ',' << result["ops"] << ',' << result["seconds"] << ',' << result["p50_us"] << ',' << result["p95_us"]
                << ',' << result["p99_us"] << ',' << result["samples"] << ',' << result["extra_calls"] << ','
                << result["attempted_calls"] << ',' << mask << ',' << log << '\n';
            if (round != 0)
            {
                rates.push_back(result["ops"]);
                continue;
            }
            repeats = fixedRepeats != 0 ? fixedRepeats : static_cast<int32_t>((std::min)(64.0, 0.12 / result["seconds"] + 1));
            if (spec.m_operation == LockFreeHashMapPerfClear)
            {
                repeats = (std::min)(repeats, 4);
            }
        }
        summary << std::setprecision(12) << spec.m_id << ',' << spec.m_name << ',' << median(rates) << ','
            << *std::min_element(rates.begin(), rates.end()) << ',' << *std::max_element(rates.begin(), rates.end())
            << ',' << rounds << ',' << repeats << '\n';
    }
    raw.close();
    summary.close();
    catalogue.close();
    return raw && summary && catalogue;
}

bool LockFreeHashMapPerfRunner::memoryMatrix(const std::string& program, const std::string& prefix, unsigned affinityMask)
{
    std::ofstream memory;
    if (!LockFreeHashMapRunner::openReport(prefix + "-memory.csv", memory))
    {
        throw std::runtime_error("cannot open memory report: " + prefix);
    }
    memory << "id,name,retained,peak,allocations,work_allocations,after_clear,after_free,net,log\n";
    std::vector<LockFreeHashMapPerfSpec> specs = LockFreeHashMapPerfModels::memoryCatalogue();
    for (size_t i = 0; i < specs.size(); ++i)
    {
        std::string log = prefix + "-memory-id" + std::to_string(specs[i].m_id) + ".log";
        std::map<std::string, double> result = measure(program, specs[i], 1, true, false, log, affinityMask);
        memory << std::setprecision(12) << specs[i].m_id << ',' << specs[i].m_name << ',' << result["retained"] << ',' << result["peak"] << ',' << result["allocations"] << ',' << result["work_allocations"]
            << ',' << result["after_clear"] << ',' << result["after_free"] << ',' << result["net"] << ',' << log << '\n';
        memory.flush();
        if (!memory)
        {
            throw std::runtime_error("cannot write memory report: " + prefix + " id=" + std::to_string(specs[i].m_id));
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
    std::string missingCapability(valid), wrongCapability(valid), standardCapability(valid);
    missingCapability.erase(missingCapability.find(" conditional_interfaces=1"), std::strlen(" conditional_interfaces=1"));
    wrongCapability.replace(wrongCapability.find("conditional_interfaces=1"), std::strlen("conditional_interfaces=1"), "conditional_interfaces=2");
    standardCapability.replace(standardCapability.find("conditional_interfaces=1"), std::strlen("conditional_interfaces=1"), "conditional_interfaces=0");
    if (parse(missingCapability, specs[0], 1, false, result) || parse(wrongCapability, specs[0], 1, false, result) ||
        !parse(standardCapability, specs[0], 1, false, result))
    {
        return false;
    }
    const std::string current = "BACKEND LockFreeHashMap\n";
    const std::string locked = "BACKEND std_unordered_map_mutex max_load_factor=1 automatic_rehash=1\n";
    // 使用手写的负载输出，只验证归属和判定，不启动实际的计时进程。
    const auto accepted = [&](const std::string& log, bool standard) -> bool
    {
        std::istringstream input(log);
        return verifyObservation(input, specs[0], 1, false, standard, result);
    };
    if (!accepted(current + valid, false) || !accepted(locked + standardCapability, true) ||
        accepted(locked + standardCapability, false) || accepted(current + valid, true) ||
        accepted(current + standardCapability, false) || accepted(locked + valid, true) ||
        accepted(valid, false) || accepted(current + current + valid, false) ||
        accepted(valid + "\n" + current, false) || accepted(current + valid + "\nMEM id=1", false))
    {
        return false;
    }
    LockFreeHashMapPerfSpec conditionalSpec = specs[0];
    conditionalSpec.m_operation = LockFreeHashMapPerfFind;
    if (!parse(valid, conditionalSpec, 1, false, result) || parse(standardCapability, conditionalSpec, 1, false, result))
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
@param [in] argv selftest、memory、matrix 或 std 及相应程序路径和参数
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
        if (argc == 5 && std::string(argv[1]) == "memory")
        {
            int mask = 0;
            if (std::string(argv[4]) != "0" && !LockFreeHashMapRunner::number(argv[4], (std::numeric_limits<int>::max)(), mask))
            {
                return 2;
            }
            return LockFreeHashMapPerfRunner::memoryMatrix(argv[2], argv[3], static_cast<unsigned>(mask)) ? 0 : 1;
        }
        if ((argc != 6 && argc != 8 && argc != 9 && argc != 10) ||
            (std::string(argv[1]) != "matrix" && std::string(argv[1]) != "std"))
        {
            return 2;
        }
        int rounds = 0, mask = 0, first = 1, last = 0, repeats = 0, multiplier = 1;
        if (!LockFreeHashMapRunner::number(argv[4], 9, rounds) ||
            (std::string(argv[5]) != "0" && !LockFreeHashMapRunner::number(argv[5], (std::numeric_limits<int>::max)(), mask)) ||
            (argc >= 8 && (!LockFreeHashMapRunner::number(argv[6], 999, first) || !LockFreeHashMapRunner::number(argv[7], 999, last))) ||
            (argc >= 9 && !LockFreeHashMapRunner::number(argv[8], 64, repeats)) ||
            (argc == 10 && !LockFreeHashMapRunner::number(argv[9], 256, multiplier)))
        {
            return 2;
        }
        return LockFreeHashMapPerfRunner::matrix(argv[2], argv[3], rounds, static_cast<unsigned>(mask), first, last,
            std::string(argv[1]) == "std", repeats, static_cast<size_t>(multiplier)) ? 0 : 1;
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