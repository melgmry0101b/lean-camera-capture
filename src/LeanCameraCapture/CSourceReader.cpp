/*-----------------------------------------------------------------*\
 *
 * CSourceReader.cpp
 *   LeanCameraCapture
 *     lean-camera-capture
 *
 * MIT - see LICENSE at root directory
 *
 * CREATED: 2022-4-8 10:31 PM
 * AUTHORS: Mohammed Elghamry <elghamry.connect[at]outlook[dot]com>
 *
\*-----------------------------------------------------------------*/

#include "leancamercapture.h"

#include "CSourceReader.h"

#define OUTPUT_VIDEO_SUBTYPE MFVideoFormat_RGB32
#define OUTPUT_BYTES_PER_PIXEL 4

#pragma managed(push, off)

using namespace std::string_literals;

using namespace LeanCameraCapture::Native;

// ==============================
// ====== IUnknown methods ======
// ==============================

ULONG CSourceReader::AddRef()
{
    return InterlockedIncrement(&m_nRefCount);
}

ULONG CSourceReader::Release()
{
    ULONG uCount = InterlockedDecrement(&m_nRefCount);
    if (uCount == 0)
    {
        delete this;
    }
    return uCount;
}

HRESULT CSourceReader::QueryInterface(REFIID riid, void **ppv)
{
    static const QITAB qit[]{
        QITABENT(CSourceReader, IMFSourceReaderCallback),
        { 0 },
    };
    return QISearch(this, qit, riid, ppv);
}

// =============================================
// ====== IMFSourceReaderCallback methods ======
// =============================================

// --------------------------------------------------------------------
// OnReadSample
//
// A callback method that is called on receiving
//  a sample after a request using IMFSourceReader::ReadSample
// --------------------------------------------------------------------

HRESULT CSourceReader::OnReadSample(
    HRESULT hrStatus,
    DWORD /*dwStreamIndex*/,
    DWORD /*dwStreamFlags*/,
    LONGLONG /*llTimestamp*/,
    IMFSample *pSample
    )
{
    HRESULT hr{ hrStatus };

    std::string exWhatString{};

    IMFSample       *pOutputSample{ nullptr };
    IMFMediaBuffer  *pBuffer{ nullptr };

    BYTE *pbScanline0{ nullptr };
    LONG lStride{ 0 };

    _RPT1(_CRT_WARN, "Waiting to enter critical section from %s.\n", STRINGIZE(OnReadSample));

    EnterCriticalSection(&m_criticalSection);

    _RPT1(_CRT_WARN, "Entered critical section in %s.\n", STRINGIZE(OnReadSample));

    // Check if the CSourceReader has been closed before entering the critical section.
    if (!m_bIsAvailable)
    {
        LeaveCriticalSection(&m_criticalSection);
        _RPT1(_CRT_WARN, "CSourceReader isn't available during '%s', left critical section and returning.\n", STRINGIZE(OnReadSample));
        return hr;
    }

    // Check if hr is failed
    CHECK_FAILED_HR_WITH_GOTO_AND_EX_STR(hr, done, exWhatString, "Error passed from IMFSourceReader.");

    // Read from the sample if available
    if (pSample)
    {
        // Convert the buffer to RGB32
        try
        {
            ProcessorProcessSample(0, pSample, &pOutputSample);
        }
        catch (const std::system_error &ex)
        {
            hr = ex.code().value();

            exWhatString = std::string{ MAKE_EX_STR("Error occurred while processing sample.") }
                + "\nWith Error: " + ex.what() + " (" + std::to_string(ex.code().value()) + ")";

            goto done;
        }

        // Get the buffer for the frame from the sample if the buffer is set
        if (pOutputSample)
        {
            hr = pOutputSample->GetBufferByIndex(0, &pBuffer);
            CHECK_FAILED_HR_WITH_GOTO_AND_EX_STR(hr, done, exWhatString, "Error occurred during IMFSample::GetBufferByIndex().");

            // Lock the buffer
            CBufferLock buffer{ pBuffer };
            hr = buffer.LockBuffer(m_lSrcDefaultStride, m_frameHeight, &pbScanline0, &lStride);
            CHECK_FAILED_HR_WITH_GOTO_AND_EX_STR(hr, done, exWhatString, "Error occurred during locking buffer.");

            // HACK: We should've probably checked for the width and the height of the sample, but as we don't change
            //  the processor's output media type, we have a guarantee about the width and height, and thus the buffer's size.

            // Copy the frame
            // Note that here we are multiplying with 4 as we are using RGB32 (4 byte) format
            hr = MFCopyImage(
                m_frameBuffer.get(),
                m_frameWidth * OUTPUT_BYTES_PER_PIXEL,
                pbScanline0,
                lStride,
                m_frameWidth * OUTPUT_BYTES_PER_PIXEL,
                m_frameHeight
            );
            CHECK_FAILED_HR_WITH_GOTO_AND_EX_STR(hr, done, exWhatString, "Error occurred during MFCopyImage().");
        }
    }

done:
    SafeRelease(&pOutputSample);
    SafeRelease(&pBuffer);

    LeaveCriticalSection(&m_criticalSection);

    _RPT1(_CRT_WARN, "Left critical section in %s.\n", STRINGIZE(OnReadSample));

    if (SUCCEEDED(hr))
    {
        if (m_pReadSampleSuccessCallback)
        {
            m_pReadSampleSuccessCallback(m_frameBuffer.get(), m_frameWidth, m_frameHeight, OUTPUT_BYTES_PER_PIXEL);
        }
    }
    else
    {
        if (m_pReadSampleFailCallback)
        {
            m_pReadSampleFailCallback(hr, exWhatString);
        }        
    }

    return hr;
}

