/*-----------------------------------------------------------------*\
 *
 * CameraCaptureDevice.cpp
 *   LeanCameraCapture
 *     lean-camera-capture
 *
 * MIT - see LICENSE at root directory
 *
 * CREATED: 2022-3-29 08:46 PM
 * AUTHORS: Mohammed Elghamry <elghamry.connect[at]outlook[dot]com>
 *
\*-----------------------------------------------------------------*/

#include "leancamercapture.h"

#include "CameraCaptureDevice.h"

using namespace System::Collections::ObjectModel;
using namespace System::Collections::Generic;
using namespace LeanCameraCapture;

// ============================
// ====== Static Methods ======
// ============================

// --------------------------------------------------------------------
// GetCameraCaptureDevices
// --------------------------------------------------------------------

ReadOnlyCollection<CameraCaptureDevice ^> ^CameraCaptureDevice::GetCameraCaptureDevices()
{
    // Check if Media Foundation is running
    if (!CameraCaptureManager::IsStarted)
    {
        throw gcnew CameraCaptureException("CameraCaptureManager has not been started.");
    }

    HRESULT hr{ S_OK };
    System::String ^errorMsg{ nullptr };

    IMFAttributes *pAttributes{ nullptr };

    IMFActivate **ppDevices{ nullptr };
    UINT32 devicesCount{ 0 };

    // Initialize an attribute store for being used for the devices enumeration parameters
    hr = MFCreateAttributes(&pAttributes, 1);
    if (FAILED(hr))
    {
        errorMsg = "Error occurred during MFCreateAttributes.";
        goto done;
    }

    // Ask for video capture devices
    hr = pAttributes->SetGUID(
        MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE,
        MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE_VIDCAP_GUID
    );
    if (FAILED(hr))
    {
        errorMsg = "Error occurred during IMFAttributes::SetGUID().";
        goto done;
    }

    // Enumerate the requested devices
    hr = MFEnumDeviceSources(pAttributes, &ppDevices, &devicesCount);
    if (FAILED(hr))
    {
        errorMsg = "Error occurred during MFEnumDeviceSources.";
        goto done;
    }

    // Create the managed list for the devices and return it
    List<CameraCaptureDevice ^> ^cameraCaptureDevices = gcnew List<CameraCaptureDevice ^>();
    for (UINT32 i = 0; i < devicesCount; i++)
    {
        try
        {
            cameraCaptureDevices->Add(gcnew CameraCaptureDevice(ppDevices[i]));
        }
        catch (CameraCaptureException ^ex)
        {
            hr = ex->HResult;
            errorMsg = ex->Message;
            goto done;
        }
    }

done:
    SafeRelease(&pAttributes);

    for (UINT32 i = 0; i < devicesCount; i++)
    {
        SafeRelease(&ppDevices[i]);
    }

    CoTaskMemFree(ppDevices);

    if (FAILED(hr))
    {
        throw gcnew CameraCaptureException(hr, errorMsg);
    }

    return cameraCaptureDevices->AsReadOnly();
}

// =========================
// ====== Constructor ======
// =========================

CameraCaptureDevice::CameraCaptureDevice(IMFActivate *device)
{
    assert(device != nullptr);
    assert(CameraCaptureManager::IsStarted == true);

    HRESULT hr{ S_OK };

    // Get Device's symbolic link
    hr = device->GetAllocatedString(
        MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE_VIDCAP_SYMBOLIC_LINK,
        pin_ptr<WCHAR *>(&m_pwszDeviceSymbolicLink),
        pin_ptr<UINT32>(&m_cchDeviceSymbolicLink)
    );
    if (FAILED(hr))
    {
        throw gcnew CameraCaptureException(hr, "Error occurred during retrieving device symbolic link.");
    }

    hr = device->GetAllocatedString(
        MF_DEVSOURCE_ATTRIBUTE_FRIENDLY_NAME,
        pin_ptr<WCHAR *>(&m_pwszDeviceFriendlyName),
        pin_ptr<UINT32>(&m_cchDeviceFriendlyName)
    );
    if (FAILED(hr))
    {
        throw gcnew CameraCaptureException(hr, "Error occurred during retrieving device friendly name.");
    }
}

// ========================
// ====== Destructor ======
// ========================

CameraCaptureDevice::~CameraCaptureDevice()
{
    // Release Managed Resources

    // Call the finalizer
    this->!CameraCaptureDevice();
}

// =======================
// ====== Finalizer ======
// =======================

CameraCaptureDevice::!CameraCaptureDevice()
{
    // Release Unmanaged <Native> Resources

    CoTaskMemFree(m_pwszDeviceFriendlyName);
    m_pwszDeviceFriendlyName = nullptr;

    CoTaskMemFree(m_pwszDeviceSymbolicLink);
    m_pwszDeviceSymbolicLink = nullptr;
}
