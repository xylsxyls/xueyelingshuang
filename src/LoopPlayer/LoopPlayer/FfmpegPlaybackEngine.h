#pragma once
#include "LoopPlayerPlatform.h"
#include "PlaybackEngineTypes.h"

#include <atomic>
#include <string>
#include <vector>

class FFmpegCppPlaybackReader;
struct FFmpegCppPlaybackFrame;
struct FFmpegCppPlaybackAudioFrame;
struct FFmpegCppPlaybackVideoFrame;

namespace LoopPlayer
{
    /**
     * FFmpegCpp based playback engine with independent video, audio, and preview readers.
     */
    class FfmpegPlaybackEngine
    {
    public:
        /**
         * Build an empty playback engine.
         */
        FfmpegPlaybackEngine();

        /**
         * Stop playback and release all native resources.
         */
        ~FfmpegPlaybackEngine();

        /**
         * Open a media file and start playback worker threads.
         * @param [in] param Playback initialization parameters.
         * @return S_OK if the engine was initialized.
         */
        HRESULT init(const PlaybackEngineInitParam& param);

        /**
         * Close the current media file and release playback resources.
         */
        void uninit();

        /**
         * Start or resume playback from the current position.
         * @return S_OK if playback was started.
         */
        HRESULT play();

        /**
         * Pause playback and keep the current clock position.
         * @return S_OK if playback was paused.
         */
        HRESULT pause();

        /**
         * Stop playback and seek to the beginning.
         * @return S_OK if playback was stopped.
         */
        HRESULT stop();

        /**
         * Seek to a playback position.
         * @param [in] position Target position in 100ns units.
         * @param [in] resumeIfPreviousPlaying Resume when the previous state was playing.
         * @param [in] previewMaxReadCount Maximum preview frames decoded after seek. Zero uses the default.
         * @return S_OK if seeking succeeded.
         */
        HRESULT seek(REFERENCE_TIME position, bool resumeIfPreviousPlaying = true, size_t previewMaxReadCount = 0);

        /**
         * Decode and publish a preview video frame without moving the main playback readers.
         * @param [in] position Target position in 100ns units.
         * @param [in] previewMaxReadCount Maximum preview frames decoded after seek. Zero uses the default.
         * @return S_OK if a preview frame was published.
         */
        HRESULT previewVideoFrame(REFERENCE_TIME position, size_t previewMaxReadCount = 0);

        /**
         * Set playback rate.
         * @param [in] rate New playback rate.
         * @return S_OK if the rate was accepted.
         */
        HRESULT setRate(double rate);

        /**
         * Read the current playback position.
         * @param [out] position Current position in 100ns units.
         * @return true if a position was written.
         */
        bool getPosition(REFERENCE_TIME& position);

        /**
         * Read a snapshot of the playback engine state.
         * @param [out] snapshot Snapshot output.
         */
        void snapshot(PlaybackEngineSnapshot& snapshot);

        /**
         * Check whether media is open.
         * @return true if media is open.
         */
        bool isOpen() const;

        /**
         * Read media duration.
         * @return Duration in 100ns units.
         */
        REFERENCE_TIME duration() const;

        /**
         * Read video frame duration.
         * @return Frame duration in 100ns units.
         */
        REFERENCE_TIME frameDuration() const;

        /**
         * Read native video size.
         * @param [out] width Native video width.
         * @param [out] height Native video height.
         * @return true if video size is available.
         */
        bool getNativeVideoSize(int& width, int& height) const;

        /**
         * Read current playback state.
         * @return Current playback state.
         */
        PlaybackEngineState state();

        /**
         * Read the last error message.
         * @return Last error message.
         */
        std::wstring lastError() const;

        /**
         * Paint the latest video frame.
         * @param [in] hdc Target device context.
         * @param [in] rect Target rectangle.
         */
        void paintVideo(HDC hdc, const RECT& rect);

    private:
        /**
         * Copying is disabled because the engine owns native resources.
         * @param [in] other Another engine.
         */
        FfmpegPlaybackEngine(const FfmpegPlaybackEngine& other);

