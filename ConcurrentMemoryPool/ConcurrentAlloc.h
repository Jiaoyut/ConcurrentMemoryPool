#pragma once

#include "Common.h"
#include "ThreadCache.h"
#include "PageCache.h"
#include "ObjectPool.h"

static void* ConcurrentAlloc(size_t size) {
    //通过TLS 每个线程无锁的获取自己专属的threadCache对象
    if (size > MAX_BYTES) {
        size_t alignSize = SizeClass::RoundUp(size);
        size_t kpage = alignSize >> PAGE_SHIFT;

        PageCache::GetInstance()->_pageMtx.lock();
        Span* span = PageCache::GetInstance()->NewSpan(kpage);
        span->_objSize = size;
        PageCache::GetInstance()->_pageMtx.unlock();

        void* ptr = (void*)(span->_pageId << PAGE_SHIFT);
        return ptr;
    }
    else {
        if (pTLSThreadCache == nullptr) {
            //pTLSThreadCache = new ThreadCache;
            static ObjectPool<ThreadCache> tcPool;
            pTLSThreadCache = tcPool.New();
        }
        cout << std::this_thread::get_id() << ":" << pTLSThreadCache << endl;
        return pTLSThreadCache->Allocate(size);
    }
    
}

static void ConcurrentFree(void* ptr) {
    Span* span = PageCache::GetInstance()->MapObjectToSpan(ptr);//将ptr指针转换成对应的页号
    size_t size = span->_objSize;
    if (size > MAX_BYTES) {
        PageCache::GetInstance()->_pageMtx.lock();
        PageCache::GetInstance()->ReleaseSpanToPageCache(span);
        PageCache::GetInstance()->_pageMtx.unlock();
    }
    else {
        assert(pTLSThreadCache);

	    pTLSThreadCache->Deallocate(ptr,size);
    }
   
    
}
