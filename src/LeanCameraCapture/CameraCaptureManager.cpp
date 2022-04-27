/*-----------------------------------------------------------------*\
 *
 * CameraCaptureManager.cpp
 *   LeanCameraCapture
 *     lean-camera-capture
 *
 * MIT - see LICENSE at root directory
 *
 * CREATED: 2022-3-31 05:13 PM
 * AUTHORS: Mohammed Elghamry <elghamry.connect[at]outlook[dot]com>
 *
\*-----------------------------------------------------------------*/

#include "leancamercapture.h"

#include "CameraCaptureManager.h"

using namespace LeanCameraCapture;

// ============================
// ====== Static Methods ======
// ============================

// --------------------------------------------------------------------
// Start
// --------------------------------------------------------------------

void CameraCaptureManager::Start()
{
    try
    {
        StartMediaFoundation();
    }
    catch (const std::system_error &ex)
    {
        throw gcnew CameraCaptureException{ ex.code().value(), gcnew System::String{ ex.what() } };
    }
}

// --------------------------------------------------------------------
// Stop
// --------------------------------------------------------------------

void CameraCaptureManager::Stop()
{
    try
    {
        StopMediaFoundation();
    }
    catch (const std::system_error &ex)
    {
        throw gcnew CameraCaptureException{ ex.code().value(), gcnew System::String{ ex.what() } };
    }
}