        /**
         * Assignment is disabled because the engine owns native resources.
         * @param [in] other Another engine.
         * @return Current engine.
         */
        FfmpegPlaybackEngine& operator=(const FfmpegPlaybackEngine& other);

        /**
         * Open one playback reader.
         * @param [in] filePath Media file path.
         * @param [in] decodeVideo Whether to decode video.
         * @param [in] decodeAudio Whether to decode audio.
         * @param [out] reader Reader output.
         * @param [in] readerName Text used in logs.
         * @return S_OK if the reader was opened.
         */
        HRESULT openReader(const std::wstring& filePath, bool decodeVideo, bool decodeAudio, FFmpegCppPlaybackReader** reader, const wchar_t* readerName);

        /**
         * Open all FFmpegCpp readers needed by playback.
         * @param [in] filePath Media file path.
         * @return S_OK if at least one playback stream was opened.
         */
        HRESULT openReaders(const std::wstring& filePath);

        /**
         * Open the waveOut audio device.
         * @return S_OK if the device was opened or audio is unavailable.
         */
        HRESULT openWaveOut();

        /**
         * Start video and audio worker threads.
         * @return S_OK if all required threads were created.
         */
        HRESULT startThreads();

        /**
         * Stop video and audio worker threads.
         */
        void stopThreads();

        /**
         * Video playback thread entry point.
         * @param [in] param Playback engine pointer.
         * @return Thread exit code.
         */
        static DWORD WINAPI VideoThreadProc(LPVOID param);

        /**
         * Audio playback thread entry point.
         * @param [in] param Playback engine pointer.
         * @return Thread exit code.
         */
        static DWORD WINAPI AudioThreadProc(LPVOID param);

        /**
         * Video playback thread loop.
         */
        void videoThreadMain();

        /**
         * Audio playback thread loop.
         */
        void audioThreadMain();

        /**
         * Wait until playback can continue.
         * @return true if playback can continue.
         */
        bool waitForPlayingState();

        /**
         * Read the next decoded video frame.
         * @param [out] frame Decoded frame output.
         * @return FFmpegCppPlaybackReadResult value.
         */
        int readNextVideoFrame(FFmpegCppPlaybackFrame& frame);

        /**
         * Read the next decoded audio frame.
         * @param [out] frame Decoded frame output.
         * @return FFmpegCppPlaybackReadResult value.
         */
        int readNextAudioFrame(FFmpegCppPlaybackFrame& frame);

        /**
         * Wait until a video frame should be displayed.
         * @param [in] frameTime Video frame timestamp.
         * @param [in] frameDuration Video frame duration.
         * @param [in] seekSerial Current seek serial.
         * @return true if the frame should be displayed.
         */
        bool waitForVideoClock(REFERENCE_TIME frameTime, REFERENCE_TIME frameDuration, DWORD seekSerial);

        /**
         * Keep decoded audio close to the playback clock.
         * @param [in] frameTime Audio frame timestamp.
         * @param [in] frameDuration Audio frame duration.
         * @param [in] seekSerial Current seek serial.
         * @return true if the audio frame should be queued.
         */
        bool waitForAudioLead(REFERENCE_TIME frameTime, REFERENCE_TIME frameDuration, DWORD seekSerial);

        /**
         * Publish a decoded FFmpegCpp video frame to the GDI frame cache.
         * @param [in] frame Decoded video frame.
         */
        void publishVideoFrame(const FFmpegCppPlaybackVideoFrame& frame);

        /**
         * Queue a decoded FFmpegCpp audio frame to waveOut.
         * @param [in] frame Decoded audio frame.
         * @param [in] seekSerial Current seek serial.
         * @return S_OK if the frame was queued.
         */
        HRESULT queueAudioFrame(const FFmpegCppPlaybackAudioFrame& frame, DWORD seekSerial);

