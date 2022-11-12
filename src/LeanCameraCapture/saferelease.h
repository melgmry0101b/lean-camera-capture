/*-----------------------------------------------------------------*\
 *
 * saferelease.h
 *   LeanCameraCapture
 *     lean-camera-capture
 *
 * MIT - see LICENSE at root directory
 *
 * CREATED: 2022-3-30 06:54 PM
 * AUTHORS: Mohammed Elghamry <elghamry.connect[at]outlook[dot]com>
 *
\*-----------------------------------------------------------------*/

#pragma once

#pragma managed(push, off)

// See: https://docs.microsoft.com/en-us/windows/win32/medfound/saferelease

/// <summary>
/// Release COM interface pointers
/// </summary>
template <class T>
void SafeRelease(T **ppT)
{
    if (*ppT)
    {
        (*ppT)->Release();
        *ppT = nullptr;
    }
}

#pragma managed(pop)
