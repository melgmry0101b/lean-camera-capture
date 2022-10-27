/*-----------------------------------------------------------------*\
 *
 * devicechangenotif.h
 *   LeanCameraCapture
 *     lean-camera-capture
 *
 * MIT - see LICENSE at root directory
 *
 * CREATED: 2022-4-28 11:46 AM
 * AUTHORS: Mohammed Elghamry <elghamry.connect[at]outlook[dot]com>
 *
\*-----------------------------------------------------------------*/

#pragma once

#include "leancamercapture.h"

#pragma managed(push, off)

/// <summary>
/// Function pointer definition for the device change notification handlers
/// </summary>
typedef std::function<void()> CAPTURE_DEVICE_CAHNGE_NOTIF_HANDLER;

/// <summary>
/// [Internal][Native] Register capture device change notification listener on a window handler
/// </summary>
void RegisterCaptureDeviceChangeNotificationForHwnd(HWND hwnd) noexcept(false);

/// <summary>
/// [Internal][Native] Remove the registered listener
/// </summary>
void UnregisterRegisteredCaptureDeviceChangeNotification(HWND hwnd) noexcept(false);

/// <summary>
/// [Internal][Native] Add a handler for a specific device capture change notification
/// </summary>
void AddCaptureDeviceChangeNotificationHandler(
    const WCHAR *pwszDeviceSymbolicLink,
    CAPTURE_DEVICE_CAHNGE_NOTIF_HANDLER *ppCallback
    );

/// <summary>
/// [Internal][Native] Remove handler
/// </summary>
void RemoveCaptureDeviceChangeNotificationHandler(
    const WCHAR *pwszDeviceSymbolicLink,
    CAPTURE_DEVICE_CAHNGE_NOTIF_HANDLER *ppCallback
    );

#pragma managed(pop)
