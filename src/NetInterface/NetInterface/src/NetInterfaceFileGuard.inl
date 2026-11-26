#include <cstring>

inline void NetInterfaceFileGuard::copyFrameBody(uint8_t* output, const uint8_t* input, uint32_t size)
{
    if (size >= 8)
    {
        std::memcpy(output, input, size);
    }
    else if (size >= 4)
    {
        std::memcpy(output, input, 4);
        std::memcpy(output + size - 4, input + size - 4, 4);
    }
    else if (size >= 2)
    {
        std::memcpy(output, input, 2);
        std::memcpy(output + size - 2, input + size - 2, 2);
    }
    else if (size != 0)
    {
        *output = *input;
    }
}