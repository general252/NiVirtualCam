/*
    Copyright (C) 2013  Soroush Falahati - soroush@falahati.net

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see [http://www.gnu.org/licenses/].
    */

#pragma warning(disable:4244)
#pragma warning(disable:4711)
    //---------------------------------------------------------------------------
    // Includes
    //---------------------------------------------------------------------------
#include <streams.h>
#include <stdio.h>
#include <olectl.h>
#include <dvdmedia.h>
#include <math.h>
#include <direct.h>
#include <dos.h>
#include <math.h>
#include "NiVirtualCam.h"
#include "resource.h"
#include <windows.h>

#include "d3d9_screen_capture.h"

//Context g_context;
//DepthGenerator g_depth;
//DepthMetaData g_depthMD;

int m_iImageWidth = 1920;
int m_iImageHeight = 1080;
std::string buffer_image;
DX::D3D9ScreenCapture screenCapture;

//////////////////////////////////////////////////////////////////////////
//  CKCam is the source filter which masquerades as a capture device
//////////////////////////////////////////////////////////////////////////
CUnknown* WINAPI CKCam::CreateInstance(LPUNKNOWN lpunk, HRESULT* phr)
{
    ASSERT(phr);
    CUnknown* punk = new CKCam(lpunk, phr);
    return punk;
}

CKCam::CKCam(LPUNKNOWN lpunk, HRESULT* phr) :
    CSource(NAME("OpenNi Virtual Camera"), lpunk, CLSID_NiVirtualCam)
{
    ASSERT(phr);
    CAutoLock cAutoLock(&m_cStateLock);

    // Create the one and only output pin
    m_paStreams = (CSourceStream**)new CKCamStream * [1];
    m_paStreams[0] = new CKCamStream(phr, this, L"OpenNi Virtual Camera");
}

HRESULT CKCam::QueryInterface(REFIID riid, void** ppv)
{
    //Forward request for IAMStreamConfig & IKsPropertySet to the pin
    if (riid == _uuidof(IAMStreamConfig) || riid == _uuidof(IKsPropertySet))
        return m_paStreams[0]->QueryInterface(riid, ppv);
    return CSource::QueryInterface(riid, ppv);
}

//////////////////////////////////////////////////////////////////////////
// CKCamStream is the one and only output pin of CKCam which handles 
// all the stuff.
//////////////////////////////////////////////////////////////////////////
CKCamStream::CKCamStream(HRESULT* phr, CKCam* pParent, LPCWSTR pPinName) :
    CSourceStream(NAME("OpenNi Virtual Camera"), phr, pParent, pPinName), m_pParent(pParent)
{
    // Set the default media type as 640x480x24@30
    GetMediaType(8, &m_mt);
}

CKCamStream::~CKCamStream()
{
}

ULONG CKCamStream::Release()
{
    return GetOwner()->Release();
}

ULONG CKCamStream::AddRef()
{
    return GetOwner()->AddRef();
}

HRESULT CKCamStream::QueryInterface(REFIID riid, void** ppv)
{
    // Standard OLE stuff
    if (riid == _uuidof(IAMStreamConfig))
        *ppv = (IAMStreamConfig*)this;
    else if (riid == _uuidof(IKsPropertySet))
        *ppv = (IKsPropertySet*)this;
    else
        return CSourceStream::QueryInterface(riid, ppv);

    AddRef();
    return S_OK;
}


HRESULT CKCamStream::FillBuffer(IMediaSample* pms)
{
    // Updating frame data and frame rate
    REFERENCE_TIME rtNow;
    REFERENCE_TIME avgFrameTime = ((VIDEOINFOHEADER*)m_mt.pbFormat)->AvgTimePerFrame;
    rtNow = m_rtLastTime;
    m_rtLastTime += avgFrameTime;
    pms->SetTime(&rtNow, &m_rtLastTime);
    pms->SetSyncPoint(TRUE);

    // Validating and guessing selected resolution
    BYTE* pData;
    long lDataLen;
    pms->GetPointer(&pData);
    lDataLen = pms->GetSize();

    if (0)
    {
    }
    else
    {
        int  s = lDataLen;
        if (buffer_image.size() < s) {
            s = buffer_image.size();
        }

        memcpy(pData, buffer_image.data(), s);
    }


    return NOERROR;
} // FillBuffer