// =========================
// ====== Constructor ======
// =========================

CSourceReader::CSourceReader() :
    m_nRefCount{ 1 },
    m_criticalSection{},
    m_bIsInitialized{ false },
    m_bIsAvailable{ false },
    m_pMediaSource{ nullptr },
    m_pSourceReader{ nullptr },
    m_pProcessor{ nullptr },
    m_lSrcDefaultStride{ 0 },
    m_frameWidth{ 0 },
    m_frameHeight{ 0 },
    m_processorOutputBuffer{ nullptr },
    m_processorOutputInfo{ 0 },
    m_frameBuffer{ nullptr },
    m_wstrDeviceSymbolicLink{},
    m_pReadSampleSuccessCallback{ nullptr },
    m_pReadSampleFailCallback{ nullptr },
    m_pDeviceChangeNotifHandler{ nullptr }
{
    InitializeCriticalSection(&m_criticalSection);

    // Set device change notification handler
    m_pDeviceChangeNotifHandler = [this] { CaptureDeviceChangeNotificationHandler(); };
}

// ========================
// ====== Destructor ======
// ========================

CSourceReader::~CSourceReader()
{
    FreeResources();

    // Remove the device change notification handler
    RemoveCaptureDeviceChangeNotificationHandler(m_wstrDeviceSymbolicLink, &m_pDeviceChangeNotifHandler);

    DeleteCriticalSection(&m_criticalSection);

    _RPT0(_CRT_WARN, "CSourceReader destructor has been called.\n");
}

// ===============================
// ====== Private Functions ======
// ===============================

// --------------------------------------------------------------------
// FreeResources
// --------------------------------------------------------------------

void CSourceReader::FreeResources()
{
    _RPT1(_CRT_WARN, "Waiting to enter critical section from %s.\n", STRINGIZE(FreeResources));

    EnterCriticalSection(&m_criticalSection);

    _RPT1(_CRT_WARN, "Entered critical section in %s.\n", STRINGIZE(FreeResources));

    // Shutdown the media source before releasing
    if (m_pMediaSource)
    {
        m_pMediaSource->Shutdown();
    }

    SafeRelease(&m_pProcessor);
    SafeRelease(&m_pSourceReader);

    SafeRelease(&m_pMediaSource);

    SafeRelease(&m_processorOutputBuffer);

    m_bIsAvailable = false;

    LeaveCriticalSection(&m_criticalSection);

    _RPT1(_CRT_WARN, "Left critical section in %s.\n", STRINGIZE(FreeResources));
}

// --------------------------------------------------------------------
// CaptureDeviceChangeNotificationHandler
//
// On changes to the attached capture device, we will set the reader
//  as unavailable so the consumer should initialize a new reader
//  for a new device.
// --------------------------------------------------------------------

void CSourceReader::CaptureDeviceChangeNotificationHandler()
{
    _RPT1(_CRT_WARN, "Waiting to enter critical section from %s.\n", STRINGIZE(CaptureDeviceChangeNotificationHandler));

    EnterCriticalSection(&m_criticalSection);

    _RPT1(_CRT_WARN, "Entered critical section in %s.\n", STRINGIZE(CaptureDeviceChangeNotificationHandler));

    // Set the reader as unavailable
    m_bIsAvailable = false;

    LeaveCriticalSection(&m_criticalSection);

    _RPT1(_CRT_WARN, "Left critical section in %s.\n", STRINGIZE(CaptureDeviceChangeNotificationHandler));
}

// --------------------------------------------------------------------
// ProcessorProcessOutput
// --------------------------------------------------------------------

