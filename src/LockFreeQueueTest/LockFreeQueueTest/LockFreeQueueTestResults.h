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

    /** Derive throughput and separately charged maintenance cost
    @param [in] sample Valid workload row
    @param [in] extraMaintenanceNs Nonnegative maintenance outside the timed loop
    @return Enriched row; embedded maintenance is never added again
    */
    static Json::Value rates(const Json::Value& sample, double extraMaintenanceNs);
};