#pragma once

inline bool NetInterfaceTlsOptions::setHandshakeTimeout(uint32_t value)
{
    if ((value != 0 && value < 100) || value > 60000)
    {
        return false;
    }
    handshakeTimeoutMs = value;
    return true;
}

inline uint32_t NetInterfaceTlsOptions::effectiveHandshakeTimeout() const
{
    return handshakeTimeoutMs == 0 ? 10000 : handshakeTimeoutMs;
}
