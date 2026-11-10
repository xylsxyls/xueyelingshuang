#pragma once
#include "LockFreeQueueTestTool.h"

/** Frozen-input, serial subprocess runner for the existing 46 cases and benchmark matrices. */
class LockFreeQueueTestRunner
{
public:
    /** Borrow options for one synchronous run
    @param [in] tool Options, valid until this runner is destroyed
    */
    explicit LockFreeQueueTestRunner(const LockFreeQueueTestTool& tool);

    /** Materialize and execute the requested matrix
    @param [in] command Valid host subcommand
    @return Zero on completion; failures throw to the outer reporting boundary
    */
    int32_t run(const std::string& command);

    /** Resolve the stable case registry
    @param [in] text all/instant/stress or comma-separated IDs/ranges
    @return Unique ascending IDs; invalid selections throw
    */
    static std::set<int32_t> select(const std::string& text);

    /** Identify cases executed by the core binary
    @param [in] id Stable case ID
    @return Whether the ID belongs to the 36-case core
    */
    static bool core(int32_t id);

    /** Identify the registered pressure cases
    @param [in] id Stable case ID
    @return Whether the case belongs to the 15-case pressure selection
    */
    static bool pressure(int32_t id);

private:
    /** Freeze a library revision and the unchanged workload sources
    @param [in] version Private input directory name
    @param [in] source Read-only library source directory
    */
    void snapshot(const std::string& version, const std::string& source);

    /** Freeze the original headers from the requested directory or pinned Git commit
    */
    void baseline();

    /** Preserve exact bytes and reject attempts to overwrite different evidence
    @param [in] path Destination strictly within this batch
    @param [in] bytes Complete intended content
    */
    void freeze(const std::string& path, const std::string& bytes);

    /** Recheck every frozen source and executable SHA256, throwing on any mismatch
    */
    void verifyFrozen() const;

    /** Register a private compiler script without executing it
    @param [in] arch Win32 or x64
    @param [in] configuration Debug or Release
    @param [in] version Isolated build-directory name
    @param [in] target Executable stem
    @param [in] commands Compiler/linker argv arrays, each expected to exit zero
    */
    void build(const std::string& arch, const std::string& configuration, const std::string& version,
        const std::string& target, const std::vector<std::vector<std::string>>& commands);

    /** Select the preserved workload compiler settings
    @param [in] configuration Debug or Release
    @return VS2013 compiler executable and common flags
    */
    std::vector<std::string> flags(const std::string& configuration) const;

    /** Append one uniquely named command and its independent oracle
    @param [in] label Unique relative evidence path stem
    @param [in] command Executable and arguments
    @param [in] cwd Logical workload directory, enforced by generated scripts
    @param [in] meta Workload identity and expected outcome
    @param [in] timeout Positive deadline in seconds
    */
    void add(const std::string& label, const std::vector<std::string>& command, const std::string& cwd,
        const Json::Value& meta, int32_t timeout = 180);

    /** Materialize selected stable IDs, with instant checks before pressure checks
    */
    void functional();

    /** Materialize the preserved capacity/interface/version matrix
    @param [in] mode performance, three-way or revision
    */
    void performance(const std::string& mode);

    /** Materialize equal-priority producer/consumer progress scenarios
    */
    void scheduling();

    /** Materialize separate and simultaneous shared_ptr throughput scenarios
    */
    void sharedPtr();

    /** Execute pending commands serially, preserving failures and validating each oracle
    */
    void execute();

    /** Locate a private compiler output directory
    @param [in] arch Win32 or x64
    @param [in] version Frozen revision/call combination
    @return Absolute batch-local directory
    */
    std::string buildDirectory(const std::string& arch, const std::string& version) const;

    /** Locate a frozen workload/library file
    @param [in] version Frozen revision directory name
    @param [in] filename File name, or empty for the directory
    @return Absolute batch-local path
    */
    std::string input(const std::string& version, const std::string& filename) const;

    /** Validate and expand the platform option
    @param [in] fallback Default platform when omitted
    @return One or both Windows target architectures
    */
    std::vector<std::string> arches(const std::string& fallback = "both") const;

    /** Validate an explicit comma-separated selection
    @param [in] option CLI key
    @param [in] defaults Default comma-separated choices
    @param [in] allowed Complete set of allowed choices
    @return Requested choices without duplicates
    */
    std::vector<std::string> validatedList(const std::string& option, const std::string& defaults,
        const std::string& allowed) const;

    /** Parse a nonzero CPU mask without narrowing on a 32-bit host
    @param [in] text Unsigned decimal or hexadecimal mask
    @return Valid mask; malformed/overflowing values throw
    */
    static uint64_t cpuMask(const std::string& text);

private:
    // Borrowed synchronous options; never owned or modified
    const LockFreeQueueTestTool* m_tool;
    // Strict descendant of the QueueTest temporary root
    std::string m_batch;
    // Complete ordered command list, fixed before execution
    Json::Value m_plan;
    // SHA256 identities of frozen source, scripts and binaries
    Json::Value m_hashes;
    // Completed command evidence, including retained failures
    Json::Value m_commands;
    // Original host affinity, restored on both success and failure
    uint64_t m_inheritedAffinity;
};