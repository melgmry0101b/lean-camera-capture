/*-----------------------------------------------------------------*\
 *
 * CameraCaptureReader.h
 *   LeanCameraCapture
 *     lean-camera-capture
 *
 * MIT - see LICENSE at root directory
 *
 * CREATED: 2022-4-8 10:27 PM
 * AUTHORS: Mohammed Elghamry <elghamry.connect[at]outlook[dot]com>
 *
\*-----------------------------------------------------------------*/

#pragma once

#include "leancamercapture.h"

using namespace System::Collections::Generic;

namespace LeanCameraCapture
{
    public ref class CameraCaptureReader sealed
    {
        /* === Member Functions === */
    public:
        /// <summary>
        /// Create a new reader for the specified device.
        /// </summary>
        /// <param name="device">Camera capture device to be read from.</param>
        CameraCaptureReader(CameraCaptureDevice ^device);

        /// <summary>
        /// Open reader.
        /// </summary>
        void Open();

        /// <summary>
        /// Close reader.
        /// </summary>
        void Close();

        /// <summary>
        /// Reopen reader, same as calling Close() then Open().
        /// </summary>
        void Reopen();

        /// <summary>
        /// Read next available sample from the device.
        /// </summary>
        void ReadSample();

        /// <summary>
        /// Read sample succeeded event.
        /// </summary>
        event System::EventHandler<ReadSampleSucceededEventArgs ^> ^ReadSampleSucceeded;

        /// <summary>
        /// Read sample failed event
        /// </summary>
        event System::EventHandler<ReadSampleFailedEventArgs ^> ^ReadSampleFailed;

        ~CameraCaptureReader();
        !CameraCaptureReader();

    private:
        // NOTE: On* methods are usually protected and virtual, but as this class
        //  is sealed, they are declared as private.

        void OnReadSampleSucceeded(System::Object ^sender, ReadSampleSucceededEventArgs ^e);
        void OnReadSampleFailed(System::Object ^sender, ReadSampleFailedEventArgs ^e);

        void ReadFrameSuccessNativeHandler(
            const BYTE *pbBuffer,
            UINT32 widthInPixels,
            UINT32 heightInPixels,
            UINT32 bytesPerPixel
        );
        void ReadFrameFailNativeHandler(
            const HRESULT hr,
            const std::string &errorString
        );

        /* === Delegates === */
    private:
        delegate void ReadFrameSuccessNativeCallback(
            const BYTE *pbBuffer,
            UINT32 widthInPixels,
            UINT32 heightInPixels,
            UINT32 bytesPerPixel
        );
        delegate void ReadFrameFailNativeCallback(
            const HRESULT hr,
            const std::string &errorString
        );

        /* === Properties === */
    public:
        /// <summary>
        /// Gets the capture device used for the reader.
        /// </summary>
        property CameraCaptureDevice ^Device
        {
            CameraCaptureDevice ^get() { return m_device; }
        }

        /// <summary>
        /// Gets if the reader is open.
        /// </summary>
        property System::Boolean IsOpen
        {
            System::Boolean get() { return m_pCSourceReader != nullptr; }
        }

        /* === Data Members === */
    private:
        CameraCaptureDevice     ^m_device;  // Reference to the device used for the reader.

        System::Object          ^m_lock;    // Lock object for synchronization.

        array<System::Byte>     ^m_buffer;  // Here we store buffer to avoid multiple invocations of GC.

        // On opening the managed reader, a new native reader is allocated and initialized,
        //  and on close, the native reader is released.
        // We don't use unique_ptr here as this is a COM object that has to be used
        //  with CComPtr or track it ourselves with `SafeRelease`
        Native::CSourceReader               *m_pCSourceReader; // Native CSourceReader.

        // Delegates to the underlying native CSourceReader.
        // We save the delegates here as member in the class to avoid them being GCed,
        //  as the CLR won't track the delegate in the native outer space.
        ReadFrameSuccessNativeCallback      ^m_CSourceReaderReadFrameSuccessHandler;
        ReadFrameFailNativeCallback         ^m_CSourceReaderReadFrameFailHandler;
    };
}
