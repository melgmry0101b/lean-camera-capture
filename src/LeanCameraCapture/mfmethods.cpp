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

#pragma managed(push, off)

// =======================================================================
// ====== Global variable for storing the state of Media Foundation ======
// =======================================================================

static bool g_IsMediaFoundationStarted{ false };
static HWND g_hwndMain{ nullptr };

// ======================================
// ====== Media Foundation Methods ======
// ======================================

// --------------------------------------------------------------------
// StartMediaFoundation
//
// This method takes a window handler for the main window where
//  the library is going to attach its window-based handlers
//  like device change notification handler.
// --------------------------------------------------------------------

void StartMediaFoundation(HWND hwndMain) noexcept(false)
{
    assert(hwndMain != nullptr);

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

    // Register capture device change notification for the main window
    RegisterCaptureDeviceChangeNotificationForHwnd(hwndMain);
    g_hwndMain = hwndMain;

    g_IsMediaFoundationStarted = true;
}

// --------------------------------------------------------------------
// StopMediaFoundation
// --------------------------------------------------------------------

void StopMediaFoundation() noexcept(false)
{
    if (!g_IsMediaFoundationStarted) { return; }

    HRESULT hr{ S_OK };

    // Unregister capture device change notification for main window
    if (g_hwndMain)
    {
        UnregisterRegisteredCaptureDeviceChangeNotification(g_hwndMain);
        g_hwndMain = nullptr;
    }

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

// --------------------------------------------------------------------
// GetMainHwnd
// --------------------------------------------------------------------

HWND GetMainHwnd()
{
    return g_hwndMain;
}

#pragma managed(pop)
