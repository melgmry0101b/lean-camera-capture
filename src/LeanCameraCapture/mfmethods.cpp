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

// =======================================================================
// ====== Global variable for storing the state of Media Foundation ======
// =======================================================================

static bool g_IsMediaFoundationStarted{ false };

// ======================================
// ====== Media Foundation Methods ======
// ======================================

// --------------------------------------------------------------------
// StartMediaFoundation
// --------------------------------------------------------------------

void StartMediaFoundation() noexcept(false)
{
    if (g_IsMediaFoundationStarted) { return; }

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

    g_IsMediaFoundationStarted = true;
}

// --------------------------------------------------------------------
// StopMediaFoundation
// --------------------------------------------------------------------

void StopMediaFoundation() noexcept(false)
{
    if (!g_IsMediaFoundationStarted) { return; }

    HRESULT hr{ S_OK };

    hr = MFShutdown();
    if (FAILED(hr))
    {
        throw std::system_error{ hr, std::system_category(), "Error occurred during MFShutdown." };
    }

    CoUninitialize();

    g_IsMediaFoundationStarted = false;
}

// --------------------------------------------------------------------
// GetIsMediaFoundationStarted
// --------------------------------------------------------------------

bool GetIsMediaFoundationStarted()
{
    return g_IsMediaFoundationStarted;
}