void CSourceReader::ProcessorProcessOutput(
    DWORD dwOutputStreamID,
    IMFSample **ppOutputSample,
    bool bDrain
    )
{
    assert(m_pProcessor != nullptr);
    assert(!bDrain || ppOutputSample == nullptr); // You can't set ppOutputSample for drain.

    HRESULT hr{ S_OK };
    std::string exWhatString{ };

    IMFSample *pOutputSample{ nullptr };

    // NOTE: why did we use `bDrain`?
    //      I don't encourage exceptions as a part of the normal control flow,
    //  so, catching an exception for a normal `Drain` operation is not favorable.
    //  This lead me to add a flag which if set the function just drains the processor.
    //  I added this flag to the function instead of creating a new one because of the shared logic.

    if (bDrain)
    {
        hr = m_pProcessor->ProcessMessage(MFT_MESSAGE_NOTIFY_END_OF_STREAM, 0);
        CHECK_FAILED_HR_WITH_GOTO_AND_EX_STR(hr, done, exWhatString, "Error occurred during IMFTransform::ProcessMessage().");

        hr = m_pProcessor->ProcessMessage(MFT_MESSAGE_COMMAND_DRAIN, 0);
        CHECK_FAILED_HR_WITH_GOTO_AND_EX_STR(hr, done, exWhatString, "Error occurred during IMFTransform::ProcessMessage().");
    }

    do {
        DWORD dwProcessOutputStatus{ 0 };
        MFT_OUTPUT_STREAM_INFO outputStreamInfo{ 0 };
        MFT_OUTPUT_DATA_BUFFER outputDataBuffer{ 0 };

        hr = m_pProcessor->GetOutputStreamInfo(dwOutputStreamID, &outputStreamInfo);
        CHECK_FAILED_HR_WITH_GOTO_AND_EX_STR(hr, done, exWhatString, "Error occurred during IMFTransform::GetOutputStreamInfo().");

        // Check if the sample should be allocated by us
        if ((outputStreamInfo.dwFlags & (MFT_OUTPUT_STREAM_PROVIDES_SAMPLES | MFT_OUTPUT_STREAM_CAN_PROVIDE_SAMPLES))
            != (MFT_OUTPUT_STREAM_PROVIDES_SAMPLES | MFT_OUTPUT_STREAM_CAN_PROVIDE_SAMPLES))
        {
            // TODO: This part needs optimization, we should reuse previous buffers instead of creating a new buffer each time,
            //  this is an overhead we should optimize, and handle cases where the processor can provide the sample for us.
            // NOTE: As this library is intended for quick preview and take a still shot, this isn't particularly painful,
            //  but this note is here for next iterations and versions -if so :)-

            // Create buffer if we don't have one
            if (!m_processorOutputBuffer
                || m_processorOutputInfo.cbSize != outputStreamInfo.cbSize
                || m_processorOutputInfo.cbAlignment != outputStreamInfo.cbAlignment)
            {
                _RPT0(_CRT_WARN, "Creating a new buffer for processor output.\n");

                // Release old buffer
                SafeRelease(&m_processorOutputBuffer);

                // Create new buffer
                hr = MFCreateAlignedMemoryBuffer(outputStreamInfo.cbSize, outputStreamInfo.cbAlignment, &m_processorOutputBuffer);
                CHECK_FAILED_HR_WITH_GOTO_AND_EX_STR(hr, done, exWhatString, "Error occurred during MFCreateAlignedMemoryBuffer().");

                // Save buffer's details
                m_processorOutputInfo = outputStreamInfo;
            }

            // Create the output sample
            hr = MFCreateSample(&pOutputSample);
            CHECK_FAILED_HR_WITH_GOTO_AND_EX_STR(hr, done, exWhatString, "Error occurred during MFCreateSample().");

            // Add buffer to the sample
            hr = pOutputSample->AddBuffer(m_processorOutputBuffer);
            CHECK_FAILED_HR_WITH_GOTO_AND_EX_STR(hr, done, exWhatString, "Error occurred during IMFSample::AddBuffer().");
        }

        // Get the output
        outputDataBuffer.dwStreamID = dwOutputStreamID;
        outputDataBuffer.pSample = pOutputSample;
        outputDataBuffer.dwStatus = 0;
        outputDataBuffer.pEvents = nullptr;
        hr = m_pProcessor->ProcessOutput(0, 1, &outputDataBuffer, &dwProcessOutputStatus);
        CHECK_FAILED_HR_WITH_GOTO_AND_EX_STR(hr, done, exWhatString, "Error occurred during IMFTransfer::ProcessOutput().");

        // set the output pointer if the caller is willing to receive the sample, and is available to begin with!
        // Note that we assert on if bDrain is true, ppOutputSample has to be nullptr
        if (ppOutputSample
            && ((outputDataBuffer.dwStatus & MFT_OUTPUT_DATA_BUFFER_NO_SAMPLE) != MFT_OUTPUT_DATA_BUFFER_NO_SAMPLE))
        {
            *ppOutputSample = pOutputSample;
            (*ppOutputSample)->AddRef();

            // Here we break out of the loop as ppOutputSample won't be set in case of a drain process,
            //  so we break out otherwise to not release it twice.
            break;
        }

        // Release for next iteration in case of drain
        SafeRelease(&pOutputSample);
    } while (bDrain); // If we are in drain mode, loop till we get exception with `MF_E_TRANSFORM_NEED_MORE_INPUT` or others.



done:
    SafeRelease(&pOutputSample);

    if (FAILED(hr))
    {
        if (hr == MF_E_TRANSFORM_NEED_MORE_INPUT && bDrain) { return; }

        throw std::system_error{ hr, std::system_category(), exWhatString };
    }
}

// --------------------------------------------------------------------
// ProcessorProcessSample
// --------------------------------------------------------------------

