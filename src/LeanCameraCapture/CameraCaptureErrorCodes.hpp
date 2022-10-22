/*-----------------------------------------------------------------*\
 *
 * CameraCaptureErrorCodes.hpp
 *   LeanCameraCapture
 *     lean-camera-capture
 *
 * MIT - see LICENSE at root directory
 *
 * CREATED: 2022-10-22 08:26 PM
 * AUTHORS: Mohammed Elghamry <elghamry.connect[at]outlook[dot]com>
 *
\*-----------------------------------------------------------------*/

#pragma once

#include "leancamercapture.h"

namespace LeanCameraCapture
{
    public ref class CameraCaptureErrorCodes sealed
    {
    public:
        /// <summary>
        /// Capture device lost error code.
        /// </summary>
        static const int DeviceLost = LEANCAMERACAPTURE_E_DEVICELOST;

    private:
        CameraCaptureErrorCodes() { } // Static Class
    };
}
