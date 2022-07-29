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

	/*FillBuffer接口用于填写每一帧的音视频数据
	* 这个接口一般先调用IMediaSample::GetPointer获得新一帧数据的缓冲，然后把数据写入这个缓冲。
	* 接着调用IMediaSample::SetTime设置帧的时间戳。最后调用IMediaSample::SetSyncPoint设置该帧是否为关键帧。
	* 对于提供非压缩数据的source filter，每一帧都应该设置为TRUE。 https://www.jianshu.com/p/42489956f866
	*/ 
	HRESULT FillBuffer(IMediaSample* pms) override;
	/* DecideBufferSize接口用于设置需要多大的buffer
	* CSourceStream类的父类CBasePin中有个m_mt的成员保存了GetMediaType设置的媒体格式。
	* 一份buffer的大小通常就是GetMediaType中设置的pmt->lSampleSize。一般用一份buffer也够了。
	*/
	HRESULT DecideBufferSize(IMemAllocator* pIMemAlloc, ALLOCATOR_PROPERTIES* pProperties) override;
	// GetMediaType接口用于返回pin接受的媒体格式
	HRESULT GetMediaType(int iPosition, CMediaType* pmt) override;
	HRESULT SetMediaType(const CMediaType* pmt) override;
	HRESULT OnThreadCreate(void) override;
	/*
	* 如果source filter的下游是vmr7（vmr9测试没有这个问题），还需要多实现两个接口
	* HRESULT CheckMediaType(const CMediaType *pMediaType)
	* STDMETHODIMP Notify(IBaseFilter *pSelf, Quality q)
	* 
	* vmr7要自己实现CheckMediaType的原因是，
	  如果不重载CheckMediaType，DecideBufferSize函数中调用IMemAllocator::SetProperties会报错，
	  返回E_FAIL，从而导致FillBuffer不会被调用（更正，如果IMemAllocator::SetProperties，则DecideBufferSize也应该报错，这样仍可以运行）

	  上面的解释是，VMR/EVR filter所需要的buffer stride会跟source filter GetMediaType设置的图像宽度不一致（显存需要字节对齐），
	  导致格式发生改变。当VMR/EVR改变格式数据的时候，它会调用IPin::QueryAccept询问上游filter是否接受这个格式。
	  CSourceStream实现的CheckMediaType是这样的
	*/
	HRESULT CheckMediaType(const CMediaType* pMediaType) override;

private:
	CKCam* m_pParent;
	REFERENCE_TIME m_rtLastTime;
	HBITMAP m_hLogoBmp;
	CCritSec m_cSharedState;
	IReferenceClock* m_pClock;
};
