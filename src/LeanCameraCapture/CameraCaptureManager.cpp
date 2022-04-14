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
    if (Started) { return; }

    try
    {
        StartMediaFoundation();
        Started = true;
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
    if (!Started) { return; }

    try
    {
        StopMediaFoundation();
        Started = false;
    }
    catch (const std::system_error &ex)
    {
        throw gcnew CameraCaptureException{ ex.code().value(), gcnew System::String{ ex.what() } };
    }
}
