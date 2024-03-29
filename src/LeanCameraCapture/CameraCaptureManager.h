/*-----------------------------------------------------------------*\
 *
 * CameraCaptureManager.h
 *   LeanCameraCapture
 *     lean-camera-capture
 *
 * MIT - see LICENSE at root directory
 *
 * CREATED: 2022-3-31 04:30 PM
 * AUTHORS: Mohammed Elghamry <elghamry.connect[at]outlook[dot]com>
 *
\*-----------------------------------------------------------------*/

#pragma once

#include "leancamercapture.h"

namespace LeanCameraCapture
{
    public ref class CameraCaptureManager sealed
    {
        /* Member Functions */
    public:
        /// <summary>
        /// Start the infrastructure for the library (Initialize COM and Media Foundation)
        /// </summary>
        /// <param name="MainHWnd">HWND for main window that can be used to receive notifications.</param>
        static void Start(System::IntPtr MainHWnd);

        /// <summary>
        /// Stop the infrastructure for the library (Stop Media Foundation and Uninitialize COM)
        /// </summary>
        static void Stop();

    private:
        CameraCaptureManager() {  } // Static class

        /* Properties */
    public:
        /// <summary>
        /// The state of the manager
        /// </summary>
        static property bool IsStarted
        {
            bool get() { return GetIsMediaFoundationStarted(); }
        }
    };
}
