/*-----------------------------------------------------------------*\
 *
 * CBufferLock.hpp
 *   LeanCameraCapture
 *     lean-camera-capture
 *
 * MIT - see LICENSE at root directory
 *
 * CREATED: 2022-4-9 01:20 AM
 * AUTHORS: Mohammed Elghamry <elghamry.connect[at]outlook[dot]com>
 *
\*-----------------------------------------------------------------*/

#pragma once

#include "leancamercapture.h"

#pragma managed(push, off)

namespace LeanCameraCapture
{
    namespace Native
    {
        class CBufferLock
        {
            /* === Member Functions === */
        public:
            /// <summary>
            /// Create a IMFMediaBuffer locking helper
            /// </summary>
            CBufferLock(IMFMediaBuffer *pBuffer) :
                m_p2DBuffer{ nullptr },
                m_isLocked{ false }
            {
                assert(pBuffer != nullptr);

                // Add reference to the COM object
                m_pBuffer = pBuffer;
                pBuffer->AddRef();

                // Query for 2D buffer if available
                (void)m_pBuffer->QueryInterface(IID_PPV_ARGS(&m_p2DBuffer));
            }

            /// <summary>
            /// Locks the buffer and outs its contents
            /// </summary>
            HRESULT LockBuffer(
                LONG    lDefaultStride,     // Stride is fundamentally the width of the viewable image in addition to padding
                DWORD   dwHeightInPixels,   // The height of the image that the device reports
                BYTE    **ppbScanLine0,     // Receiving pointer to the first scanline -row- of the image
                LONG    *plStride           // Receiving the actual stride of the image
                )
            {
                HRESULT hr{ S_OK };

                // If the type of the buffer is IMF2DBuffer use its methods,
                //  if not use the methods of the raw buffer
                if (m_p2DBuffer)
                {
                    hr = m_p2DBuffer->Lock2D(ppbScanLine0, plStride);
                }
                else
                {
                    BYTE *pData{ nullptr };

                    hr = m_pBuffer->Lock(&pData, nullptr, nullptr);
                    if (SUCCEEDED(hr))
                    {
                        *plStride = lDefaultStride;
                        if (lDefaultStride < 0)
                        {
                            // if the stride is negative, get the last row of the image as the first scanline,
                            //  this if the image is flipped upside down
                            *ppbScanLine0 = pData + (std::abs(lDefaultStride) * (dwHeightInPixels - 1));
                        }
                        else
                        {
                            *ppbScanLine0 = pData;
                        }
                    }
                }

                m_isLocked = (SUCCEEDED(hr));

                return hr;
            }

            /// <summary>
            /// Unlock the buffer
            /// </summary>
            void UnlockBuffer()
            {
                if (m_isLocked)
                {
                    if (m_p2DBuffer)
                    {
                        (void)m_p2DBuffer->Unlock2D();
                    }
                    else
                    {
                        (void)m_pBuffer->Unlock();
                    }
                    m_isLocked = false;
                }
            }

            ~CBufferLock()
            {
                UnlockBuffer();
                SafeRelease(&m_pBuffer);
                SafeRelease(&m_p2DBuffer);
            }

            /* === Data Members === */
        private:
            IMFMediaBuffer *m_pBuffer;
            IMF2DBuffer *m_p2DBuffer;

            bool m_isLocked;
        };
    }
}

#pragma managed(pop)
