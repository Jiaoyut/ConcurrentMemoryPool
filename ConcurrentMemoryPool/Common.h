#pragma once

#include <iostream> 
#include <vector>

#include <new>
#include <time.h>
#include <assert.h>

#include <thread>
#include <mutex>

using std::bad_alloc;
using std::cout;
using std::endl;

static const size_t MAX_BYTES = 256 * 1024;
static const size_t NFREELIST = 208;
  ;
#ifdef _WIN64
    typedef unsigned long long PAGE_ID
#elif _WIN32
    typedef size_t PAGE_ID; 
#elif
  //Linux��
#endif 

static void*& NextObj(void* obj) {
    return *(void**)obj;
}

//�����зֺõ�С�������������
class FreeList {
public:
    void Push(void* obj) {
        assert(obj);
        //ͷ��
        //*(void**)obj= _freeList;
        NextObj(obj) = _freeList;
        _freeList = obj;
    }

    void* Pop() {
        assert(_freeList);
        //ͷɾ
        void* obj = _freeList;
        _freeList = NextObj(obj);
        return obj;
    }

    bool Empty() {
        return _freeList == nullptr;
    }
private:
    void* _freeList = nullptr;
};


//��������С�Ķ���ӳ�����
class SizeClass {
    // ������������10%���ҵ�����Ƭ�˷�
    // [1,128]                      8byte����           freelist[0,16)
    // [128+1,1024]                 16byte����          freelist[16,72)
    // [1024+1,8 * 1024]            128byte����         freelist[72,128)
    // [8 * 1024+1,64 * 1024]       1024byte����        freelist[128,184)
    // [64 * 1024+1,256 * 1024]     8 * 1024byte����    freelist[184,208)
public:
    /*
    size_t _RoundUp(size_t size, size_t alignNum) {
        size_t alignSize;
        if (size % 8 != 0) {
            alignSize = (size / alignNum + 1) * alignNum;
        }
        else {
            alignSize = size;
        }
    }
    */
    static inline size_t _RoundUp(size_t bytes, size_t alignNums) {
        return ((bytes + alignNums - 1) & ~(alignNums - 1));
    }

    static inline size_t RoundUp(size_t size) {
        if (size <= 128) {
            return _RoundUp(size, 8);
        }
        else if (size <= 1024) {
            return _RoundUp(size, 16);
        }
        else if (size <= 8 * 1024) {
            return _RoundUp(size, 128);
        }
        else if (size <= 64 * 1024) {
            return _RoundUp(size, 1024);
        }
        else if (size < 256 * 1024) {
            return _RoundUp(size, 8 * 1024);
        }
        else {
            assert(false);
            return -1;
        }
    }

    // size_t _Index(size_t bytes,size_t alignNum) {
    //     if (bytes % alignNum == 0) {
    //         return bytes / alignNum - 1;
    //     }
    //     else {
    //         return bytes / alignNum;
    //     }
    // }

    static inline size_t _Index(size_t bytes, size_t align_shift) {
        return ((bytes + (1 << align_shift) - 1) >> align_shift) - 1;
    }

    //����ӳ�����һ����������Ͱ
    static inline size_t Index(size_t bytes) {
        assert(bytes <= MAX_BYTES);

        //ÿ�������ж��ٸ���
        static int group_array[4] = { 16,56,56,56 };
        if (bytes <= 128) {
            return _Index(bytes, 3);
        }
        if (bytes <= 1024) {
            return _Index(bytes - 128, 4) + group_array[0];
        }
        else if (bytes <= 8 * 1024) {
            return _Index(bytes - 1024, 7) + group_array[1] + group_array[0];
        }
        else if (bytes <= 64 * 1024) {
            return _Index(bytes - 8 * 1024, 10) + group_array[2] + group_array[1] + group_array[0];
        }
        else if (bytes <= 256 * 1024) {
            return _Index(bytes - 64 * 1024, 13) + group_array[3] + group_array[2] + group_array[1] + group_array[0];
        }
        else {
            assert(false);
        }
        return -1;
    }
};

//����������ҳ�Ĵ���ڴ��Ƚṹ
struct Span {
    size_t _pageId = 0;   //����ڴ����ʼҳ��ҳ��
    size_t _n = 0;        //ҳ������

    Span* _next =nullptr;      //˫������Ľṹ
    Span* _prev = nullptr;

    size_t _useCount = 0;  //�к�С���ڴ棬�������thread cache�ļ���
    void* _freeList = nullptr;   //�кõ�С���ڴ����������

};

//��ͷ˫��ѭ������
class SpanList
{
public:
    SpanList() {
        _head = new Span;
        _head->_next = _head;
        _head->_prev = _head;
    }

    void Insert(Span* pos, Span* newSpan) {
        assert(pos);
        assert(newSpan);

        Span* prev = pos->_prev;
        //prev newSpan pos
        prev->_next = newSpan;
        newSpan->_prev = prev;
        newSpan->_next = pos;
        pos->_prev = newSpan;
    }

    void Erase(Span* pos) {
        assert(pos);
        assert(pos != _head);

        Span* prev = pos->_prev;
        Span* next = pos->_next;

        prev->_next = next;
        next->_prev = prev;


    }

private:
    Span* _head;
    std::mutex _mtx;     //Ͱ��
};