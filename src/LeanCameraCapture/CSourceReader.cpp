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

// ---------------------------------------------------
// OnReadSample
// ---------------------------------------------------

HRESULT CSourceReader::OnReadSample(
    HRESULT hrStatus,
    DWORD /*dwStreamIndex*/,
    DWORD /*dwStreamFlags*/,
    LONGLONG /*llTimestamp*/,
    IMFSample *pSample
    )
{
    HRESULT hr{ S_OK };
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

// ---------------------------------------------------
// FreeResources
// ---------------------------------------------------

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

// ---------------------------------------------------
// InitializeForDevice
//
// Here we initialize this instance of CSourceReader
// to device that exposes IMFActivate. This can be
// done only once per instance.
// ---------------------------------------------------

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
    GUID outputSubtype{ 0 };

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

    // Read the first sample
    hr = m_pSourceReader->ReadSample(
        (DWORD)MF_SOURCE_READER_FIRST_VIDEO_STREAM,
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
