/*-----------------------------------------------------------------*\
 *
 * CameraCaptureReader.cpp
 *   LeanCameraCapture
 *     lean-camera-capture
 *
 * MIT - see LICENSE at root directory
 *
 * CREATED: 2022-4-8 10:27 PM
 * AUTHORS: Mohammed Elghamry <elghamry.connect[at]outlook[dot]com>
 *
\*-----------------------------------------------------------------*/

#include <msclr\lock.h>

#include "leancamercapture.h"

#include "CameraCaptureReader.h"

using namespace System::Runtime::InteropServices;

using namespace LeanCameraCapture;

// =========================
// ====== Constructor ======
// =========================

CameraCaptureReader::CameraCaptureReader(CameraCaptureDevice ^device) :
    m_pCSourceReader{ nullptr },
    m_CSourceReaderReadFrameSuccessHandler{ nullptr },
    m_CSourceReaderReadFrameFailHandler{ nullptr }
{
    if (!device)
    {
        throw gcnew System::ArgumentNullException(STRINGIZE(device));
    }

    m_device = device;

    m_buffer = nullptr;

    m_lock = gcnew System::Object();

    m_CSourceReaderReadFrameSuccessHandler
        = gcnew ReadFrameSuccessNativeCallback(this, &CameraCaptureReader::ReadFrameSuccessNativeHandler);
    m_CSourceReaderReadFrameFailHandler
        = gcnew ReadFrameFailNativeCallback(this, &CameraCaptureReader::ReadFrameFailNativeHandler);
}

// ============================
// ====== Public Methods ======
// ============================

void CameraCaptureReader::Open()
{
    System::Diagnostics::Debug::Assert(m_device != nullptr);

    // Lock
    msclr::lock l{ m_lock };

    // Check if the reader is already open.
    if (IsOpen) { return; }

    // Create new CSourceReader
    auto newSourceReader = new Native::CSourceReader();

    // Prepare the source reader
    try
    {
        // Initialize native source reader.
        newSourceReader->InitializeForDevice(m_device->GetNativeDeviceSymbolicLink());
    }
    catch (const std::logic_error &ex)
    {
        SafeRelease(&newSourceReader);
        throw gcnew System::InvalidOperationException(gcnew System::String(ex.what()));
    }
    catch (const std::system_error &ex)
    {
        SafeRelease(&newSourceReader);
        throw gcnew CameraCaptureException(ex.code().value(), gcnew System::String(ex.what()));
    }
    catch (const std::exception &ex)
    {
        SafeRelease(&newSourceReader);
        throw gcnew CameraCaptureException(E_UNEXPECTED, gcnew System::String(ex.what()));
    }

    // Set handlers
    newSourceReader->SetReadFrameSuccessCallback(
        static_cast<Native::FP_READ_SAMPLE_SUCCESS_HANDLER>(
            Marshal::GetFunctionPointerForDelegate(m_CSourceReaderReadFrameSuccessHandler).ToPointer()
            )
    );

    newSourceReader->SetReadFrameFailCallback(
        static_cast<Native::FP_READ_SAMPLE_FAIL_HANDLER>(
            Marshal::GetFunctionPointerForDelegate(m_CSourceReaderReadFrameFailHandler).ToPointer()
            )
    );

    m_pCSourceReader = newSourceReader;
    // Don't use AddRef, as this is just "moving" the reference not adding new one.
}

void CameraCaptureReader::Close()
{
    // Lock
    msclr::lock l{ m_lock };

    // Check if the reader is already closed
    if (!IsOpen) { return; }

    m_pCSourceReader->SetReadFrameSuccessCallback(nullptr);
    m_pCSourceReader->SetReadFrameFailCallback(nullptr);
    
    m_pCSourceReader->Close();

    // Release the native source reader
    // Copying pointer to a local variable avoiding
    //  Error C2784 "could not deduce template argument for 'T **' from 'cli::interior_ptr<CSourceReader *>'"
    // btw, decided not to hop around pin_ptr for this.
    Native::CSourceReader *pCSourceReader{ m_pCSourceReader };
    SafeRelease(&pCSourceReader);
    m_pCSourceReader = nullptr;
}