//
// Notify
// Ignore quality management messages sent from the downstream filter
STDMETHODIMP CKCamStream::Notify(IBaseFilter* pSender, Quality q)
{
    return E_NOTIMPL;
} // Notify

//////////////////////////////////////////////////////////////////////////
// This is called when the output format has been negotiated
//////////////////////////////////////////////////////////////////////////
HRESULT CKCamStream::SetMediaType(const CMediaType* pmt)
{
    DECLARE_PTR(VIDEOINFOHEADER, pvi, pmt->Format());
    HRESULT hr = CSourceStream::SetMediaType(pmt);
    return hr;
}

// See Directshow help topic for IAMStreamConfig for details on this method
HRESULT CKCamStream::GetMediaType(int iPosition, CMediaType* pmt)
{
    if (iPosition < 0) return E_INVALIDARG;
    if (iPosition > 16) return VFW_S_NO_MORE_ITEMS;

    if (iPosition == 0)
    {
        *pmt = m_mt;
        return S_OK;
    }

    VIDEOINFOHEADER* pvi = (VIDEOINFOHEADER*)(pmt->AllocFormatBuffer(sizeof(VIDEOINFOHEADER)));
    //DECLARE_PTR(VIDEOINFOHEADER, pvi, pmt->AllocFormatBuffer(sizeof(VIDEOINFOHEADER)));
    ZeroMemory(pvi, sizeof(VIDEOINFOHEADER));

    pvi->bmiHeader.biCompression = BI_RGB;
    pvi->bmiHeader.biBitCount = 24;
    pvi->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    pvi->bmiHeader.biWidth = m_iImageWidth; // 80 * iPosition;
    pvi->bmiHeader.biHeight = m_iImageHeight; // 60 * iPosition;
    pvi->bmiHeader.biPlanes = 1;
    pvi->bmiHeader.biSizeImage = GetBitmapSize(&pvi->bmiHeader);
    pvi->bmiHeader.biClrImportant = 0;

    pvi->AvgTimePerFrame = (1000000000 / 100) / 30; // 500000;

    SetRectEmpty(&(pvi->rcSource)); // we want the whole image area rendered.
    SetRectEmpty(&(pvi->rcTarget)); // no particular destination rectangle

    pmt->SetType(&MEDIATYPE_Video);
    pmt->SetFormatType(&FORMAT_VideoInfo);
    pmt->SetTemporalCompression(FALSE);

    // Work out the GUID for the subtype from the header info.
    const GUID SubTypeGUID = GetBitmapSubtype(&pvi->bmiHeader);
    pmt->SetSubtype(&SubTypeGUID);
    pmt->SetSampleSize(pvi->bmiHeader.biSizeImage);

    return NOERROR;
} // GetMediaType

// This method is called to see if a given output format is supported
HRESULT CKCamStream::CheckMediaType(const CMediaType* pMediaType)
{
    VIDEOINFOHEADER* pvi = (VIDEOINFOHEADER*)(pMediaType->Format());
    if (*pMediaType != m_mt) {
        return E_INVALIDARG;
    }

    return S_OK;
} // CheckMediaType

// This method is called after the pins are connected to allocate buffers to stream data
HRESULT CKCamStream::DecideBufferSize(IMemAllocator* pAlloc, ALLOCATOR_PROPERTIES* pProperties)
{
    CAutoLock cAutoLock(m_pFilter->pStateLock());
    HRESULT hr = NOERROR;

    VIDEOINFOHEADER* pvi = (VIDEOINFOHEADER*)m_mt.Format();
    pProperties->cBuffers = 1;
    pProperties->cbBuffer = pvi->bmiHeader.biSizeImage;

    ALLOCATOR_PROPERTIES Actual;
    hr = pAlloc->SetProperties(pProperties, &Actual);

    if (FAILED(hr)) {
        return hr;
    }

    if (Actual.cbBuffer < pProperties->cbBuffer) {
        return E_FAIL;
    }

    return NOERROR;
} // DecideBufferSize


#include <process.h>

