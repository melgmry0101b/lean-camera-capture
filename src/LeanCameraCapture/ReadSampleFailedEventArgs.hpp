/*-----------------------------------------------------------------*\
 *
 * ReadSampleFailedEventArgs.hpp
 *   LeanCameraCapture
 *     lean-camera-capture
 *
 * MIT - see LICENSE at root directory
 *
 * CREATED: 2022-10-27 12:30 AM
 * AUTHORS: Mohammed Elghamry <elghamry.connect[at]outlook[dot]com>
 *
\*-----------------------------------------------------------------*/

#pragma once

#include "leancamercapture.h"

namespace LeanCameraCapture
{
    /// <summary>
    /// Provides data for ReadSampleFailed event.
    /// </summary>
    public ref class ReadSampleFailedEventArgs : public System::EventArgs
    {
        /* === Constructor === */
    public:
        ReadSampleFailedEventArgs(System::Int32 hresult, System::String ^errorString) :
            m_hresult{ hresult },
            m_errorString{ errorString }
        { }

        /* === Properties === */
    public:
        /// <summary>
        /// Gets error HResult.
        /// </summary>
        property System::Int32 HResult
        {
            System::Int32 get() { return m_hresult; }
        }

        /// <summary>
        /// Gets error string.
        /// </summary>
        property System::String ^ErrorString
        {
            System::String ^get() { return m_errorString; }
        }

        /* === Backing Fields === */
    private:
        System::Int32   m_hresult;
        System::String  ^m_errorString;
    };
}
