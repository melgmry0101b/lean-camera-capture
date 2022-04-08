/*-----------------------------------------------------------------*\
 *
 * leancamercapture.h
 *   LeanCameraCapture
 *     lean-camera-capture
 *
 * MIT - see LICENSE at root directory
 *
 * CREATED: 2022-3-30 06:10 PM
 * AUTHORS: Mohammed Elghamry <elghamry.connect[at]outlook[dot]com>
 *
\*-----------------------------------------------------------------*/

#pragma once

#include <cassert>
#include <system_error>

// WIN32_LEAN_AND_MEAN isn't much of a performance boost for
//  compilation these days, but helps cleaning the global namespace
//  if you don't want extra api like GDI or Networking
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <objbase.h>
#include <mfapi.h>
#include <mfidl.h>

#include "saferelease.h"
#include "mfmethods.h"

#include "CSourceReader.h"

#include "CameraCaptureException.hpp"
#include "CameraCaptureManager.h"
#include "CameraCaptureDevice.h"
#include "CameraCaptureReader.h"