unsigned __stdcall my_StartAddress(void*)
{

    if (screenCapture.Init()) {
        // MessageBox(nullptr, L"screenCapture init success", L"screenCapture", MB_OK);
    }
    else {
        MessageBox(nullptr,
            L"screenCapture init fail",
            L"screenCapture", MB_ICONSTOP);
        return -1;
    }

    while (true)
    {
        Sleep(1000);


        DX::Image image;
        if (screenCapture.Capture(image)) {
            // MessageBoxA(NULL, "Capture success", "", MB_OK);
            int s = buffer_image.size();
            if (s != 1920 * 1080 * 4) {
                MessageBoxA(NULL, "Capture size fail", "", MB_OK);
                continue;
            }

            if (image.bgra.size() < s) {
                s = image.bgra.size();
            }

            uint8_t* src_argb = &image.bgra[0];
            uint8_t* dst_rgb = (uint8_t*)buffer_image.data();

            for (int x = 0; x < 1920*1080; ++x) {
                uint8_t b = src_argb[0];
                uint8_t g = src_argb[1];
                uint8_t r = src_argb[2];
                dst_rgb[0] = r;
                dst_rgb[1] = g;
                dst_rgb[2] = b;// (b + r) / 2;
                dst_rgb += 3;
                src_argb += 4;
            }

            //memcpy((char*)buffer_image.data(), image.bgra.data(), s);
        }
        else {
            MessageBoxA(NULL, "Capture fail", "", MB_OK);
        }
    }

    return 0;
}

// Called when graph is run
HRESULT CKCamStream::OnThreadCreate()
{
    m_rtLastTime = 0;
    buffer_image.resize(1920 * 1080 * 4);

    _beginthreadex(NULL, 0, my_StartAddress, NULL, 0, NULL);

    return NOERROR;
} // OnThreadCreate


//////////////////////////////////////////////////////////////////////////
//  IAMStreamConfig
//////////////////////////////////////////////////////////////////////////

