/*-----------------------------------------------------------------*\
 *
 * CameraCaptureDevice.h
 *   LeanCameraCapture
 *     lean-camera-capture
 *
 * MIT - see LICENSE at root directory
 *
 * CREATED: 2022-3-29 08:46 PM
 * AUTHORS: Mohammed Elghamry <elghamry.connect[at]outlook[dot]com>
 *
\*-----------------------------------------------------------------*/

#pragma once

using namespace System::Collections::ObjectModel;

namespace LeanCameraCapture
{
    public ref class CameraCaptureDevice sealed
    {
        /* === Member Functions === */
    public:
        /// <summary>
        /// Get camera capture devices available through the Media Foundation API
        /// </summary>
        /// <returns>Readonly collection of the found devices</returns>
        static ReadOnlyCollection<CameraCaptureDevice ^> ^GetCameraCaptureDevices();

        ~CameraCaptureDevice();
        !CameraCaptureDevice();

    internal:
        /// <summary>
        /// Create new camera capture device
        /// </summary>
        CameraCaptureDevice(IMFActivate *device);

        /// <summary>
        /// Get the underlying IMFActivate
        /// </summary>
        IMFActivate *getUnderlyingIMFActivate() { return m_pDevice; }

        /* === Properties === */
    public:
        /// <summary>
        /// The name of the camera capture device
        /// </summary>
        property System::String ^DeviceName
        {
            System::String ^get() { return m_deviceName; }
        protected:
            void set(System::String ^val) { m_deviceName = val; }
        }

        /* === Data Members === */
    private:
        IMFActivate *m_pDevice{ nullptr };
        System::String ^m_deviceName{ nullptr };
    };
}
