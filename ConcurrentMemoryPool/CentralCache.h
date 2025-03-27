#pragma once

#include "Common.h"


//单例模式（饿汉）
class CentralCache {
public:
	static CentralCache* GetInstance() {
		return &_sInst;
	}

	//获取一个非空的Span
	Span* GetOneSpan(SpanList& list, size_t size);

	//从中心缓存获取一定数量的对象给threadcache
	size_t FetchRangeObj(void*& start, void*& end, size_t batchNum, size_t size);

	//将一定数量的对象释放掉span跨度
	void ReleaseListToSpans(void* start, size_t byte_size);
private:
	SpanList _spanList[NFREELIST];

private:
	CentralCache(){}	  //构造函数私有化
	CentralCache(const CentralCache&) = delete;


private:
	static CentralCache _sInst;

};
