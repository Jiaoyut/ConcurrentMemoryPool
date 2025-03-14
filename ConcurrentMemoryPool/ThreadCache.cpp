#include "ThreadCache.h"

void* ThreadCache::FetchFromCentralCache(size_t index, size_t size) {
    //满开始反馈调节算法
    size_t batchNum = 

    return nullptr;
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

}