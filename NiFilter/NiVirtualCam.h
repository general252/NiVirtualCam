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

#pragma once

#define DECLARE_PTR(type, ptr, expr) type* ptr = (type*)(expr);

EXTERN_C const GUID CLSID_NiVirtualCam;

class CKCam : public CSource
{
public:
	//////////////////////////////////////////////////////////////////////////
	//  IUnknown
	//////////////////////////////////////////////////////////////////////////
	static CUnknown* WINAPI CreateInstance(LPUNKNOWN lpunk, HRESULT* phr);
	STDMETHODIMP QueryInterface(REFIID riid, void** ppv);

	IFilterGraph* GetGraph() { return m_pGraph; }

private:
	CKCam(LPUNKNOWN lpunk, HRESULT* phr);
};

class CKCamStream : public CSourceStream, public IAMStreamConfig, public IKsPropertySet
{
public:

	//////////////////////////////////////////////////////////////////////////
	//  IUnknown
	//////////////////////////////////////////////////////////////////////////
	STDMETHODIMP QueryInterface(REFIID riid, void** ppv);
	STDMETHODIMP_(ULONG) AddRef();
	STDMETHODIMP_(ULONG) Release();

	//////////////////////////////////////////////////////////////////////////
	//  IQualityControl
	//////////////////////////////////////////////////////////////////////////
	STDMETHODIMP Notify(IBaseFilter* pSender, Quality q);

	//////////////////////////////////////////////////////////////////////////
	//  IAMStreamConfig
	//////////////////////////////////////////////////////////////////////////
	HRESULT STDMETHODCALLTYPE SetFormat(AM_MEDIA_TYPE* pmt) override;
	HRESULT STDMETHODCALLTYPE GetFormat(AM_MEDIA_TYPE** ppmt) override;
	HRESULT STDMETHODCALLTYPE GetNumberOfCapabilities(int* piCount, int* piSize) override;
	HRESULT STDMETHODCALLTYPE GetStreamCaps(int iIndex, AM_MEDIA_TYPE** pmt, BYTE* pSCC) override;

	//////////////////////////////////////////////////////////////////////////
	//  IKsPropertySet
	//////////////////////////////////////////////////////////////////////////
	HRESULT STDMETHODCALLTYPE Set(REFGUID guidPropSet, DWORD dwID, void* pInstanceData, DWORD cbInstanceData,
	                              void* pPropData, DWORD cbPropData) override;
	HRESULT STDMETHODCALLTYPE Get(REFGUID guidPropSet, DWORD dwPropID, void* pInstanceData, DWORD cbInstanceData,
	                              void* pPropData, DWORD cbPropData, DWORD* pcbReturned) override;
	HRESULT STDMETHODCALLTYPE QuerySupported(REFGUID guidPropSet, DWORD dwPropID, DWORD* pTypeSupport) override;

	//////////////////////////////////////////////////////////////////////////
	//  CSourceStream
	//////////////////////////////////////////////////////////////////////////
	CKCamStream(HRESULT* phr, CKCam* pParent, LPCWSTR pPinName);
	~CKCamStream();

	//////////////////////////////////////////////////////////////////////////
	// CSourceStream
	//////////////////////////////////////////////////////////////////////////

	/*FillBuffer�ӿ�������дÿһ֡������Ƶ����
	* ����ӿ�һ���ȵ���IMediaSample::GetPointer�����һ֡���ݵĻ��壬Ȼ�������д��������塣
	* ���ŵ���IMediaSample::SetTime����֡��ʱ�����������IMediaSample::SetSyncPoint���ø�֡�Ƿ�Ϊ�ؼ�֡��
	* �����ṩ��ѹ�����ݵ�source filter��ÿһ֡��Ӧ������ΪTRUE�� https://www.jianshu.com/p/42489956f866
	*/ 
	HRESULT FillBuffer(IMediaSample* pms) override;
	/* DecideBufferSize�ӿ�����������Ҫ����buffer
	* CSourceStream��ĸ���CBasePin���и�m_mt�ĳ�Ա������GetMediaType���õ�ý���ʽ��
	* һ��buffer�Ĵ�Сͨ������GetMediaType�����õ�pmt->lSampleSize��һ����һ��bufferҲ���ˡ�
	*/
	HRESULT DecideBufferSize(IMemAllocator* pIMemAlloc, ALLOCATOR_PROPERTIES* pProperties) override;
	// GetMediaType�ӿ����ڷ���pin���ܵ�ý���ʽ
	HRESULT GetMediaType(int iPosition, CMediaType* pmt) override;
	HRESULT SetMediaType(const CMediaType* pmt) override;
	HRESULT OnThreadCreate(void) override;
	/*
	* ���source filter��������vmr7��vmr9����û��������⣩������Ҫ��ʵ�������ӿ�
	* HRESULT CheckMediaType(const CMediaType *pMediaType)
	* STDMETHODIMP Notify(IBaseFilter *pSelf, Quality q)
	* 
	* vmr7Ҫ�Լ�ʵ��CheckMediaType��ԭ���ǣ�
	  ���������CheckMediaType��DecideBufferSize�����е���IMemAllocator::SetProperties�ᱨ��
	  ����E_FAIL���Ӷ�����FillBuffer���ᱻ���ã����������IMemAllocator::SetProperties����DecideBufferSizeҲӦ�ñ��������Կ������У�

	  ����Ľ����ǣ�VMR/EVR filter����Ҫ��buffer stride���source filter GetMediaType���õ�ͼ���Ȳ�һ�£��Դ���Ҫ�ֽڶ��룩��
	  ���¸�ʽ�����ı䡣��VMR/EVR�ı��ʽ���ݵ�ʱ���������IPin::QueryAcceptѯ������filter�Ƿ���������ʽ��
	  CSourceStreamʵ�ֵ�CheckMediaType��������
	*/
	HRESULT CheckMediaType(const CMediaType* pMediaType) override;

private:
	CKCam* m_pParent;
	REFERENCE_TIME m_rtLastTime;
	HBITMAP m_hLogoBmp;
	CCritSec m_cSharedState;
	IReferenceClock* m_pClock;
};
