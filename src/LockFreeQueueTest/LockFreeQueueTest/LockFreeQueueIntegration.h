#pragma once
// Read-only consumer of frozen queue and companion-library snapshots
class LockFreeQueueIntegration
{
public:
    /** Queue notification adapter against a frozen real Semaphore source snapshot
    @return Whether every stated invariant passed
    */
    static bool notification();

    /** Compile and instantiate the four library APIs in this translation unit
    @return Whether every stated invariant passed
    */
    static bool fourHeaders();
};