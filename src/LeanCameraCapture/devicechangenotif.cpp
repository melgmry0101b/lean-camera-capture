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

#pragma managed(push, off)

// =====================
// ====== Globals ======
// =====================

// The registered notification pointer
static HDEVNOTIFY g_HDevNofity{ nullptr };

// Map for the devices' symbolic link and handlers
static std::multimap<const std::wstring &, CAPTURE_DEVICE_CAHNGE_NOTIF_HANDLER*> g_mmapHandlers{};

// The original WindowProc before subclassing
static WNDPROC g_wndprocOriginal{ nullptr };

// ============================================
// ====== Device Change Handler Function ======
// ============================================

// --------------------------------------------------------------------
// OnCaptureDeviceChangeNotification
// --------------------------------------------------------------------

static void OnCaptureDeviceChangeNotification(PDEV_BROADCAST_HDR pHdr)
{
    DEV_BROADCAST_DEVICEINTERFACE *pDi{ nullptr };

    if (!pHdr) { return; }
    if (pHdr->dbch_devicetype != DBT_DEVTYP_DEVICEINTERFACE) { return; }

    pDi = reinterpret_cast<DEV_BROADCAST_DEVICEINTERFACE *>(pHdr);

    for (const auto& item : g_mmapHandlers)
    {
        if (!item.second) { continue; }
        if (_wcsicmp(item.first.c_str(), pDi->dbcc_name) != 0) { continue; }

        // Call the handler
        (*(item.second))();
    }
}

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
        OnCaptureDeviceChangeNotification(reinterpret_cast<PDEV_BROADCAST_HDR>(lParam));
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

    // Here we are ignoring `ERROR_INVALID_WINDOW_HANDLE` as the consumer may unregister the handler
    //  after the owner window has been destroyed and the handle became invalid.
    DWORD dwErrorCode{ GetLastError() };
    if (returnResult == 0 && dwErrorCode != ERROR_SUCCESS && dwErrorCode != ERROR_INVALID_WINDOW_HANDLE)
    {
        throw std::system_error{ E_FAIL, std::system_category(), "Couldn't restore the original WndProc." };
    }

    UnregisterDeviceNotification(g_HDevNofity);

    // Set the registered notification pointer to nullptr
    g_HDevNofity = nullptr;
}

// --------------------------------------------------------------------
// AddCaptureDeviceChangeNotificationHandler
// --------------------------------------------------------------------

void AddCaptureDeviceChangeNotificationHandler(
    const std::wstring &deviceSymbolicLink,
    CAPTURE_DEVICE_CAHNGE_NOTIF_HANDLER *ppCallback
    )
{
    // Check if no entry is already present
    auto entryFindIterator = std::find_if(
        g_mmapHandlers.begin(),
        g_mmapHandlers.end(),
        [&deviceSymbolicLink, &ppCallback](std::pair<const std::wstring &, CAPTURE_DEVICE_CAHNGE_NOTIF_HANDLER*> item)
        {
            if (item.first != deviceSymbolicLink) { return false; }

            // We are comparing pointers for the callback
            if (item.second != ppCallback) { return false; }

            return true;
        }
    );

    // Entry already present
    if (entryFindIterator != g_mmapHandlers.end()) { return; }

    // Insert the entry
    g_mmapHandlers.insert({ deviceSymbolicLink, ppCallback });
}

// --------------------------------------------------------------------
// RemoveCaptureDeviceChangeNotificationHandler
// --------------------------------------------------------------------

void RemoveCaptureDeviceChangeNotificationHandler(
    const std::wstring &deviceSymbolicLink,
    CAPTURE_DEVICE_CAHNGE_NOTIF_HANDLER *ppCallback
    )
{
    auto entryFindIterator = std::find_if(
        g_mmapHandlers.begin(),
        g_mmapHandlers.end(),
        [&deviceSymbolicLink, &ppCallback](std::pair<const std::wstring &, CAPTURE_DEVICE_CAHNGE_NOTIF_HANDLER *> item)
        {
            if (item.first != deviceSymbolicLink) { return false; }

            // We are comparing pointers for the callback
            if (item.second != ppCallback) { return false; }

            return true;
        }
    );

    // Entry not present
    if (entryFindIterator == g_mmapHandlers.end()) { return; }

    g_mmapHandlers.erase(entryFindIterator);
}

#pragma managed(pop)
