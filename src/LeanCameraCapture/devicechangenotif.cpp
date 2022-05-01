/*-----------------------------------------------------------------*\
 *
 * devicechangenotif.cpp
 *   LeanCameraCapture
 *     lean-camera-capture
 *
 * MIT - see LICENSE at root directory
 *
 * CREATED: 2022-4-28 11:52 AM
 * AUTHORS: Mohammed Elghamry <elghamry.connect[at]outlook[dot]com>
 *
\*-----------------------------------------------------------------*/

#include "leancamercapture.h"

#include "devicechangenotif.h"

// =====================
// ====== Globals ======
// =====================

// The registered notification pointer
static HDEVNOTIFY g_HDevNofity{ nullptr };

// Map for the devices' symbolic link and handlers
static std::multimap<const WCHAR *, CAPTURE_DEVICE_CAHNGE_NOTIF_HANDLER> g_mapHandlers{};

// The original WindowProc before subclassing
static WNDPROC g_wndprocOriginal{ nullptr };

// ========================================
// ====== WindowProc For Subclassing ======
// ========================================

// --------------------------------------------------------------------
// DeviceChangeNotificationWindowProc
// --------------------------------------------------------------------

static LRESULT CALLBACK DeviceChangeNotificationWindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
    case WM_DEVICECHANGE:
        break;
    }

    return CallWindowProc(g_wndprocOriginal, hwnd, uMsg, wParam, lParam);
}

// =======================
// ====== Functions ======
// =======================

// --------------------------------------------------------------------
// RegisterCaptureDeviceChangeNotificationForHwnd
// --------------------------------------------------------------------

void RegisterCaptureDeviceChangeNotificationForHwnd(HWND hwnd) noexcept(false)
{
    if (g_HDevNofity)
    {
        throw std::logic_error{ "Capture device change notification is already registered." };
    }

    // Register the window for receiving the device change notification messages
    DEV_BROADCAST_DEVICEINTERFACE di{ 0 };
    di.dbcc_size = sizeof(di);
    di.dbcc_devicetype = DBT_DEVTYP_DEVICEINTERFACE;
    di.dbcc_classguid = KSCATEGORY_CAPTURE;

    g_HDevNofity = RegisterDeviceNotification(hwnd, &di, DEVICE_NOTIFY_WINDOW_HANDLE);
    if (!g_HDevNofity)
    {
        throw std::system_error{ E_FAIL, std::system_category(), "Couldn't register capture device change notification." };
    }

    // Subclass the window passed to the argument for being used as the receiver for the notification
    SetLastError(0);
    g_wndprocOriginal = reinterpret_cast<WNDPROC>(
        SetWindowLongPtr(
            hwnd,
            GWLP_WNDPROC,
#ifdef _WIN64
            reinterpret_cast<LONG_PTR>(&DeviceChangeNotificationWindowProc)
#else
            reinterpret_cast<LONG>(&DeviceChangeNotificationWindowProc)
#endif
        ));
    if (!g_wndprocOriginal && GetLastError() != ERROR_SUCCESS)
    {
        UnregisterDeviceNotification(g_HDevNofity);
        throw std::system_error{ E_FAIL, std::system_category(), "Couldn't subclass the passed window." };
    }
}

// --------------------------------------------------------------------
// UnregisterRegisteredCaptureDeviceChangeNotification
// --------------------------------------------------------------------

void UnregisterRegisteredCaptureDeviceChangeNotification(HWND hwnd) noexcept(false)
{
    if (!g_HDevNofity) { return; }

    // Restore the original WindowProc
    SetLastError(0);
    auto returnResult = SetWindowLongPtr(
        hwnd,
        GWLP_WNDPROC,
#ifdef _WIN64
            reinterpret_cast<LONG_PTR>(g_wndprocOriginal)
#else
            reinterpret_cast<LONG>(g_wndprocOriginal)
#endif
    );
    if (returnResult == 0 && GetLastError() != ERROR_SUCCESS)
    {
        throw std::system_error{ E_FAIL, std::system_category(), "Couldn't restore the original WndProc." };
    }

    UnregisterDeviceNotification(g_HDevNofity);
}

// --------------------------------------------------------------------
// AddCaptureDeviceChangeNotificationHandler
// --------------------------------------------------------------------

void AddCaptureDeviceChangeNotificationHandler(
    const WCHAR *pwszDeviceSymbolicLink,
    CAPTURE_DEVICE_CAHNGE_NOTIF_HANDLER pCallback
    )
{
    // Check if no entry is already present
    auto entryFindIterator = std::find_if(
        g_mapHandlers.begin(),
        g_mapHandlers.end(),
        [&pCallback](std::pair<const WCHAR *, CAPTURE_DEVICE_CAHNGE_NOTIF_HANDLER> item)
        {
            return item.second == pCallback;
        }
    );

    // Entry already present
    if (entryFindIterator == g_mapHandlers.end()) { return; }

    // Insert the entry
    g_mapHandlers.insert({ pwszDeviceSymbolicLink, pCallback });
}

// --------------------------------------------------------------------
// RemoveCaptureDeviceChangeNotificationHandler
// --------------------------------------------------------------------

void RemoveCaptureDeviceChangeNotificationHandler(
    const WCHAR *pwszDeviceSymbolicLink,
    CAPTURE_DEVICE_CAHNGE_NOTIF_HANDLER pCallback
    )
{
    auto entryFindIterator = std::find_if(
        g_mapHandlers.begin(),
        g_mapHandlers.end(),
        [&pCallback](std::pair<const WCHAR *, CAPTURE_DEVICE_CAHNGE_NOTIF_HANDLER> item)
        {
            return item.second == pCallback;
        }
    );

    if (entryFindIterator == g_mapHandlers.end()) { return; }

    g_mapHandlers.erase(entryFindIterator);
}
