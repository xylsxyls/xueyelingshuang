#pragma once
#include "LockFreeQueueTestTool.h"

/** Independent checks on raw workload output, plus paired median/percentage exports. */
class LockFreeQueueTestResults
{
public:
    /** Parse finite numeric/string fields and reject duplicate keys
    @param [in] line One workload output line
    @return Field object; invalid numeric evidence throws
    */
    static Json::Value fields(const std::string& line);

    /** Validate the workload counts and independent oracles
    @param [in] text Complete raw log, interpreted as UTF8/ASCII fields
    @param [in] meta Frozen workload identity and expected outcome
    @param [in] exitCode Actual child status
    @return Validated tables; incomplete or failed evidence throws
    */
    static Json::Value parse(const std::string& text, const Json::Value& meta, int32_t exitCode);

    /** Reparse all logs and verify the complete frozen plan before exporting
    @param [in] batch Checked absolute batch path, output goes to batch/reports
    */
    static void summarize(const std::string& batch);

    /** Export every field, using N/A for unavailable values
    @param [in] path Checked report destination
    @param [in] rows Array of flat result objects
    */
    static void csv(const std::string& path, const Json::Value& rows);

    /** Calculate a sample median
    @param [in] values Nonempty finite samples, sorted in the private copy
    @return Middle value, or mean of the middle pair
    */
    static double median(std::vector<double> values);

    /** Describe repeated observed ranges on the same metric, including touching endpoints
    @param [in] leftCount Number of measured samples in the first group
    @param [in] leftMin Smallest observed first-group value
    @param [in] leftMax Largest observed first-group value
    @param [in] rightCount Number of measured samples in the comparison group
    @param [in] rightMin Smallest observed comparison value
    @param [in] rightMax Largest observed comparison value
    @return Sample-range label for the export, independent of whether larger values are preferable
    */
    static const char* rangeEvidence(size_t leftCount, double leftMin, double leftMax,
        size_t rightCount, double rightMin, double rightMax);

    /** Derive throughput and separately charged maintenance cost
    @param [in] sample Valid workload row
    @param [in] extraMaintenanceNs Nonnegative maintenance outside the timed loop
    @return Enriched row; embedded maintenance is never added again
    */
    static Json::Value rates(const Json::Value& sample, double extraMaintenanceNs);

private:
    /** Require an exact nonnegative integer within an explicit upper bound
    @param [in] values Parsed evidence object
    @param [in] key Required numeric field
    @param [in] maximum Inclusive upper bound, no greater than 2^53-1
    @return Validated integer; absent, fractional or out-of-range evidence throws
    */
    static double whole(const Json::Value& values, const char* key, double maximum);

    /** Require a finite number with the declared sign
    @param [in] values Evidence fields
    @param [in] key Required field name
    @param [in] positive Whether zero is forbidden
    @return Validated number; malformed evidence throws
    */
    static double number(const Json::Value& values, const char* key, bool positive);

    /** Independently enumerate the retained performance workload contract
    @param [in] suite 35, 38, 40 or extra
    @param [in] reference std::list and mutex comparison without queue pool APIs
    @return Scenario/cycle keys mapped to exact item or operation counts
    */
    static std::map<std::string, double> scenarios(const std::string& suite, bool reference);
};