void CameraCaptureReader::Reopen()
{
    Close();
    Open();
}

void CameraCaptureReader::ReadSample()
{
    // Lock
    msclr::lock l{ m_lock };

    // Check if the reader is closed
    if (!IsOpen)
    {
        throw gcnew System::InvalidOperationException("Cannot issue a read sample on a closed reader.");
    }

    try
    {
        m_pCSourceReader->ReadFrame();
    }
    catch (const std::logic_error &ex)
    {
        throw gcnew System::InvalidOperationException(gcnew System::String(ex.what()));
    }
    catch (const std::system_error &ex)
    {
        throw gcnew CameraCaptureException(ex.code().value(), gcnew System::String(ex.what()));
    }
    catch (const std::exception &ex)
    {
        throw gcnew CameraCaptureException(E_UNEXPECTED, gcnew System::String(ex.what()));
    }
}

// =============================
// ====== Private Methods ======
// =============================

void CameraCaptureReader::OnReadSampleSucceeded(System::Object ^sender, ReadSampleSucceededEventArgs ^e)
{
    ReadSampleSucceeded(sender, e);
}

void CameraCaptureReader::OnReadSampleFailed(System::Object ^sender, ReadSampleFailedEventArgs ^e)
{
    ReadSampleFailed(sender, e);
}

void CameraCaptureReader::ReadFrameSuccessNativeHandler(
    const BYTE *pbBuffer,
    UINT32 widthInPixels,
    UINT32 heightInPixels,
    UINT32 bytesPerPixel
)
{
    // Lock
    msclr::lock l{ m_lock };

    auto bufferLen = widthInPixels * heightInPixels * bytesPerPixel;
    if (!m_buffer || m_buffer->Length < static_cast<INT32>(bufferLen))
    {
        m_buffer = gcnew array<System::Byte>(bufferLen);
    }

    Marshal::Copy(System::IntPtr(const_cast<void *>(static_cast<const void *>(pbBuffer))), m_buffer, 0, bufferLen);

    l.release();

    OnReadSampleSucceeded(this, gcnew ReadSampleSucceededEventArgs(
        m_buffer, widthInPixels, heightInPixels, bytesPerPixel
    ));
}

void CameraCaptureReader::ReadFrameFailNativeHandler(
    const HRESULT hr,
    const std::string &errorString
)
{
    OnReadSampleFailed(this, gcnew ReadSampleFailedEventArgs(hr, gcnew System::String(errorString.c_str())));
}

// ========================
// ====== Destructor ======
// ========================

CameraCaptureReader::~CameraCaptureReader()
{
    // Release managed resources
    if (m_pCSourceReader)
    {
        m_pCSourceReader->SetReadFrameSuccessCallback(nullptr);
        m_pCSourceReader->SetReadFrameFailCallback(nullptr);
    }

    m_CSourceReaderReadFrameSuccessHandler = nullptr;
    m_CSourceReaderReadFrameFailHandler = nullptr;

    // Call finalizer
    this->!CameraCaptureReader();
}

// =======================
// ====== Finalizer ======
// =======================

CameraCaptureReader::!CameraCaptureReader()
{
    // Release unmanaged resources.
    if (m_pCSourceReader)
    {
        m_pCSourceReader->Close();

        // Copying pointer to a local variable avoiding
        //  Error C2784 "could not deduce template argument for 'T **' from 'cli::interior_ptr<CSourceReader *>'"
        // btw, decided not to hop around pin_ptr for this.
        Native::CSourceReader *pCSourceReader{ m_pCSourceReader };
        SafeRelease(&pCSourceReader);
        m_pCSourceReader = nullptr;
    }
}
