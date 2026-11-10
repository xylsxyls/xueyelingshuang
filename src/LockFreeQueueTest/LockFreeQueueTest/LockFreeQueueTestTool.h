#pragma once
#include "../../JsonCpp/JsonCpp/src/json/json.h"
#include <stdint.h>
#include <map>
#include <set>
#include <string>
#include <vector>

/** Native test orchestration, never linked into a timed workload
CSystem and JsonCpp are privately compiled from repository sources; no shared SDK publication
*/
class LockFreeQueueTestTool
{
public:
    /** Parse strict host command options
    @param [in] argc Number of arguments
    @param [in] argv UTF8 arguments, borrowed for this call
    */
    LockFreeQueueTestTool(int32_t argc, char** argv);

    /** Dispatch the host command
    @return Exit status; exceptions reach the outer reporting boundary
    */
    int32_t run();

    /** Read an option or its default
    @param [in] key Option name without dashes
    @param [in] fallback Value when omitted
    @return Selected value
    */
    std::string option(const std::string& key, const std::string& fallback = "") const;

    /** Read a unique comma-separated selection
    @param [in] key Option name
    @param [in] fallback Default comma-separated selection
    @return Choices in requested order; empty/duplicate choices throw
    */
    std::vector<std::string> list(const std::string& key, const std::string& fallback) const;

    /** Read a bounded decimal option
    @param [in] key Option name
    @param [in] fallback Default value
    @param [in] low Inclusive lower bound
    @param [in] high Inclusive upper bound
    @return Valid integer; malformed/out-of-range values throw
    */
    int32_t number(const std::string& key, int32_t fallback, int32_t low, int32_t high) const;

    /** Inspect an explicitly supplied flag
    @param [in] key Flag name
    @return Whether the flag was supplied
    */
    bool flag(const std::string& key) const;

    /** Locate this checkout
    @return Canonical absolute repository root
    */
    std::string root() const;

    /** Locate the formal test sources
    @return Absolute test project directory
    */
    std::string tests() const;

    /** Locate the read-only production Queue sources
    @return Absolute library source directory
    */
    std::string source() const;

    /** Resolve the requested isolated batch
    @return Strict descendant of src/temp/LockFreeQueueTest; escapes throw
    */
    std::string batch() const;

    /** Enumerate one source directory without following reparse files
    @param [in] directory Existing UTF8 directory
    @return Sorted immediate .h/.inl/.cpp files
    */
    static std::vector<std::string> sources(const std::string& directory);

    /** Read a complete binary input
    @param [in] path UTF8 file path, maximum 256MiB
    @return Exact bytes; missing, partial or failed reads throw
    */
    static std::string read(const std::string& path);

    /** Write and verify a complete test artifact
    @param [in] path UTF8 destination, already checked against the batch boundary
    @param [in] data Exact bytes, possibly empty
    */
    static void write(const std::string& path, const std::string& data);

    /** Normalize source text for reversible patch matching
    @param [in] data UTF8 bytes, optionally BOM/CRLF
    @return BOM-free LF text, preserving final-newline presence
    */
    static std::string plain(const std::string& data);

    /** Encode a VS2013-compatible source copy
    @param [in] text UTF8 source text
    @return BOM/CRLF bytes with unchanged final-newline presence
    */
    static std::string compiled(const std::string& text);

    /** Calculate evidence identity with the platform SHA256 implementation
    @param [in] data Exact bytes
    @return Lowercase 64-digit hexadecimal digest
    */
    static std::string digest(const std::string& data);

    /** Parse a checked JSON input
    @param [in] path UTF8 input file
    @return Parsed value; read/parse failures throw
    */
    static Json::Value load(const std::string& path);

    /** Serialize checked test evidence
    @param [in] path Validated artifact path
    @param [in] value JSON data
    */
    static void save(const std::string& path, const Json::Value& value);

    /** Join a directory and test-relative name
    @param [in] directory Parent path
    @param [in] name Relative name, checked by the caller
    @return Combined path
    */
    static std::string join(const std::string& directory, const std::string& name);

    /** Resolve a canonical UTF8 path through CSystem
    @param [in] value Input path
    @param [in] existing Whether the target must exist
    @return Canonical absolute path; invalid paths throw
    */
    static std::string path(const std::string& value, bool existing = true);

    /** Obtain a test path's parent without changing directories
    @param [in] value File or directory path
    @return Lexical parent
    */
    static std::string parent(const std::string& value);

    /** Extract a source filename through CSystem
    @param [in] value Source path
    @return Filename including extension
    */
    static std::string name(const std::string& value);

    /** Check whether a UTF8 target resolves
    @param [in] value File or directory path
    @return Whether an existing target can be resolved
    */
    static bool exists(const std::string& value);

    /** Create a checked test directory and its missing parents
    @param [in] value UTF8 directory path
    */
    static void directory(const std::string& value);

    /** Enforce the private output boundary and check existing ancestors
    @param [in] value Proposed output path
    @param [in] allowed Required parent directory, which is not itself allowed
    */
    static void inside(const std::string& value, const std::string& allowed);

    /** Replace one uniquely reviewed source context
    @param [in] text Source text
    @param [in] before Nonempty unique context
    @param [in] after Replacement text
    @return Patched text; missing/ambiguous context throws
    */
    static std::string replaceOnce(const std::string& text, const std::string& before, const std::string& after);

    /** Split literal test selections or evidence lines
    @param [in] text Input text
    @param [in] delimiter Literal delimiter
    @return Tokens including empty tokens for later validation
    */
    static std::vector<std::string> split(const std::string& text, char delimiter);

    /** Encode an evidence counter or command integer
    @param [in] value Counter value
    @return Decimal text
    */
    static std::string integer(int64_t value);

    /** Locate this native tool
    @return UTF8 absolute executable path
    */
    static std::string executable();

    /** Quote a generated compiler-script argument
    @param [in] value Argument without cmd expansion characters
    @return Quoted argument; unsupported metacharacters throw
    */
    static std::string quote(const std::string& value);

    /** Capture one owned Windows child with a bounded deadline
    @param [in] command Absolute executable and UTF8 arguments
    @param [in] cwd Logical workload directory; compiler scripts explicitly enter it
    @param [in] log New absolute log path within the caller's batch
    @param [in] timeoutSeconds Positive deadline; timeout returns exit 124
    @return Exit, duration and command metadata; launch/cleanup setup failures throw
    */
    static Json::Value process(const std::vector<std::string>& command, const std::string& cwd,
        const std::string& log, int32_t timeoutSeconds);

    /** Query the affinity inherited by child workloads
    @return Windows process mask, or zero on a non-Windows host
    */
    static uint64_t affinity();

    /** Apply and verify one host affinity mask
    @param [in] mask Nonzero subset of the available Windows CPUs
    */
    static void affinity(uint64_t mask);

    /** Run deterministic parser, registry, patch, IO and deadline checks
    */
    void selfTest() const;

private:
    // Requested subcommand
    std::string m_command;
    // Parsed immutable command options
    std::map<std::string, std::string> m_options;
    // Canonical checkout root, resolved once
    std::string m_root;
};