HRESULT STDMETHODCALLTYPE CKCamStream::SetFormat(AM_MEDIA_TYPE* pmt)
{
    DECLARE_PTR(VIDEOINFOHEADER, pvi, m_mt.pbFormat);
    if (!pmt) {
        return VFW_E_INVALIDMEDIATYPE;
    }

    m_mt = *pmt;
    IPin* pin;
    ConnectedTo(&pin);

    if (pin) {
        IFilterGraph* pGraph = m_pParent->GetGraph();
        pGraph->Reconnect(this);
    }
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CKCamStream::GetFormat(AM_MEDIA_TYPE** ppmt)
{
    *ppmt = CreateMediaType(&m_mt);
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CKCamStream::GetNumberOfCapabilities(int* piCount, int* piSize)
{
    *piCount = 16;
    *piSize = sizeof(VIDEO_STREAM_CONFIG_CAPS);
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CKCamStream::GetStreamCaps(int iIndex, AM_MEDIA_TYPE** pmt, BYTE* pSCC)
{
    *pmt = CreateMediaType(&m_mt);
    DECLARE_PTR(VIDEOINFOHEADER, pvi, (*pmt)->pbFormat);

    iIndex++;

    pvi->bmiHeader.biCompression = BI_RGB;
    pvi->bmiHeader.biBitCount = 24;
    pvi->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    pvi->bmiHeader.biWidth = m_iImageWidth; //80 * iIndex;
    pvi->bmiHeader.biHeight = m_iImageHeight; // 60 * iIndex;
    pvi->bmiHeader.biPlanes = 1;
    pvi->bmiHeader.biSizeImage = GetBitmapSize(&pvi->bmiHeader);
    pvi->bmiHeader.biClrImportant = 0;

    SetRectEmpty(&(pvi->rcSource)); // we want the whole image area rendered.
    SetRectEmpty(&(pvi->rcTarget)); // no particular destination rectangle

    (*pmt)->majortype = MEDIATYPE_Video;
    (*pmt)->subtype = MEDIASUBTYPE_RGB24;
    (*pmt)->formattype = FORMAT_VideoInfo;
    (*pmt)->bTemporalCompression = FALSE;
    (*pmt)->bFixedSizeSamples = FALSE;
    (*pmt)->lSampleSize = pvi->bmiHeader.biSizeImage;
    (*pmt)->cbFormat = sizeof(VIDEOINFOHEADER);

    DECLARE_PTR(VIDEO_STREAM_CONFIG_CAPS, pvscc, pSCC);

    pvscc->guid = FORMAT_VideoInfo;
    pvscc->VideoStandard = AnalogVideo_None;
    pvscc->InputSize.cx = 0;// 640;
    pvscc->InputSize.cy = 0; // 480;
    pvscc->MinCroppingSize.cx = m_iImageWidth;// 80;
    pvscc->MinCroppingSize.cy = m_iImageHeight;// 60;
    pvscc->MaxCroppingSize.cx = m_iImageWidth;// 1280;
    pvscc->MaxCroppingSize.cy = m_iImageHeight;//  1024;
    pvscc->CropGranularityX = m_iImageWidth;// 80;
    pvscc->CropGranularityY = m_iImageHeight;// 60;
    pvscc->CropAlignX = 0;
    pvscc->CropAlignY = 0;



    pvscc->MinOutputSize.cx = m_iImageWidth;//80 * iIndex;
    pvscc->MinOutputSize.cy = m_iImageHeight;// 60 * iIndex;
    pvscc->MaxOutputSize.cx = m_iImageWidth;// 1280;
    pvscc->MaxOutputSize.cy = m_iImageHeight;// 1024;

    pvscc->OutputGranularityX = 0;
    pvscc->OutputGranularityY = 0;
    pvscc->StretchTapsX = 0;
    pvscc->StretchTapsY = 0;
    pvscc->ShrinkTapsX = 0;
    pvscc->ShrinkTapsY = 0;

    if (0) {
        pvscc->MinFrameInterval = 160000; //60 fps
        pvscc->MaxFrameInterval = 50000000; // 0.2 fps
        pvscc->MinBitsPerSecond = (80 * 60 * 3 * 8) / 5;
        pvscc->MaxBitsPerSecond = 1280 * 1024 * 3 * 8 * 60;
    }
    else {
        pvscc->MinFrameInterval = 33333; //30 fps
        pvscc->MaxFrameInterval = 33333;// 10000000; //0.1 fps
        pvscc->MinBitsPerSecond = (m_iImageWidth * m_iImageHeight * 3 * 8) * 30;
        pvscc->MaxBitsPerSecond = (m_iImageWidth * m_iImageHeight * 3 * 8) * 30;
    }

    return S_OK;
}

//////////////////////////////////////////////////////////////////////////
// IKsPropertySet
//////////////////////////////////////////////////////////////////////////


HRESULT CKCamStream::Set(REFGUID guidPropSet, DWORD dwID, void* pInstanceData,
    DWORD cbInstanceData, void* pPropData, DWORD cbPropData)
{
    // Set: Cannot set any properties.
    return E_NOTIMPL;
}

// Get: Return the pin category (our only property). 
HRESULT CKCamStream::Get(
    REFGUID guidPropSet, // Which property set.
    DWORD dwPropID, // Which property in that set.
    void* pInstanceData, // Instance data (ignore).
    DWORD cbInstanceData, // Size of the instance data (ignore).
    void* pPropData, // Buffer to receive the property data.
    DWORD cbPropData, // Size of the buffer.
    DWORD* pcbReturned // Return the size of the property.
)
{
    if (guidPropSet != AMPROPSETID_Pin) return E_PROP_SET_UNSUPPORTED;
    if (dwPropID != AMPROPERTY_PIN_CATEGORY) return E_PROP_ID_UNSUPPORTED;
    if (pPropData == nullptr && pcbReturned == nullptr) return E_POINTER;

    if (pcbReturned) *pcbReturned = sizeof(GUID);
    if (pPropData == nullptr) return S_OK; // Caller just wants to know the size. 
    if (cbPropData < sizeof(GUID)) return E_UNEXPECTED; // The buffer is too small.

    *(GUID*)pPropData = PIN_CATEGORY_CAPTURE;
    return S_OK;
}

// QuerySupported: Query whether the pin supports the specified property.
HRESULT CKCamStream::QuerySupported(REFGUID guidPropSet, DWORD dwPropID, DWORD* pTypeSupport)
{
    if (guidPropSet != AMPROPSETID_Pin) return E_PROP_SET_UNSUPPORTED;
    if (dwPropID != AMPROPERTY_PIN_CATEGORY) return E_PROP_ID_UNSUPPORTED;
    // We support getting this property, but not setting it.
    if (pTypeSupport) *pTypeSupport = KSPROPERTY_SUPPORT_GET;
    return S_OK;
}
