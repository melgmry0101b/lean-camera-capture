/*-----------------------------------------------------------------*\
 *
 * mfmethods.cpp
 *   LeanCameraCapture
 *     lean-camera-capture
 *
 * MIT - see LICENSE at root directory
 *
 * CREATED: 2022-3-30 10:10 PM
 * AUTHORS: Mohammed Elghamry <elghamry.connect[at]outlook[dot]com>
 *
\*-----------------------------------------------------------------*/

#include "leancamercapture.h"

#include "mfmethods.h"

void StartMediaFoundation() noexcept(false)
{
    HRESULT hr{ S_OK };

    hr = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
    if (FAILED(hr))
    {
        throw std::system_error{ hr, std::system_category(), "Error occurred during CoInitializeEx." };
    }

    hr = MFStartup(MF_VERSION);
    if (FAILED(hr))
    {
        throw std::system_error{ hr, std::system_category(), "Error occurred during MFStartup." };
    }
}

void StopMediaFoundation() noexcept(false)
{
    HRESULT hr{ S_OK };

    hr = MFShutdown();
    if (FAILED(hr))
    {
        throw std::system_error{ hr, std::system_category(), "Error occurred during MFShutdown." };
    }

    CoUninitialize();
}
