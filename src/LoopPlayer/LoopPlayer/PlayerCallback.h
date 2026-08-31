#pragma once

#include "LoopPlayerPlatform.h"

namespace LoopPlayer
{
    class PlayerCallback : public IMFPMediaPlayerCallback
    {
    public:
        PlayerCallback();
        void SetWindow(HWND hwnd);
        STDMETHODIMP QueryInterface(REFIID riid, void** ppv);
        STDMETHODIMP_(ULONG) AddRef();
        STDMETHODIMP_(ULONG) Release();
        void STDMETHODCALLTYPE OnMediaPlayerEvent(MFP_EVENT_HEADER* eventHeader);

    private:
        volatile LONG refCount_;
        HWND hwnd_;
    };
}