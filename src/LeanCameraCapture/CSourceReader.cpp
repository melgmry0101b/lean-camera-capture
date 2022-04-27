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

    EnterCriticalSection(&m_criticalSection);

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

            // Copy the frame
            // Note that here we are multiplying with 4 as we are using RGB32 (4 byte) format
            hr = MFCopyImage(m_frameBuffer.get(), m_frameWidth, pbScanline0, lStride, m_frameWidth * 4, m_frameHeight);
            CHECK_FAILED_HR_WITH_GOTO_AND_EX_STR(hr, done, exWhatString, "Error occurred during MFCopyImage().");

            if (m_pReadSampleSuccessCallback)
            {
                (*m_pReadSampleSuccessCallback)(m_frameBuffer.get(), m_frameWidth, m_frameHeight, 4);
            }
        }
    }

done:
    SafeRelease(&pOutputSample);
    SafeRelease(&pBuffer);

    if (FAILED(hr))
    {
        if (m_pReadSampleFailCallback)
        {
            (*m_pReadSampleFailCallback)(hr, exWhatString);
        }
    }

    LeaveCriticalSection(&m_criticalSection);

    return hr;
}

// =========================
// ====== Constructor ======
// =========================

CSourceReader::CSourceReader() :
    m_nRefCount{ 1 },
    m_criticalSection{},
    m_pDevice{ nullptr },
    m_pSourceReader{ nullptr },
    m_pProcessor{ nullptr },
    m_lSrcDefaultStride{ 0 },
    m_frameWidth{ 0 },
    m_frameHeight{ 0 },
    m_frameBuffer{ nullptr },
    m_pwszSymbolicLink{ nullptr },
    m_cchSymbolicLink{ 0 },
    m_pReadSampleSuccessCallback{ nullptr },
    m_pReadSampleFailCallback{ nullptr }
{
    InitializeCriticalSection(&m_criticalSection);
}

// ========================
// ====== Destructor ======
// ========================

CSourceReader::~CSourceReader()
{
    FreeResources();
    DeleteCriticalSection(&m_criticalSection);
}

// ===============================
// ====== Private Functions ======
// ===============================

// --------------------------------------------------------------------
// FreeResources
// --------------------------------------------------------------------

void CSourceReader::FreeResources()
{
    EnterCriticalSection(&m_criticalSection);

    SafeRelease(&m_pProcessor);
    SafeRelease(&m_pSourceReader);
    SafeRelease(&m_pDevice);

    CoTaskMemFree(m_pwszSymbolicLink);
    m_pwszSymbolicLink = nullptr;
    m_cchSymbolicLink = 0;

    LeaveCriticalSection(&m_criticalSection);
}

// --------------------------------------------------------------------
// ProcessorProcessOutput
// --------------------------------------------------------------------