void CSourceReader::ProcessorProcessSample(
    DWORD dwStreamID,
    IMFSample *pInputSample,
    IMFSample **ppOutputSample
)
{
    assert(m_pProcessor != nullptr);
    assert(pInputSample != nullptr);
    assert(ppOutputSample != nullptr);

    HRESULT hr{ S_OK };
    std::string exWhatString{ };

    IMFSample *pOutputSample{ nullptr };

    hr = m_pProcessor->ProcessMessage(MFT_MESSAGE_NOTIFY_BEGIN_STREAMING, 0);
    CHECK_FAILED_HR_WITH_GOTO_AND_EX_STR(hr, done, exWhatString, "Error occurred during IMFTransform::ProcessMessage().");

    hr = m_pProcessor->ProcessInput(dwStreamID, pInputSample, 0);
    CHECK_FAILED_HR_WITH_GOTO_AND_EX_STR(hr, finalizeAndDrain, exWhatString, "Error occurred during IMFTransform::ProcessInput().");

    try
    {
        ProcessorProcessOutput(dwStreamID, &pOutputSample);
    }
    catch (const std::system_error &ex)
    {
        hr = ex.code().value();

        exWhatString = std::string{ MAKE_EX_STR("Error occurred while processing output.") }
        + "\nWith Error: " + ex.what() + " (" + std::to_string(ex.code().value()) + ")";

        goto finalizeAndDrain;
    }

    if (ppOutputSample)
    {
        *ppOutputSample = pOutputSample;
        (*ppOutputSample)->AddRef();
    }

finalizeAndDrain:
    try
    {
        ProcessorProcessOutput(dwStreamID, nullptr, true);
    }
    catch (const std::system_error &ex)
    {
        hr = ex.code().value();
        if (exWhatString.empty())
        {
            exWhatString = std::string{ MAKE_EX_STR("Error occurred while draining processor.") }
                + "\nWith Error: " + ex.what() + " (" + std::to_string(ex.code().value()) + ")";
        }
        else
        {
            exWhatString = std::string{ MAKE_EX_STR("Error occurred while draining processor.") }
                + "\nWith Error: " + ex.what() + " (" + std::to_string(ex.code().value()) + ")"
                + "\nWith Error: " + exWhatString;
        }

        goto done;
    }

done:
    SafeRelease(&pOutputSample);

    if (FAILED(hr))
    {
        throw std::system_error{ hr, std::system_category(), exWhatString };
    }
}

// ==============================
// ====== Public Functions ======
// ==============================

// --------------------------------------------------------------------
// SetReadFrameSuccessCallback
// --------------------------------------------------------------------

void CSourceReader::SetReadFrameSuccessCallback(READ_SAMPLE_SUCCESS_HANDLER pCallback)
{
    m_pReadSampleSuccessCallback = pCallback;
}

// --------------------------------------------------------------------
// SetReadFrameFailCallback
// --------------------------------------------------------------------

void CSourceReader::SetReadFrameFailCallback(READ_SAMPLE_FAIL_HANDLER pCallback)
{
    m_pReadSampleFailCallback = pCallback;
}

// --------------------------------------------------------------------
// ReadFrame
// --------------------------------------------------------------------

void CSourceReader::ReadFrame()
{
    _RPT1(_CRT_WARN, "Waiting to enter critical section from %s.\n", STRINGIZE(ReadFrame));

    EnterCriticalSection(&m_criticalSection);

    _RPT1(_CRT_WARN, "Entered critical section in %s.\n", STRINGIZE(ReadFrame));

    if (!m_bIsInitialized)
    {
        LeaveCriticalSection(&m_criticalSection);
        throw std::logic_error{ "Source reader hasn't been initialized." };
    }

    if (!m_pSourceReader)
    {
        LeaveCriticalSection(&m_criticalSection);
        throw std::logic_error{ "Instance's source reader is null." };
    }

    if (!GetIsMediaFoundationStarted())
    {
        LeaveCriticalSection(&m_criticalSection);
        throw std::logic_error{ "Media Foundation hasn't started." };
    }

    if (!m_bIsAvailable)
    {
        LeaveCriticalSection(&m_criticalSection);
        throw std::system_error{ static_cast<int>(LEANCAMERACAPTURE_E_DEVICELOST), std::system_category(), "Capture device isn't available." };
    }

    HRESULT hr{ S_OK };

    hr = m_pSourceReader->ReadSample(
            static_cast<DWORD>(MF_SOURCE_READER_FIRST_VIDEO_STREAM),
            0,
            nullptr,
            nullptr,
            nullptr,
            nullptr
            );

    LeaveCriticalSection(&m_criticalSection);

    _RPT1(_CRT_WARN, "Left critical section in %s.\n", STRINGIZE(ReadFrame));

    if (FAILED(hr))
    {
        throw std::system_error{ hr, std::system_category(), "Error occurred during IMFSourceReader::ReadSample()." };
    }
}

// --------------------------------------------------------------------
// InitializeForDevice
//
// Here we initialize this instance of CSourceReader
//  to device that exposes IMFActivate. This can be
//  done only once per instance.
// --------------------------------------------------------------------

