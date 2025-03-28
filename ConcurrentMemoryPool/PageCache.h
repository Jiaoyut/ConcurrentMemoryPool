#pragma once

#include "Common.h"
#include "ObjectPool.h"

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
	ObjectPool<Span> _spanPool;
	std::unordered_map<PAGE_ID, Span*> _idSpanMap;
	//std::unordered_map<PAGE_ID, size_t> _idSizeMap;

	PageCache() 
	{}
	PageCache(const PageCache&) = delete;

	static PageCache _sInst;
};
