#include "PlayerCallback.h"
#include "LoopPlayerConstants.h"
#include "LoopPlayerUtils.h"

namespace LoopPlayer
{
    PlayerCallback::PlayerCallback()
        : refCount_(1),
          hwnd_(NULL)
    {
    }

    void PlayerCallback::SetWindow(HWND hwnd)
    {
        hwnd_ = hwnd;
    }

    STDMETHODIMP PlayerCallback::QueryInterface(REFIID riid, void** ppv)
    {
        if (!ppv)
        {
            return E_POINTER;
        }

        if (riid == __uuidof(IUnknown) || riid == __uuidof(IMFPMediaPlayerCallback))
        {
            *ppv = static_cast<IMFPMediaPlayerCallback*>(this);
            AddRef();
            return S_OK;
        }

        *ppv = NULL;
        return E_NOINTERFACE;
    }

    STDMETHODIMP_(ULONG) PlayerCallback::AddRef()
    {
        return static_cast<ULONG>(InterlockedIncrement(&refCount_));
    }

    STDMETHODIMP_(ULONG) PlayerCallback::Release()
    {
        LONG value = InterlockedDecrement(&refCount_);
        if (value < 0)
        {
            InterlockedIncrement(&refCount_);
            value = 0;
        }
        return static_cast<ULONG>(value);
    }

    void STDMETHODCALLTYPE PlayerCallback::OnMediaPlayerEvent(MFP_EVENT_HEADER* eventHeader)
    {
        if (!eventHeader)
        {
            return;
        }

        if (FAILED(eventHeader->hrEvent))
        {
            Logf(L"MFPlay callback error: event=%s(%d), hr=0x%08X, state=%s",
                 MediaPlayerEventName(eventHeader->eEventType),
                 static_cast<int>(eventHeader->eEventType),
                 static_cast<unsigned int>(eventHeader->hrEvent),
                 MediaPlayerStateName(eventHeader->eState));
        }

        HWND hwnd = hwnd_;
        if (hwnd)
        {
            PostMessageW(hwnd,
                         WM_PLAYER_EVENT,
                         static_cast<WPARAM>(eventHeader->eEventType),
                         static_cast<LPARAM>(eventHeader->hrEvent));
        }
    }
}