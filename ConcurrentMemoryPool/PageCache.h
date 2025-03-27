#pragma once

#include "Common.h"

class PageCache {
public:
	static PageCache* GetInstance() {
		return &_sInst;
	}
	
	//获取从对象到Span的映射
	Span* MapObjectToSpan(void* obj);

	//释放空闲span回到pagecache，合并相邻的span
	void ReleaseSpanToPageCache(Span* span);

	//获取一个k页的span
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
