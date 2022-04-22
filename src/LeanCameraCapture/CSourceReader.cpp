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

    IMFMediaBuffer *pBuffer{ nullptr };

    EnterCriticalSection(&m_criticalSection);

    // Check if hr is failed
    if (FAILED(hr)) { goto done; }

    // Read from the sample if available
    if (pSample)
    {
        hr = pSample->GetBufferByIndex(0, &pBuffer);
        if (FAILED(hr)) { goto done; }


    }

    // Request the next frame
    hr = m_pSourceReader->ReadSample(
        static_cast<DWORD>(MF_SOURCE_READER_FIRST_VIDEO_STREAM),
        0,
        nullptr,
        nullptr,
        nullptr,
        nullptr
        );
    if (FAILED(hr)) { goto done; }

done:
    SafeRelease(&pBuffer);

    if (FAILED(hr))
    {
        // TODO: Call error callback
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
    m_pwszSymbolicLink{ nullptr },
    m_cchSymbolicLink{ 0 }
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
// GetWidthHeightDefaultStrideForMediaType [static]
// --------------------------------------------------------------------

HRESULT CSourceReader::GetWidthHeightDefaultStrideForMediaType(
    IMFMediaType *pMediaType,
    LONG *plDefaultStride,
    UINT32 *pWidth,
    UINT32 *pHeight
    )
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
    GUID mediaSubtype{ GUID_NULL };

    LONG lStride{ 0 };
    UINT32 width{ 0 };
    UINT32 height{ 0 };

    // Get the width and height for the frame
    hr = MFGetAttributeSize(pMediaType, MF_MT_FRAME_SIZE, &width, &height);
    if (FAILED(hr)) { goto done; }

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
        if (FAILED(hr)) { goto done; }

        hr = MFGetStrideForBitmapInfoHeader(mediaSubtype.Data1, width, &lStride);
        if (FAILED(hr)) { goto done; }

        // Set the attribute for later reference if needed
        (void)pMediaType->SetUINT32(MF_MT_DEFAULT_STRIDE, static_cast<UINT32>(lStride));
    }

    *plDefaultStride = lStride;
    *pWidth = width;
    *pHeight = height;

done:
    return hr;
}

// ==============================
// ====== Public Functions ======
// ==============================

// --------------------------------------------------------------------
// InitializeForDevice
//
// Here we initialize this instance of CSourceReader
//  to device that exposes IMFActivate. This can be
//  done only once per instance.
// --------------------------------------------------------------------

void CSourceReader::InitializeForDevice(IMFActivate *pActivate) noexcept(false)
{
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
    if (FAILED(hr))
    {
        exWhatString = "Error occurred during IMFActivate::ActivateObject().";
        goto done;
    }

    // Get the symbolic link
    hr = m_pDevice->GetAllocatedString(
        MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE_VIDCAP_SYMBOLIC_LINK,
        &m_pwszSymbolicLink,
        &m_cchSymbolicLink
        );
    if (FAILED(hr))
    {
        exWhatString = "Error occurred during IMFActivate::GetAllocatedString().";
        goto done;
    }

    // ---
    // --- Create the source reader
    // ---
    
    // Create attributes to hold settings with 2 settings' slots
    hr = MFCreateAttributes(&pAttributes, 2);
    if (FAILED(hr))
    {
        exWhatString = "Error occurred during MFCreateAttributes().";
        goto done;
    }
    hr = pAttributes->SetUINT32(MF_READWRITE_DISABLE_CONVERTERS, true);
    if (FAILED(hr))
    {
        exWhatString = "Error occurred during IMFAttributes::SetUINT32().";
        goto done;
    }

    // Set the an attribute slot for this class instance as callback for events e.g. OnReadSample
    hr = pAttributes->SetUnknown(
        MF_SOURCE_READER_ASYNC_CALLBACK,
        this
        );
    if (FAILED(hr))
    {
        exWhatString = "Error occurred during IMFAttributes::SetUnknown().";
        goto done;
    }

    // Create source reader for the media source using the attributes
    hr = MFCreateSourceReaderFromMediaSource(
        pMediaSource,
        pAttributes,
        &m_pSourceReader
        );
    if (FAILED(hr))
    {
        exWhatString = "Error occurred during MFCreateSourceReaderFromMediaSource().";
        goto done;
    }

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
            (DWORD)MF_SOURCE_READER_FIRST_VIDEO_STREAM,
            i,
            &pMediaType
            );
        if (FAILED(hr))
        {
            exWhatString = "Could not find suitable codec converting into RGB32. IMFSourceReader::GetNativeMediaType().";
            goto done;
        }

        hr = pMediaType->GetGUID(MF_MT_SUBTYPE, &outputSubtype);
        if (FAILED(hr))
        {
            exWhatString = "Error occurred during IMFMediaType::GetGUID().";
            goto done;
        }

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
        if (FAILED(hr))
        {
            exWhatString = "Error occurred during MFTEnum().";
            goto done;
        }

        // We found a processor
        if (MFTCLSIDsCount > 0) { break; }

        // Free for the next iteration, in case of jump to `done`, a free will be performed there too
        SafeRelease(&pMediaType);
    }

    // Create the processor
    hr = CoCreateInstance(pMFTCLSIDs[0], nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&m_pProcessor));
    if (FAILED(hr))
    {
        exWhatString = "Error occurred while creating video processor using CoCreateInstance().";
        goto done;
    }

    // Get the DefaultStride, Width, Height for the frames
    hr = GetWidthHeightDefaultStrideForMediaType(pMediaType, &m_lImageDefaultStride, &m_imageWidth, &m_imageHeight);
    if (FAILED(hr))
    {
        exWhatString = "Error occurred during retrieving Width, Height, and DefualtStride for media type.";
        goto done;
    }

    // Read the first sample
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
        exWhatString = "Error occurred during IMFSourceReader::ReadSample().";
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
