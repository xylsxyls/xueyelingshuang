#pragma once
#include "LockFreeQueueTestTool.h"

/** Exact, reversible source instrumentation. Only test-owned temporary copies are writable. */
class LockFreeQueueTestWhitebox
{
public:
    /** Remove test conditionals while retaining the production branch
    @param [in] text LF source text with test-only conditionals
    @return Production text; unclosed conditionals throw
    */
    static std::string strip(const std::string& text);

    /** Validate all patches before writing any generated source
    @param [in] tool Checkout paths and configuration
    @param [in] source Read-only source directory
    @param [in] output Strict descendant of the private QueueTest temporary root
    @return SHA256 manifest; nonreversible, stale or missing contexts throw
    */
    static Json::Value prepare(const LockFreeQueueTestTool& tool, const std::string& source, const std::string& output);
};