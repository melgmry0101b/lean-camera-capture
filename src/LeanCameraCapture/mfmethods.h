/*-----------------------------------------------------------------*\
 *
 * mfmethods.h
 *   LeanCameraCapture
 *     lean-camera-capture
 *
 * MIT - see LICENSE at root directory
 *
 * CREATED: 2022-3-30 08:42 PM
 * AUTHORS: Mohammed Elghamry <elghamry.connect[at]outlook[dot]com>
 *
\*-----------------------------------------------------------------*/

#pragma once

#include "leancamercapture.h"

/// <summary>
/// [Internal][Native] Start the media foundation
/// </summary>
void StartMediaFoundation(HWND hwndMain) noexcept(false);

/// <summary>
/// [Internal][Native] Stop the media foundation
/// </summary>
void StopMediaFoundation() noexcept(false);

/// <summary>
/// [Internal][Native] Gets the state of Media Foundation
/// </summary>
bool GetIsMediaFoundationStarted();

/// <summary>
/// [Internal][Native] Get registered Main HWND.
/// </summary>
HWND GetMainHwnd();