void CSourceReader::InitializeForDevice(WCHAR *pwszDeviceSymbolicLink) noexcept(false)
{
    _RPTFW1(_CRT_WARN, L"CSourceReader::InitializeForDevice() called for device with symlink '%s'.\n", pwszDeviceSymbolicLink);

    if (!pwszDeviceSymbolicLink)
    {
        throw std::logic_error{ "Device symbolic link is null." };
    }

    if (!GetIsMediaFoundationStarted())
    {
        throw std::logic_error{ "Media foundation hasn't started." };
    }

    // This method should be called only once
    if (m_bIsInitialized)
    {
        throw std::logic_error{ "This instance of CSourceReader is already initialized for a device." };
    }

    HRESULT hr{ S_OK };
    std::string exWhatString{};

    IMFAttributes   *pAttributes{ nullptr };
    IMFMediaType    *pSourceOutputMediaType{ nullptr };
    IMFMediaType    *pProcessorOutputMediaType{ nullptr };

    CLSID *pMFTCLSIDs{ nullptr };
    UINT32 MFTCLSIDsCount{ 0 };

    MFT_REGISTER_TYPE_INFO processorInputInfo{ 0 };
    MFT_REGISTER_TYPE_INFO processorOutputInfo{ 0 };

    GUID sourceOutputSubtype{ GUID_NULL };

    _RPTF1(_CRT_WARN, "Waiting to enter critical section from %s.\n", STRINGIZE(InitializeForDevice));

    EnterCriticalSection(&m_criticalSection);

    _RPTF1(_CRT_WARN, "Entered critical section in %s.\n", STRINGIZE(InitializeForDevice));

    // ---
    // --- Create media source for the symbolic link
    // ---

    // Create attributes to pass to `MFCreateDeviceSource` with two slots
    hr = MFCreateAttributes(&pAttributes, 2);
    CHECK_FAILED_HR_WITH_GOTO_AND_EX_STR(hr, done, exWhatString, "Error occurred during MFCreateAttributes().");

    hr = pAttributes->SetGUID(MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE, MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE_VIDCAP_GUID);
    CHECK_FAILED_HR_WITH_GOTO_AND_EX_STR(hr, done, exWhatString, "Error occurred during IMFAttributes::SetGUID().");

    hr = pAttributes->SetString(MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE_VIDCAP_SYMBOLIC_LINK, pwszDeviceSymbolicLink);
    CHECK_FAILED_HR_WITH_GOTO_AND_EX_STR(hr, done, exWhatString, "Error occurred during IMFAttributes::SetString().");

    // Create the media source for the device
    hr = MFCreateDeviceSource(pAttributes, &m_pMediaSource);
    CHECK_FAILED_HR_WITH_GOTO_AND_EX_STR(hr, done, exWhatString, "Error occurred during MFCreateDeviceSource().");

    // Release the attributes for the next use
    SafeRelease(&pAttributes);

    _RPTFW1(_CRT_WARN, L"Device source created for '%s'.\n", pwszDeviceSymbolicLink);

    // ---
    // --- Create the source reader
    // ---
    
    // Create attributes to hold settings with 2 settings' slots
    hr = MFCreateAttributes(&pAttributes, 2);
    CHECK_FAILED_HR_WITH_GOTO_AND_EX_STR(hr, done, exWhatString, "Error occurred during MFCreateAttributes().");

    hr = pAttributes->SetUINT32(MF_READWRITE_DISABLE_CONVERTERS, true);
    CHECK_FAILED_HR_WITH_GOTO_AND_EX_STR(hr, done, exWhatString, "Error occurred during IMFAttributes::SetUINT32().");

    // Set the an attribute slot for this class instance as callback for events e.g. OnReadSample
    hr = pAttributes->SetUnknown(
        MF_SOURCE_READER_ASYNC_CALLBACK,
        this
        );
    CHECK_FAILED_HR_WITH_GOTO_AND_EX_STR(hr, done, exWhatString, "Error occurred during IMFAttributes::SetUnknown().");

    // Create source reader for the media source using the attributes
    hr = MFCreateSourceReaderFromMediaSource(
        m_pMediaSource,
        pAttributes,
        &m_pSourceReader
        );
    CHECK_FAILED_HR_WITH_GOTO_AND_EX_STR(hr, done, exWhatString, "Error occurred during MFCreateSourceReaderFromMediaSource().");

    // Release the attributes for the next use
    SafeRelease(&pAttributes);

    _RPTFW1(_CRT_WARN, L"Source reader created for '%s'.\n", pwszDeviceSymbolicLink);

    // ---
    // --- Find the suitable codec for the video to RGB32
    // ---

    processorInputInfo.guidMajorType = MFMediaType_Video;
    
    processorOutputInfo.guidMajorType = MFMediaType_Video;
    processorOutputInfo.guidSubtype = OUTPUT_VIDEO_SUBTYPE; // Our output type is RGB32

    // Loop through the available output types in the source reader and check
    for (DWORD i = 0; ; i++)
    {
        hr = m_pSourceReader->GetNativeMediaType(
            static_cast<DWORD>(MF_SOURCE_READER_FIRST_VIDEO_STREAM),
            i,
            &pSourceOutputMediaType
            );
        CHECK_FAILED_HR_WITH_GOTO_AND_EX_STR(hr, done, exWhatString, "Could not find suitable codec converting into RGB32. IMFSourceReader::GetNativeMediaType().");

        hr = pSourceOutputMediaType->GetGUID(MF_MT_SUBTYPE, &sourceOutputSubtype);
        CHECK_FAILED_HR_WITH_GOTO_AND_EX_STR(hr, done, exWhatString, "Error occurred during IMFMediaType::GetGUID().");

        processorInputInfo.guidSubtype = sourceOutputSubtype;

        _RPTFW2(_CRT_WARN, L"Checking transformer for media type '%d' on '%s'.\n", i, pwszDeviceSymbolicLink);

        hr = MFTEnum(
            MFT_CATEGORY_VIDEO_PROCESSOR, // Process from input to output type
            0,              // Reserved
            &processorInputInfo,     // Input type
            &processorOutputInfo,    // Output type
            nullptr,        // Reserved
            &pMFTCLSIDs,
            &MFTCLSIDsCount
            );
        CHECK_FAILED_HR_WITH_GOTO_AND_EX_STR(hr, done, exWhatString, "Error occurred during MFTEnum().");

        // We found a processor
        if (MFTCLSIDsCount > 0)
        {
            _RPTFW2(_CRT_WARN, L"Found transformer for media type '%d' on '%s'.\n", i, pwszDeviceSymbolicLink);
            break;
        }

        // Free for the next iteration, in case of jump to `done`, a free will be performed there too
        SafeRelease(&pSourceOutputMediaType);
    }

    if (MFTCLSIDsCount == 0)
    {
        hr = E_UNEXPECTED;
        CHECK_FAILED_HR_WITH_GOTO_AND_EX_STR(hr, done, exWhatString, "Could not find proper video processor.");
    }

    // Create the processor
    hr = CoCreateInstance(pMFTCLSIDs[0], nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&m_pProcessor));
    CHECK_FAILED_HR_WITH_GOTO_AND_EX_STR(hr, done, exWhatString, "Error occurred while creating video processor using CoCreateInstance().");

    _RPTFW1(_CRT_WARN, L"MFT (Processor) created for '%s'.\n", pwszDeviceSymbolicLink);

    // Set the media type for the processor
    try
    {
        SetVideoProcessorOutputForInputMediaType(m_pProcessor, pSourceOutputMediaType, OUTPUT_VIDEO_SUBTYPE, /*OUT*/ pProcessorOutputMediaType);
    }
    catch (const std::system_error &ex)
    {
        hr = ex.code().value();

        exWhatString = std::string{ MAKE_EX_STR("Error occurred while preparing the video processor for the media types.") }
            + "\nWith Error: " + ex.what() + " (" + std::to_string(ex.code().value()) + ")";

        goto done;
    }

    _RPTFW1(_CRT_WARN, L"Get frame width and height for '%s'.\n", pwszDeviceSymbolicLink);

    // Get the DefaultStride, Width, Height for the frames
    try
    {
        GetWidthHeightDefaultStrideForMediaType(pProcessorOutputMediaType, &m_lSrcDefaultStride, &m_frameWidth, &m_frameHeight);

        _RPTFW4(_CRT_WARN, L"Dimensions are w(%d) x h(%d) with stride(%d) on '%s'.\n", m_frameWidth, m_frameHeight, m_lSrcDefaultStride, pwszDeviceSymbolicLink);
    }
    catch (const std::system_error &ex)
    {
        hr = ex.code().value();

        exWhatString = std::string{ MAKE_EX_STR("Error occurred during retrieving Width, Height, and DefualtStride for media type.") }
        + "\nWith Error: " + ex.what() + " (" + std::to_string(ex.code().value()) + ")";

        goto done;
    }

    _RPTFW1(_CRT_WARN, L"Create frame buffer for '%s'.\n", pwszDeviceSymbolicLink);

    // Create the buffer for the frames
    try
    {
        m_frameBuffer = std::make_unique<BYTE[]>(static_cast<size_t>(m_frameWidth) * static_cast<size_t>(m_frameHeight) * OUTPUT_BYTES_PER_PIXEL);
    }
    catch (const std::bad_alloc &/*ex*/)
    {
        exWhatString = MAKE_EX_STR("Error occurred while allocating memory for the frame buffer.");
        hr = E_OUTOFMEMORY;
        goto done;
    }

    // Save the symbolic link
    m_wstrDeviceSymbolicLink = std::wstring{ pwszDeviceSymbolicLink };

    // Set the capture device change notification handler
    AddCaptureDeviceChangeNotificationHandler(m_wstrDeviceSymbolicLink, &m_pDeviceChangeNotifHandler);

    m_bIsInitialized = true;
    m_bIsAvailable = true;

    _RPTFW1(_CRT_WARN, L"Initialization completed for '%s'.\n", pwszDeviceSymbolicLink);

