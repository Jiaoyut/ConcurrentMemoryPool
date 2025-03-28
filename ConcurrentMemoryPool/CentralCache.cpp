#include "CentralCache.h"
#include "PageCache.h"

CentralCache CentralCache::_sInst;

//��ȡһ���ǿյ�Span
Span* CentralCache::GetOneSpan(SpanList& list, size_t size) {
	
	//�鿴��ǰspanlist���Ƿ���δ��������span
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

	//�Ȱ�centralcache��Ͱ�������������������߳��ͷ��ڴ�����������������
	list._mtx.unlock();

	//�ߵ�����˵��û�п��е�span�ˣ�ֻ����page cacheҪ
	PageCache::GetInstance()->_pageMtx.lock();
	Span* span = PageCache::GetInstance()->NewSpan(SizeClass::NumMovePage(size));
	span->_isUse = true;
	span->_objSize = size;
	PageCache::GetInstance()->_pageMtx.unlock();
	//�Ի�ȡ��span�����з֣�����Ҫ��������Ϊ�����߳������ʲ������span
	
	//����span�Ĵ���ڴ����ʼ��ַ�ʹ���ڴ�Ĵ�С���ֽ�����
	char* start = (char*)(span->_pageId << PAGE_SHIFT);
	size_t bytes = span->_n << PAGE_SHIFT;
	void* end = start + bytes;

	//�Ѵ���ڴ��г�����������������
	//1.����һ��������_freeList��ͷ,����β��
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
	//�����к�span����Ҫ��span�ҵ�Ͱ����ȥ��ʱ�����
	list._mtx.lock();
	
	list.PushFront(span);

	return span;
}

//�����Ļ����ȡһ�������Ķ����threadcache
size_t CentralCache::FetchRangeObj(void*& start, void*& end, size_t batchNum, size_t size) {
	size_t index = SizeClass::Index(size);
	_spanList[index]._mtx.lock();

	Span* span = GetOneSpan(_spanList[index], size);
	assert(span);
	assert(span->_freeList);

	//��span�л�ȡbatchNum������
	//��������Ļ����ж����ö���
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
		//˵��span���зֳ�ȥ������С���ڴ涼������
		//���span�Ϳ����ٻ��ո�page cache��page cache�����ٳ���ȥ��ǰ��ҳ�ĺϲ�
		if (span->_useCount == 0) {
			_spanList[index].Erase(span);
			span->_freeList = nullptr;
			span->_next = nullptr;
			span->_prev = nullptr;
			//�ͷ�span��page cacheʱ��ʹ��page cache���У������������������thread cache�ͷ��ڴ�
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