/*-----------------------------------------------------------------*\
 *
 * CameraCaptureException.hpp
 *   LeanCameraCapture
 *     lean-camera-capture
 *
 * MIT - see LICENSE at root directory
 *
 * CREATED: 2022-3-30 10:41 PM
 * AUTHORS: Mohammed Elghamry <elghamry.connect[at]outlook[dot]com>
 *
\*-----------------------------------------------------------------*/

#pragma once

#include "leancamercapture.h"

namespace LeanCameraCapture
{
    public ref class CameraCaptureException : public System::Exception
    {
    internal:
        CameraCaptureException() {  }
        CameraCaptureException(System::String ^message) : Exception(message) {  }
        CameraCaptureException(System::String ^message, System::Exception ^inner) : Exception(message, inner) {  }
        CameraCaptureException(int hResult)
        {
            HResult = hResult;
        }
        CameraCaptureException(int hResult, System::String ^message) : Exception(message)
        {
            HResult = hResult;
        }
        CameraCaptureException(int hResult, System::String ^message, System::Exception ^inner) : Exception(message, inner)
        {
            HResult = hResult;
        }
    };
}