done:
    CoTaskMemFree(pMFTCLSIDs);
    SafeRelease(&pSourceOutputMediaType);
    SafeRelease(&pProcessorOutputMediaType);
    SafeRelease(&pAttributes);
    if (FAILED(hr)) { FreeResources(); }

    LeaveCriticalSection(&m_criticalSection);

    _RPTF1(_CRT_WARN, "Left critical section in %s.\n", STRINGIZE(InitializeForDevice));

    if (FAILED(hr))
    {
        throw std::system_error{ hr, std::system_category(), exWhatString };
    }
}

// ==============================
// ====== Static Functions ======
// ==============================

// --------------------------------------------------------------------
// SetVideoProcessorInputAndOuputMediaTypes [static]
// --------------------------------------------------------------------

void CSourceReader::SetVideoProcessorOutputForInputMediaType(
    IMFTransform *pProcessor,
    IMFMediaType *pInputMediaType,
    const GUID guidOutputVideoSubtype,
    IMFMediaType *&pOutputMediaType
    )
{
    assert(pProcessor != nullptr);
    assert(pInputMediaType != nullptr);
    assert(guidOutputVideoSubtype != GUID_NULL);

    _RPTF0(_CRT_WARN, "CSourceReader::SetVideoProcessorOutputForInputMediaType() called.\n");

    HRESULT hr{ S_OK };
    std::string exWhatString{ };
    GUID guidIndexVideoSubtype{ GUID_NULL };

    IMFMediaType *pPartialProcessorOutputMediaType{ nullptr };
    IMFMediaType *pCompleteProcessorOutputeMediaType{ nullptr };

    // Storing data about the input type.
    UINT32  inputInterlaceMode{ 0 };
    LONG    inputDefaultStride{ 0 };
    UINT32  inputWidth{ 0 };
    UINT32  inputHeight{ 0 };
    UINT    cbInputFrame{ 0 };
    MFRatio inputFrameRate{};
    MFRatio inputPar{};

    // Set the input type for the first stream
    hr = pProcessor->SetInputType(0, pInputMediaType, 0);
    CHECK_FAILED_HR_WITH_GOTO_AND_EX_STR(hr, done, exWhatString, "Error occurred during IMFTransform::SetInputType().");

    //Loop for the output type
    for (DWORD dwTypeIndex = 0; ; dwTypeIndex++)
    {
        hr = pProcessor->GetOutputAvailableType(0, dwTypeIndex, &pPartialProcessorOutputMediaType);
        CHECK_FAILED_HR_WITH_GOTO_AND_EX_STR(hr, done, exWhatString, "Error occurred during IMFTransform::GetOutputAvailableType().");

        hr = pPartialProcessorOutputMediaType->GetGUID(MF_MT_SUBTYPE, &guidIndexVideoSubtype);
        CHECK_FAILED_HR_WITH_GOTO_AND_EX_STR(hr, done, exWhatString, "Error occurred during IMFMediaType::GetGUID().");

        _RPTF1(_CRT_WARN, "Check type matching to process input to output, output type no. '%d'.\n", dwTypeIndex);

        if (guidIndexVideoSubtype == guidOutputVideoSubtype)
        {
            _RPTF1(_CRT_WARN, "Found the requested output, on type no. '%d'.\n", dwTypeIndex);
            break;
        }

        // Release for the next iteration
        SafeRelease(&pPartialProcessorOutputMediaType);
    }

    if (!pPartialProcessorOutputMediaType)
    {
        hr = E_UNEXPECTED;
        CHECK_FAILED_HR_WITH_GOTO_AND_EX_STR(hr, done, exWhatString, "The input media type cannot be processed into a suitable output type.");
    }

    // Create complete media type from what usually would be a partial media type returned from the processor, this assumption is a HACK!
    // HACK: Here we assume the `guidOutputVideoSubtype` is for an uncompressed media type e.g. RGB32.
    //  as this method is private, this can sink in, but this note has to be written here.

    // Ref: https://learn.microsoft.com/en-us/windows/win32/medfound/uncompressed-video-media-types

    _RPTF0(_CRT_WARN, "Creating complete media type.\n");

    // Step1: Get the details from the input.

    hr = pInputMediaType->GetUINT32(MF_MT_INTERLACE_MODE, &inputInterlaceMode);
    CHECK_FAILED_HR_WITH_GOTO_AND_EX_STR(hr, done, exWhatString, "Error occurred during IMFMediaType::GetUINT32().");

    try
    {
        GetWidthHeightDefaultStrideForMediaType(pInputMediaType, &inputDefaultStride, &inputWidth, &inputHeight);
    }
    catch (const std::system_error &ex)
    {
        hr = ex.code().value();
        exWhatString = ex.what();
        goto done;
    }

    hr = MFCalculateImageSize(guidOutputVideoSubtype, inputWidth, inputHeight, &cbInputFrame);
    CHECK_FAILED_HR_WITH_GOTO_AND_EX_STR(hr, done, exWhatString, "Error occurred during MFCalculateImageSize().");

    hr = MFGetAttributeRatio(
        pInputMediaType,
        MF_MT_FRAME_RATE,
        reinterpret_cast<UINT32 *>(&inputFrameRate.Numerator),
        reinterpret_cast<UINT32 *>(&inputFrameRate.Denominator)
    );
    CHECK_FAILED_HR_WITH_GOTO_AND_EX_STR(hr, done, exWhatString, "Error occurred during MFGetAttributeRatio().");

    hr = MFGetAttributeRatio(
        pInputMediaType,
        MF_MT_PIXEL_ASPECT_RATIO,
        reinterpret_cast<UINT32 *>(&inputPar.Numerator),
        reinterpret_cast<UINT32 *>(&inputPar.Denominator)
    );
    CHECK_FAILED_HR_WITH_GOTO_AND_EX_STR(hr, done, exWhatString, "Error occurred during MFGetAttributeRatio().");

    // Step 2: Create the completed media for the output.

    hr = MFCreateMediaType(&pCompleteProcessorOutputeMediaType);
    CHECK_FAILED_HR_WITH_GOTO_AND_EX_STR(hr, done, exWhatString, "Error occurred during MFCreateMediaType().");

    hr = pCompleteProcessorOutputeMediaType->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Video); // HACK: We assume video.
    CHECK_FAILED_HR_WITH_GOTO_AND_EX_STR(hr, done, exWhatString, "Error occurred during IMFMediaType::SetGUID().");

    hr = pCompleteProcessorOutputeMediaType->SetGUID(MF_MT_SUBTYPE, guidOutputVideoSubtype);
    CHECK_FAILED_HR_WITH_GOTO_AND_EX_STR(hr, done, exWhatString, "Error occurred during IMFMediaType::SetGUID().");

    hr = pCompleteProcessorOutputeMediaType->SetUINT32(MF_MT_INTERLACE_MODE, inputInterlaceMode);
    CHECK_FAILED_HR_WITH_GOTO_AND_EX_STR(hr, done, exWhatString, "Error occurred during IMFMediaType::SetUINT32().");

    hr = pCompleteProcessorOutputeMediaType->SetUINT32(MF_MT_DEFAULT_STRIDE, inputDefaultStride);
    CHECK_FAILED_HR_WITH_GOTO_AND_EX_STR(hr, done, exWhatString, "Error occurred during IMFMediaType::SetUINT32().");

    hr = MFSetAttributeSize(pCompleteProcessorOutputeMediaType, MF_MT_FRAME_SIZE, inputWidth, inputHeight);
    CHECK_FAILED_HR_WITH_GOTO_AND_EX_STR(hr, done, exWhatString, "Error occurred during MFSetAttributeSize().");

    hr = pCompleteProcessorOutputeMediaType->SetUINT32(MF_MT_SAMPLE_SIZE, cbInputFrame);
    CHECK_FAILED_HR_WITH_GOTO_AND_EX_STR(hr, done, exWhatString, "Error occurred during IMFMediaType::SetUINT32().");

    hr = pCompleteProcessorOutputeMediaType->SetUINT32(MF_MT_FIXED_SIZE_SAMPLES, TRUE);
    CHECK_FAILED_HR_WITH_GOTO_AND_EX_STR(hr, done, exWhatString, "Error occurred during IMFMediaType::SetUINT32().");

    hr = pCompleteProcessorOutputeMediaType->SetUINT32(MF_MT_ALL_SAMPLES_INDEPENDENT, TRUE);
    CHECK_FAILED_HR_WITH_GOTO_AND_EX_STR(hr, done, exWhatString, "Error occurred during IMFMediaType::SetUINT32().");

    hr = MFSetAttributeRatio(pCompleteProcessorOutputeMediaType, MF_MT_FRAME_RATE, inputFrameRate.Numerator, inputFrameRate.Denominator);
    CHECK_FAILED_HR_WITH_GOTO_AND_EX_STR(hr, done, exWhatString, "Error occurred during MFSetAttributeRatio().");

    hr = MFSetAttributeRatio(pCompleteProcessorOutputeMediaType, MF_MT_PIXEL_ASPECT_RATIO, inputPar.Numerator, inputPar.Denominator);
    CHECK_FAILED_HR_WITH_GOTO_AND_EX_STR(hr, done, exWhatString, "Error occurred during MFSetAttributeRatio().");

    // Set the output type
    hr = pProcessor->SetOutputType(0, pCompleteProcessorOutputeMediaType, 0);
    CHECK_FAILED_HR_WITH_GOTO_AND_EX_STR(hr, done, exWhatString, "Error occurred during IMFTransform::SetOutputType().");

    // Set the output param
    pOutputMediaType = pCompleteProcessorOutputeMediaType;
    pOutputMediaType->AddRef();

