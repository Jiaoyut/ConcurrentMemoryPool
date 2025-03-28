#include "CentralCache.h"
#include "PageCache.h"

CentralCache CentralCache::_sInst;

//获取一个非空的Span
Span* CentralCache::GetOneSpan(SpanList& list, size_t size) {
	
	//查看当前spanlist中是否还有未分配对象的span
	Span* it = list.Begin();
	while (it != list.End()) {
		if (it->_freeList != nullptr) {
			//it->_isUse = true;
			return it;
		}
		else{
			it = it->_next;
		}
	}

	//先把centralcache的桶锁解掉，这样如果其他线程释放内存对象回来，不会阻塞
	list._mtx.unlock();

	//走到这里说明没有空闲的span了，只能找page cache要
	PageCache::GetInstance()->_pageMtx.lock();
	Span* span = PageCache::GetInstance()->NewSpan(SizeClass::NumMovePage(size));
	span->_isUse = true;
	span->_objSize = size;
	PageCache::GetInstance()->_pageMtx.unlock();
	//对获取的span进行切分，不需要加锁，因为其他线程这会访问不到这个span
	
	//计算span的大块内存的起始地址和大块内存的大小（字节数）
	char* start = (char*)(span->_pageId << PAGE_SHIFT);
	size_t bytes = span->_n << PAGE_SHIFT;
	void* end = start + bytes;

	//把大块内存切成自由链表连接起来
	//1.先切一块下来给_freeList做头,方便尾插
	span->_freeList = start;
	start += size;
	void* tail = span->_freeList;

	int i = 1;
	while (start < end) {
		++i;
		NextObj(tail) = start;
		tail = NextObj(tail);
		start += size;
	}
	NextObj(tail) = nullptr;

	//span->_isUse = false;
	//这里切好span后需要把span挂到桶里面去的时候加锁
	list._mtx.lock();
	
	list.PushFront(span);

	return span;
}

//从中心缓存获取一定数量的对象给threadcache
size_t CentralCache::FetchRangeObj(void*& start, void*& end, size_t batchNum, size_t size) {
	size_t index = SizeClass::Index(size);
	_spanList[index]._mtx.lock();

	Span* span = GetOneSpan(_spanList[index], size);
	assert(span);
	assert(span->_freeList);

	//从span中获取batchNum个对象
	//如果不够的话，有多少拿多少
	start = span->_freeList;
	end = start;
	size_t i = 0;
	size_t actualNum = 1;
	while ( i < batchNum - 1 && NextObj(end)!= nullptr) {
		end = NextObj(end);
		++i;
		++actualNum;
	}
	span->_freeList = NextObj(end);
	NextObj(end) = nullptr;
	span->_useCount += actualNum;

	_spanList[index]._mtx.unlock();

	return actualNum;
}

void CentralCache::ReleaseListToSpans(void* start, size_t size) {
	size_t index = SizeClass::Index(size);
	_spanList[index]._mtx.lock();

	while (start != nullptr) {
		void* next = NextObj(start);

		Span* span = PageCache::GetInstance()->MapObjectToSpan(start);

		NextObj(start) = span->_freeList;
		span->_freeList = start;
		span->_useCount--;
		//span->_isUse = false;
		//说明span的切分出去的所有小块内存都回来了
		//这个span就可以再回收给page cache，page cache可以再尝试去做前后页的合并
		if (span->_useCount == 0) {
			_spanList[index].Erase(span);
			span->_freeList = nullptr;
			span->_next = nullptr;
			span->_prev = nullptr;
			//释放span给page cache时，使用page cache就行，把锁解掉，方便其他thread cache释放内存
			_spanList[index]._mtx.unlock();

			PageCache::GetInstance()->_pageMtx.lock();
			PageCache::GetInstance()->ReleaseSpanToPageCache(span);
			PageCache::GetInstance()->_pageMtx.unlock();
			_spanList[index]._mtx.lock();
		}

		start = next;
	}

	_spanList[index]._mtx.unlock();
}