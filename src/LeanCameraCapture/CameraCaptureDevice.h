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
        /// [Internal] Create new camera capture device
        /// </summary>
        CameraCaptureDevice(IMFActivate *device);

        /// <summary>
        /// [Internal] Gets native WCHAR pointer of the symbolic link.
        /// </summary>
        WCHAR *GetNativeDeviceSymbolicLink() { return m_pwszDeviceSymbolicLink; }

        /* === Properties === */
    public:
        /// <summary>
        /// Gets friendly name of the capture device.
        /// </summary>
        property System::String ^DeviceName
        {
            System::String ^get() { return gcnew System::String(m_pwszDeviceFriendlyName); }
        }

        /// <summary>
        /// Gets device's symbolic link.
        /// </summary>
        property System::String ^DeviceSymbolicLink
        {
            System::String ^get() { return gcnew System::String(m_pwszDeviceSymbolicLink); }
        }

        /* === Data Members === */
    private:
        WCHAR       *m_pwszDeviceFriendlyName{ nullptr };
        UINT32      m_cchDeviceFriendlyName{ 0 };

        WCHAR       *m_pwszDeviceSymbolicLink{ nullptr };
        UINT32      m_cchDeviceSymbolicLink{ 0 };
    };
}
