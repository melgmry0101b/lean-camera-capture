/*-----------------------------------------------------------------*\
 *
 * ReadSampleSucceededEventArgs.hpp
 *   LeanCameraCapture
 *     lean-camera-capture
 *
 * MIT - see LICENSE at root directory
 *
 * CREATED: 2022-10-27 12:29 AM
 * AUTHORS: Mohammed Elghamry <elghamry.connect[at]outlook[dot]com>
 *
\*-----------------------------------------------------------------*/

#pragma once

#include "leancamercapture.h"

namespace LeanCameraCapture
{
    /// <summary>
    /// Provides data for ReadSampleSucceeded event.
    /// </summary>
    public ref class ReadSampleSucceededEventArgs : public System::EventArgs
    {
        /* === Constructor === */
    public:
        ReadSampleSucceededEventArgs(
            array<System::Byte> ^buffer,
            System::UInt32 widthInPixels,
            System::UInt32 heightInPixels,
            System::UInt32 bytesPerPixel) :
            m_widthInPixels{ widthInPixels },
            m_heightInPixels{ heightInPixels },
            m_bytesPerPixel{ bytesPerPixel }
        {
            // We set the array in the body of the constructor not in the initializer list
            //  as a workaround for error `C2440`:
            //  `Initialization of a managed array with an initializer list is not supported in this context`
            m_buffer = buffer;
        }

        /* === Methods === */
    public:
        /// <summary>
        /// Get sample's buffer.
        /// </summary>
        array<System::Byte> ^GetBuffer()
        {
            // Refer to https://learn.microsoft.com/en-us/dotnet/fundamentals/code-analysis/quality-rules/ca1819
            //  for why to not use a property that returns arrays.
            // tl;dr, if the consumer used the property as indexed property, the code will return the pointer
            //  on each call which has a performance penalty. This is one of the reasons.

            // Here we return a copy of the array.
            return safe_cast<array<System::Byte> ^>(m_buffer->Clone());
        }

        /* === Properties === */
    public:
        /// <summary>
        /// Gets sample width in pixels.
        /// </summary>
        property System::UInt32 WidthInPixels
        {
            System::UInt32 get() { return m_widthInPixels; }
        }

        /// <summary>
        /// Gets sample height in pixels
        /// </summary>
        property System::UInt32 HeightInPixels
        {
            System::UInt32 get() { return m_heightInPixels; }
        }

        /// <summary>
        /// Gets bytes per pixel.
        /// </summary>
        property System::UInt32 BytesPerPixel
        {
            System::UInt32 get() { return m_bytesPerPixel; }
        }

        /* === Backing Fields === */
    private:
        array<System::Byte>     ^m_buffer;
        System::UInt32          m_widthInPixels;
        System::UInt32          m_heightInPixels;
        System::UInt32          m_bytesPerPixel;
    };
}
