#include "CentralCache.h"
#include "ThreadCache.h"

void* ThreadCache::FetchFromCentralCache(size_t index, size_t size) {
    //满开始反馈调节算法
    //1.最开始不会一次想central cache一次批量要太多，因为要太多可能用不完
    //2.如果你不断有size大小内存需求，那么batchNum就会不断增长，直到上限
    //3.size越大，一次向central cache要的batchNum就越小
    //4.size越小，一次向centralcache要的batchNum就越大
    size_t batchNum = min(_freeLists[index].MaxSize(), SizeClass::NumMoveSize(size));
    if (_freeLists[index].MaxSize() == batchNum) {
         _freeLists[index].MaxSize() += 1;
    }
    void* start = nullptr;
    void* end = nullptr;

    size_t actualNum = CentralCache::GetInstance()->FetchRangeObj(start, end, batchNum, size);
    assert(actualNum>=1);

    if (actualNum == 1) {
        assert(start == end);
        return  start;
    }
    else {
        _freeLists[index].PushRang(NextObj(start), end,actualNum-1);
        return start;
    }
}

void* ThreadCache::Allocate(size_t size) {
    //
    assert(size <= MAX_BYTES);

    size_t alignSize = SizeClass::RoundUp(size);
    size_t index = SizeClass::Index(size);

    if (!_freeLists[index].Empty()) {
        return _freeLists[index].Pop();
    }
    else {
        return FetchFromCentralCache(index, alignSize);
    }

}

void ThreadCache::Deallocate(void* ptr, size_t size) {
    assert(size <= MAX_BYTES);
    assert(ptr);

    
	//找出对应的自由链表桶，插入进去
	size_t index = SizeClass::Index(size);
	_freeLists[index].Push(ptr);

    //当链表长度大于一次批量申请的内存时就开始还一段list给central cache
    if (_freeLists[index].Size()>=_freeLists[index].MaxSize()) {
        ListTooLong(_freeLists[index], size);
    }

}

void ThreadCache::ListTooLong(FreeList& list, size_t size) {
    void* start = nullptr;
    void* end = nullptr;
    list.PopRange(start, end, list.MaxSize());

    CentralCache::GetInstance()->ReleaseListToSpans(start,size);
}