done:
    SafeRelease(&pPartialProcessorOutputMediaType);
    SafeRelease(&pCompleteProcessorOutputeMediaType);

    if (FAILED(hr))
    {
        throw std::system_error{ hr, std::system_category(), exWhatString };
    }
}

// --------------------------------------------------------------------
// GetWidthHeightDefaultStrideForMediaType [static]
// --------------------------------------------------------------------

void CSourceReader::GetWidthHeightDefaultStrideForMediaType(
    IMFMediaType *pMediaType,
    LONG *plDefaultStride,
    UINT32 *pWidth,
    UINT32 *pHeight
    ) noexcept(false)
{
    // As a note here for using `assert`:
    //  We use asserts when we are dealing with private code, not API code,
    //  which we want to check the integrity during development,
    //  but no unexpected entries are going to be passed to the function
    //  from external calls.

    assert(pMediaType != nullptr);
    assert(plDefaultStride != nullptr);
    assert(pWidth != nullptr);
    assert(pHeight != nullptr);

    HRESULT hr{ S_OK };
    std::string exWhatString{ };

    GUID mediaSubtype{ GUID_NULL };

    LONG lStride{ 0 };
    UINT32 width{ 0 };
    UINT32 height{ 0 };

    // Get the width and height for the frame
    hr = MFGetAttributeSize(pMediaType, MF_MT_FRAME_SIZE, &width, &height);
    CHECK_FAILED_HR_WITH_GOTO_AND_EX_STR(hr, done, exWhatString, "Error occurred during MFGetAttributeSize().");

    // Try get the default stride
    // WARN: we are using reinterpret_cast here, the docs state:
    //  "The attribute value is stored as a UINT32,
    //   but should be cast to a 32-bit signed integer value. Stride can be negative"
    //  from: https://docs.microsoft.com/en-us/windows/win32/medfound/mf-mt-default-stride-attribute
    hr = pMediaType->GetUINT32(MF_MT_DEFAULT_STRIDE, reinterpret_cast<UINT32 *>(&lStride));
    if (FAILED(hr))
    {
        // Try to calculate the default stride.
        hr = pMediaType->GetGUID(MF_MT_SUBTYPE, &mediaSubtype);
        CHECK_FAILED_HR_WITH_GOTO_AND_EX_STR(hr, done, exWhatString, "Error occurred during IMFMediaType::GetGUID().");

        hr = MFGetStrideForBitmapInfoHeader(mediaSubtype.Data1, width, &lStride);
        CHECK_FAILED_HR_WITH_GOTO_AND_EX_STR(hr, done, exWhatString, "Error occurred during MFGetStrideForBitmapInfoHeader().");

        // Set the attribute for later reference if needed
        (void)pMediaType->SetUINT32(MF_MT_DEFAULT_STRIDE, static_cast<UINT32>(lStride));
    }

    *plDefaultStride = lStride;
    *pWidth = width;
    *pHeight = height;

done:
    if (FAILED(hr))
    {
        throw std::system_error{ hr, std::system_category(), exWhatString };
    }
}

#pragma managed(pop)
