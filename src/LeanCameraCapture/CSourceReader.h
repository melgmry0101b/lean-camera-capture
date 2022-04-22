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

            LONG getImageDefaultStride() { return m_lImageDefaultStride; }
            UINT32 getImageWidth() { return m_imageWidth; }
            UINT32 getImageHeight() { return m_imageHeight; }

            // ---
            // --- Destructor
            // ---

            ~CSourceReader();

        private:
            void FreeResources();

            // ---
            // --- Static Methods
            // ---

            static HRESULT GetWidthHeightDefaultStrideForMediaType(
                IMFMediaType *pMediaType,
                LONG *plDefaultStride,
                UINT32 *pWidth,
                UINT32 *pHeight
                );

            /* === Data Members === */
        private:
            long                m_nRefCount;            // Reference count for this COM object.
            CRITICAL_SECTION    m_criticalSection;      // For thread safety.
                                                        //  We should've used std::mutex
                                                        //  but its not supported under C++/CLI
                                                        //  and no need to hop into mental gymnastics to enable it.

            IMFActivate         *m_pDevice;             // Reference for the used capture device
            IMFSourceReader     *m_pSourceReader;       // Reader for samples from the capture device
            IMFTransform        *m_pProcessor;          // Processing the input type into RGB32 output type

            LONG                m_lImageDefaultStride;
            UINT32              m_imageWidth;
            UINT32              m_imageHeight;

            WCHAR               *m_pwszSymbolicLink;
            UINT32              m_cchSymbolicLink;
        };
    }
}