void CSourceReader::ProcessorProcessOutput(
    DWORD dwOutputStreamID,
    IMFSample **ppOutputSample
    )
{
    HRESULT hr{ S_OK };
    std::string exWhatString{ };

    DWORD dwProcessOutputStatus{ 0 };
    MFT_OUTPUT_STREAM_INFO outputStreamInfo{ 0 };
    MFT_OUTPUT_DATA_BUFFER outputDataBuffer{ 0 };

    IMFMediaBuffer *pOutputBuffer{ nullptr };
    IMFSample *pOutputSample{ nullptr };

    hr = m_pProcessor->GetOutputStreamInfo(dwOutputStreamID, &outputStreamInfo);
    CHECK_FAILED_HR_WITH_GOTO_AND_EX_STR(hr, done, exWhatString, "Error occurred during IMFTransform::GetOutputStreamInfo().");

    // Check if the sample should be allocated by us
    if ((outputStreamInfo.dwFlags & (MFT_OUTPUT_STREAM_PROVIDES_SAMPLES | MFT_OUTPUT_STREAM_CAN_PROVIDE_SAMPLES))
        != (MFT_OUTPUT_STREAM_PROVIDES_SAMPLES | MFT_OUTPUT_STREAM_CAN_PROVIDE_SAMPLES))
    {
        // Create buffer
        hr = MFCreateAlignedMemoryBuffer(outputStreamInfo.cbSize, outputStreamInfo.cbAlignment, &pOutputBuffer);
        CHECK_FAILED_HR_WITH_GOTO_AND_EX_STR(hr, done, exWhatString, "Error occurred during MFCreateAlignedMemoryBuffer().");

        // Create the output sample
        hr = MFCreateSample(&pOutputSample);
        CHECK_FAILED_HR_WITH_GOTO_AND_EX_STR(hr, done, exWhatString, "Error occurred during MFCreateSample().");

        // Add buffer to the sample
        hr = pOutputSample->AddBuffer(pOutputBuffer);
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
    if (ppOutputSample
        && ((outputDataBuffer.dwStatus & MFT_OUTPUT_DATA_BUFFER_NO_SAMPLE) != MFT_OUTPUT_DATA_BUFFER_NO_SAMPLE))
    {
        *ppOutputSample = pOutputSample;
    }
    else
    {
        // If the calling function isn't receiving the sample or the buffer isn't set, release it!
        SafeRelease(&pOutputSample);
    }

done:
    SafeRelease(&pOutputBuffer);

    if (FAILED(hr))
    {
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
    }
    else
    {
        // If the calling method isn't accept the output, release the sample
        SafeRelease(&pOutputSample);
    }

finalizeAndDrain:
    hr = m_pProcessor->ProcessMessage(MFT_MESSAGE_NOTIFY_END_OF_STREAM, 0);
    if (FAILED(hr))
    {
        if (exWhatString.empty())
        {
            exWhatString = MAKE_EX_STR("Error occurred during IMFTransform::ProcessMessage().");
        }
        else
        {
            exWhatString = std::string{ MAKE_EX_STR("Error occurred during IMFTransform::ProcessMessage().") }
                + "\nWith Error: " + exWhatString;
        }

        goto done;
    }

    hr = m_pProcessor->ProcessMessage(MFT_MESSAGE_COMMAND_DRAIN, 0);
    if (FAILED(hr))
    {
        if (exWhatString.empty())
        {
            exWhatString = MAKE_EX_STR("Error occurred during IMFTransform::ProcessMessage().");
        }
        else
        {
            exWhatString = std::string{ MAKE_EX_STR("Error occurred during IMFTransform::ProcessMessage().") }
            + "\nWith Error: " + exWhatString;
        }

        goto done;
    }

    hr = S_OK;
    while (true)
    {
        try
        {
            ProcessorProcessOutput(dwStreamID, nullptr);
        }
        catch (const std::system_error &ex)
        {
            hr = ex.code().value();
            if (hr == MF_E_TRANSFORM_NEED_MORE_INPUT)
            {
                // Draining the MFT succeeded
                hr = S_OK;
            }
            else
            {
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
            }
            goto done;
        }
    }

done:
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
    if (!m_pSourceReader)
    {
        throw std::logic_error{ "Instance's source reader is null." };
    }

    if (!GetIsMediaFoundationStarted)
    {
        throw std::logic_error{ "Media Foundation hasn't started." };
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

void CSourceReader::InitializeForDevice(IMFActivate *pActivate) noexcept(false)
{
    if (!pActivate)
    {
        throw std::logic_error{ "pActivate is null." };
    }

    if (!GetIsMediaFoundationStarted())
    {
        throw std::logic_error{ "Media foundation hasn't started." };
    }

    // This method should be called only once
    if (m_pDevice)
    {
        throw std::logic_error{ "This instance of CSourceReader is already initialized for a device." };
    }

    // Copy reference for the IMFActivate associated with the capture device
    //  and AddRef to the COM object.
    m_pDevice = pActivate;
    pActivate->AddRef();

    HRESULT hr{ S_OK };
    std::string exWhatString{};

    IMFMediaSource  *pMediaSource{ nullptr };
    IMFAttributes   *pAttributes{ nullptr };
    IMFMediaType    *pMediaType{ nullptr };

    CLSID *pMFTCLSIDs{ nullptr };
    UINT32 MFTCLSIDsCount{ 0 };

    MFT_REGISTER_TYPE_INFO inputInfo{ 0 };
    MFT_REGISTER_TYPE_INFO outputInfo{ 0 };
    GUID outputSubtype{ GUID_NULL };

    EnterCriticalSection(&m_criticalSection);

    // Create the media source for the device
    hr = m_pDevice->ActivateObject(IID_PPV_ARGS(&pMediaSource));
    CHECK_FAILED_HR_WITH_GOTO_AND_EX_STR(hr, done, exWhatString, "Error occurred during IMFActivate::ActivateObject().");

    // Get the symbolic link
    hr = m_pDevice->GetAllocatedString(
        MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE_VIDCAP_SYMBOLIC_LINK,
        &m_pwszSymbolicLink,
        &m_cchSymbolicLink
        );
    CHECK_FAILED_HR_WITH_GOTO_AND_EX_STR(hr, done, exWhatString, "Error occurred during IMFActivate::GetAllocatedString().");

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
        pMediaSource,
        pAttributes,
        &m_pSourceReader
        );
    CHECK_FAILED_HR_WITH_GOTO_AND_EX_STR(hr, done, exWhatString, "Error occurred during MFCreateSourceReaderFromMediaSource().");

    // ---
    // --- Find the suitable codec for the video to RGB32
    // ---

    inputInfo.guidMajorType = MFMediaType_Video;
    
    outputInfo.guidMajorType = MFMediaType_Video;
    outputInfo.guidSubtype = MFVideoFormat_RGB32; // Our output type is RGB32

    // Loop through the available output types in the source reader and check
    for (DWORD i = 0; ; i++)
    {
        hr = m_pSourceReader->GetNativeMediaType(
            static_cast<DWORD>(MF_SOURCE_READER_FIRST_VIDEO_STREAM),
            i,
            &pMediaType
            );
        CHECK_FAILED_HR_WITH_GOTO_AND_EX_STR(hr, done, exWhatString, "Could not find suitable codec converting into RGB32. IMFSourceReader::GetNativeMediaType().");

        hr = pMediaType->GetGUID(MF_MT_SUBTYPE, &outputSubtype);
        CHECK_FAILED_HR_WITH_GOTO_AND_EX_STR(hr, done, exWhatString, "Error occurred during IMFMediaType::GetGUID().");

        inputInfo.guidSubtype = outputSubtype;

        hr = MFTEnum(
            MFT_CATEGORY_VIDEO_PROCESSOR, // Process from input to output type
            0,              // Reserved
            &inputInfo,     // Input type
            &outputInfo,    // Output type
            nullptr,        // Reserved
            &pMFTCLSIDs,
            &MFTCLSIDsCount
            );
        CHECK_FAILED_HR_WITH_GOTO_AND_EX_STR(hr, done, exWhatString, "Error occurred during MFTEnum().");

        // We found a processor
        if (MFTCLSIDsCount > 0) { break; }

        // Free for the next iteration, in case of jump to `done`, a free will be performed there too
        SafeRelease(&pMediaType);
    }

    // Create the processor
    hr = CoCreateInstance(pMFTCLSIDs[0], nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&m_pProcessor));
    CHECK_FAILED_HR_WITH_GOTO_AND_EX_STR(hr, done, exWhatString, "Error occurred while creating video processor using CoCreateInstance().");

    // Set the media type for the processor
    try
    {
        SetVideoProcessorOutputForInputMediaType(m_pProcessor, pMediaType, MFVideoFormat_RGB32);
    }
    catch (const std::system_error &ex)
    {
        hr = ex.code().value();

        exWhatString = std::string{ MAKE_EX_STR("Error occurred while preparing the video processor for the media types.") }
            + "\nWith Error: " + ex.what() + " (" + std::to_string(ex.code().value()) + ")";

        goto done;
    }

    // Get the DefaultStride, Width, Height for the frames
    try
    {
        GetWidthHeightDefaultStrideForMediaType(pMediaType, &m_lSrcDefaultStride, &m_frameWidth, &m_frameHeight);
    }
    catch (const std::system_error &ex)
    {
        hr = ex.code().value();

        exWhatString = std::string{ MAKE_EX_STR("Error occurred during retrieving Width, Height, and DefualtStride for media type.") }
        + "\nWith Error: " + ex.what() + " (" + std::to_string(ex.code().value()) + ")";

        goto done;
    }

    // Create the buffer for the frames
    try
    {
        m_frameBuffer = std::make_unique<BYTE[]>(m_frameWidth * m_frameHeight);
    }
    catch (const std::bad_alloc &/*ex*/)
    {
        exWhatString = MAKE_EX_STR("Error occurred while allocating memory for the frame buffer.");
        hr = E_OUTOFMEMORY;
        goto done;
    }

done:
    if (FAILED(hr) && pMediaSource) { pMediaSource->Shutdown(); }

    CoTaskMemFree(pMFTCLSIDs);
    SafeRelease(&pMediaType);
    SafeRelease(&pAttributes);
    SafeRelease(&pMediaSource);
    if (FAILED(hr)) { FreeResources(); }

    LeaveCriticalSection(&m_criticalSection);

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
    const GUID guidOutputVideoSubtype
    )
{
    assert(pProcessor != nullptr);
    assert(pInputMediaType != nullptr);
    assert(guidOutputVideoSubtype != GUID_NULL);

    HRESULT hr{ S_OK };
    std::string exWhatString{ };
    GUID guidIndexVideoSubtype{ GUID_NULL };

    IMFMediaType *pOutputMediaType{ nullptr };

    // Set the input type for the first stream
    hr = pProcessor->SetInputType(0, pInputMediaType, 0);
    CHECK_FAILED_HR_WITH_GOTO_AND_EX_STR(hr, done, exWhatString, "Error occurred during IMFTransform::SetInputType().");

    //Loop for the output type
    for (DWORD dwTypeIndex = 0; ; dwTypeIndex++)
    {
        hr = pProcessor->GetOutputAvailableType(0, dwTypeIndex, &pOutputMediaType);
        CHECK_FAILED_HR_WITH_GOTO_AND_EX_STR(hr, done, exWhatString, "Error occurred during IMFTransform::GetOutputAvailableType().");

        hr = pOutputMediaType->GetGUID(MF_MT_SUBTYPE, &guidIndexVideoSubtype);
        CHECK_FAILED_HR_WITH_GOTO_AND_EX_STR(hr, done, exWhatString, "Error occurred during IMFMediaType::GetGUID().");

        if (guidIndexVideoSubtype == guidOutputVideoSubtype) { break; }

        // Release for the next iteration
        SafeRelease(&pOutputMediaType);
    }

    // Set the output type
    hr = pProcessor->SetOutputType(0, pOutputMediaType, 0);
    CHECK_FAILED_HR_WITH_GOTO_AND_EX_STR(hr, done, exWhatString, "Error occurred during IMFTransform::SetOutputType().");

done:
    SafeRelease(&pOutputMediaType);

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
