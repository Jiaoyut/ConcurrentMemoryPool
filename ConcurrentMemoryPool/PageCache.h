#pragma once

#include "Common.h"

class PageCache {
public:
	static PageCache* GetInstance() {
		return &_sInst;
	}
	
	//��ȡ�Ӷ���Span��ӳ��
	Span* MapObjectToSpan(void* obj);

	//�ͷſ���span�ص�pagecache���ϲ����ڵ�span
	void ReleaseSpanToPageCache(Span* span);

	//��ȡһ��kҳ��span
	Span* NewSpan(size_t k);
public:
	std::mutex _pageMtx;
private:
	SpanList _spanLists[NPAGES];
	std::unordered_map<PAGE_ID, Span*> _idSpanMap;

	PageCache() {

	}
	PageCache(const PageCache&) = delete;

	static PageCache _sInst;
};
