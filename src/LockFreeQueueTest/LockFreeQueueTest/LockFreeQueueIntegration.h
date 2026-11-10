#pragma once
class LockFreeQueueIntegration
{
public:
    /** Queue notification adapter against a frozen real Semaphore source snapshot */
    static bool notification();
    /** Compile and instantiate the four library APIs in this translation unit */
    static bool fourHeaders();
};