        /**
         * Find a reusable waveOut buffer.
         * @param [in] seekSerial Current seek serial.
         * @return Buffer index or -1 when no buffer is available.
         */
        int findReusableAudioBuffer(DWORD seekSerial);

        /**
         * Unprepare one waveOut buffer.
         * @param [in] index Buffer index.
         */
        void unprepareAudioBuffer(size_t index);

        /**
         * Reset all queued waveOut buffers.
         */
        void resetWaveBuffers();

        /**
         * Close the waveOut device.
         */
        void closeWaveOut();

        /**
         * Reset playback clock while the state lock is held.
         * @param [in] position Target position.
         */
        void resetClockLocked(REFERENCE_TIME position);

        /**
         * Calculate playback position while the state lock is held.
         * @return Current playback position.
         */
        REFERENCE_TIME calculatePositionLocked() const;

        /**
         * Change playback state and last HRESULT while the state lock is held.
         * @param [in] stateValue New state.
         * @param [in] result Last HRESULT.
         */
        void setStateLocked(PlaybackEngineState stateValue, HRESULT result);

        /**
         * Post a playback event to the UI window.
         * @param [in] eventType Playback event type.
         * @param [in] result Event HRESULT.
         */
        void postEvent(PlaybackEngineEvent eventType, HRESULT result);

        /**
         * Mark one or more playback streams as ended.
         * @param [in] videoEnded Whether the video stream reached EOF.
         * @param [in] audioEnded Whether the audio stream reached EOF.
         */
        void markStreamEnded(bool videoEnded, bool audioEnded);

        /**
         * Read the current seek serial.
         * @return Current seek serial.
         */
        DWORD readSeekSerial();

        /**
         * Check whether worker threads should exit.
         * @return true if threads should exit.
         */
        bool shouldExit() const;

        /**
         * Copy an FFmpegCpp reader error into the engine error text.
         * @param [in] reader Reader that reported the error.
         * @param [in] prefix Error prefix.
         */
        void setReaderError(const FFmpegCppPlaybackReader* reader, const wchar_t* prefix);

    private:
        CRITICAL_SECTION m_stateLock;
        CRITICAL_SECTION m_videoDecodeLock;
        CRITICAL_SECTION m_audioDecodeLock;
        CRITICAL_SECTION m_previewLock;
        CRITICAL_SECTION m_frameLock;
        CRITICAL_SECTION m_waveLock;
        std::wstring m_filePath;
        std::wstring m_lastError;
        FFmpegCppPlaybackReader* m_videoReader;
        FFmpegCppPlaybackReader* m_audioReader;
        FFmpegCppPlaybackReader* m_previewReader;
        HWND m_videoWindow;
        HWND m_eventWindow;
        UINT m_eventMessage;
        HANDLE m_videoThread;
        HANDLE m_audioThread;
        HANDLE m_wakeupEvent;
        HANDLE m_exitEvent;
        HWAVEOUT m_waveOut;
        WAVEFORMATEX m_waveFormat;
        std::vector<PlaybackAudioBuffer> m_audioBuffers;
        size_t m_nextAudioBuffer;
        PlaybackVideoFrame m_currentFrame;
        BITMAPINFO m_videoBitmapInfo;
        bool m_isInit;
        std::atomic<bool> m_isExit;
        bool m_videoAvailable;
        bool m_audioAvailable;
        bool m_videoEnded;
        bool m_audioEnded;
        PlaybackEngineState m_state;
        HRESULT m_lastResult;
        DWORD m_seekSerial;
        REFERENCE_TIME m_duration;
        REFERENCE_TIME m_clockBasePosition;
        REFERENCE_TIME m_frameDuration;
        double m_playbackRate;
        LARGE_INTEGER m_counterFrequency;
        LARGE_INTEGER m_clockBaseCounter;
        int m_videoWidth;
        int m_videoHeight;
        DWORD m_videoFrameLogCount;
        DWORD m_audioFrameLogCount;
        DWORD m_dropFrameLogCount;
        REFERENCE_TIME m_lastPublishedVideoTime;
    };
}