/*-----------------------------------------------------------------*\
 *
 * CSourceReader.h
 *   LeanCameraCapture
 *     lean-camera-capture
 *
 * MIT - see LICENSE at root directory
 *
 * CREATED: 2022-4-8 10:30 PM
 * AUTHORS: Mohammed Elghamry <elghamry.connect[at]outlook[dot]com>
 *
\*-----------------------------------------------------------------*/

#pragma once

#include "leancamercapture.h"

namespace LeanCameraCapture
{
    namespace Native
    {
        // ========================================
        // ====== Function Pointers typedefs ======
        // ========================================

        /// <summary>
        /// Handler definition for OnReadSample success callback
        ///
        /// pbBuffer        => BYTE* points to the buffer
        /// widthInPixels   => UINT32 tells the buffer width in pixels
        /// heightInPixels  => UINT32 tells the buffer height in pixels
        /// bytesPerPixel   => UINT32 tells how many bytes per pixel
        /// </summary>
        typedef void (*READ_SAMPLE_SUCCESS_HANDLER)(
            const BYTE *pbBuffer,
            UINT32 widthInPixels,
            UINT32 heightInPixels,
            UINT32 bytesPerPixel)
            ;

        /// <summary>
        /// Handler definition for OnReadSample fail callback
        ///
        /// hr          => const HRESULT for the underlying WinAPI error
        /// errorString => const std::string& describes the error occurred
        /// </summary>
        typedef void (*READ_SAMPLE_FAIL_HANDLER)(
            const HRESULT hr,
            const std::string& errorString
            );

        // ============================================
        // ====== CSourceReader Class Definition ======
        // ============================================

        class CSourceReader : public IMFSourceReaderCallback
        {
            /* === Member Functions === */
        public:
            // ---
            // --- IUnknown methods
            // ---

            STDMETHODIMP QueryInterface(REFIID iid, void **ppv);
            STDMETHODIMP_(ULONG) AddRef();
            STDMETHODIMP_(ULONG) Release();

            // ---
            // --- IMFSourceReaderCallback methods
            // ---

            STDMETHODIMP OnReadSample(
                HRESULT hrStatus,
                DWORD dwStreamIndex,
                DWORD dwStreamFlags,
                LONGLONG llTimestamp,
                IMFSample *pSample
            );

            STDMETHODIMP OnEvent(DWORD, IMFMediaEvent *) { return S_OK; }

            STDMETHODIMP OnFlush(DWORD) { return S_OK; }

            // ---
            // --- Constructor
            // ---

            CSourceReader();

            // ---
            // --- CSourceReader methods
            // ---

            void InitializeForDevice(IMFActivate *pActivate) noexcept(false);
            void ReadFrame() noexcept(false);

            void SetReadFrameSuccessCallback(READ_SAMPLE_SUCCESS_HANDLER pCallback);
            void SetReadFrameFailCallback(READ_SAMPLE_FAIL_HANDLER pCallback);

            UINT32 getFrameWidth() const { return m_frameWidth; }
            UINT32 getFrameHeight() const { return m_frameHeight; }

            // ---
            // --- Destructor
            // ---

            ~CSourceReader();

        private:
            void FreeResources();

            void ProcessorProcessOutput(
                DWORD dwOutputStreamID,
                IMFSample **ppOutputSample
                ) noexcept(false);

            void ProcessorProcessSample(
                DWORD dwStreamID,
                IMFSample *pInputSample,
                IMFSample **ppOutputSample
                ) noexcept(false);

            // ---
            // --- Static Methods
            // ---

            static void GetWidthHeightDefaultStrideForMediaType(
                IMFMediaType *pMediaType,
                LONG *plDefaultStride,
                UINT32 *pWidth,
                UINT32 *pHeight
                ) noexcept(false);

            static void SetVideoProcessorOutputForInputMediaType(
                IMFTransform *pProcessor,
                IMFMediaType *pInputMediaType,
                const GUID guidOutputVideoSubtype
                ) noexcept(false);

            /* === Data Members === */
        private:
            long                    m_nRefCount;            // Reference count for this COM object.
            CRITICAL_SECTION        m_criticalSection;      // For thread safety.
                                                            //  We should've used std::mutex
                                                            //  but its not supported under C++/CLI
                                                            //  and no need to hop into mental gymnastics to enable it.

            IMFActivate             *m_pDevice;             // Reference for the used capture device
            IMFSourceReader         *m_pSourceReader;       // Reader for samples from the capture device
            IMFTransform            *m_pProcessor;          // Processing the input type into RGB32 output type

            LONG                    m_lSrcDefaultStride;

            UINT32                  m_frameWidth;
            UINT32                  m_frameHeight;

            std::unique_ptr<BYTE*>  m_frameBuffer;

            WCHAR                   *m_pwszSymbolicLink;
            UINT32                  m_cchSymbolicLink;

            READ_SAMPLE_SUCCESS_HANDLER m_pReadSampleSuccessCallback;
            READ_SAMPLE_FAIL_HANDLER    m_pReadSampleFailCallback;
        };
    }
}
