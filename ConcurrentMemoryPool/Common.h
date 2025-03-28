#pragma once

#include <iostream> 
#include <unordered_map>
#include <vector>
#include <algorithm>
#ifdef _WIN32
    #include <Windows.h>
#else
    //Linux
#endif // _WIN32



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
static const size_t NPAGES = 129;
static const size_t PAGE_SHIFT = 13;
  ;
#ifdef _WIN64
    typedef unsigned long long PAGE_ID
#elif _WIN32
    typedef size_t PAGE_ID; 
#elif
  //Linux��
#endif 


inline static void* SystemAlloc(size_t kpage) {
      void* ptr = nullptr;

    #ifdef _WIN32
      // Windowsƽ̨ʹ��VirtualAlloc��ÿҳ8KB��2^13��
      ptr = VirtualAlloc(0, kpage << 13, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    #else
      // Linuxƽ̨ʹ��mmap���������ڴ��
      size_t total_size = kpage * (1 << 13); // �������ֽ�����8KB/page��
      ptr = mmap(nullptr,
          total_size,
          PROT_READ | PROT_WRITE,
          MAP_PRIVATE | MAP_ANONYMOUS,
          -1, 0);  // ����˵��������ӳ�䲻��Ҫ�ļ�������

      if (ptr == MAP_FAILED) {  // mmapʧ�ܷ�������ֵ
          ptr = nullptr;
      }
    #endif

      if (ptr == nullptr) {
          throw std::bad_alloc();  // �����쳣�����ռ�
      }
      return ptr;  // ɾ�������return���
  }

inline static void SystemFree(void* ptr) {
#ifdef _WIN32
    VirtualFree(ptr, 0, MEM_RELEASE);
#else
    //linux/maxos�� sbrk unmmap��
#endif
}

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
        ++_size;
    }

    void PushRang(void* start, void* end,size_t n) {
        NextObj(end) = _freeList;
        _freeList = start;

        //������֤+�����ϵ�
        /*int i = 0;
        void* cur = start;
        while (cur) {
            cur = NextObj(cur);
            ++i;
        }
        if (i != n) {
           i = 0;
        }*/

        _size += n;
    }

    void PopRange(void*& start, void*& end, size_t n) {
       /* assert(n <= _size);*/
        assert(n >= _size);
        start = _freeList;
        end = start;

        for (size_t i = 0; i < n - 1; ++i) {
            end = NextObj(end);
        }

        _freeList = NextObj(end);
        NextObj(end) = nullptr;
        _size -= n;
    }

    size_t Size() {
        return _size;
    }

    void* Pop() {
        assert(_freeList);
        //ͷɾ
        void* obj = _freeList;
        _freeList = NextObj(obj);
        --_size;
        return obj;
        
    }

    bool Empty() {
        return _freeList == nullptr;
    }

    size_t& MaxSize() {
        return _maxSize;
    }
private:
    void* _freeList = nullptr;
    size_t _maxSize = 1;
    size_t _size = 0;
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
            return _RoundUp(size, 1 << PAGE_SHIFT);
            /*assert(false);
            return -1;*/
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

    //һ��threadcache�����Ļ����ȡ���ٸ�
    static size_t NumMoveSize(size_t size) {
        assert(size > 0);

        //[2,512],һ�������ƶ����ٸ����������ֵ
        //С����һ���������޸�
        //С����һ���������޵�
        int num = MAX_BYTES / size;
        if (num < 2)
            return 1; //return 0;
        
        if (num > 512)
            num = 512;
        return num;
    }

    //����һ����ϵͳ��ȡ����ҳ
    //�������� 8bytes
    //������
    //��������256kB
    static size_t NumMovePage(size_t size) {
        //size_t num = NumMovePage(size);
        size_t num = NumMoveSize(size);
        size_t npage = num * size;

        npage >>= PAGE_SHIFT;
        if (npage == 0)
            npage = 1;
        return npage;
    }
};

//����������ҳ�Ĵ���ڴ��Ƚṹ
struct Span {
    size_t _pageId = 0;   //����ڴ����ʼҳ��ҳ��
    size_t _n = 0;        //ҳ������

    Span* _next =nullptr;      //˫������Ľṹ
    Span* _prev = nullptr;

    size_t _objSize = 0;    //�кõ�С����Ĵ�С
    size_t _useCount = 0;  //�к�С���ڴ棬�������thread cache�ļ���
    void* _freeList = nullptr;   //�кõ�С���ڴ����������

    bool _isUse = false; //�Ƿ��ٱ�ʹ��

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

    Span* Begin() {
        return _head->_next;
    }

    Span* End() {
        return _head;
    }

    bool Empty() {
        return _head->_next == _head;
    }

    void PushFront(Span* span) {
        Insert(Begin(), span);
    }

    Span* PopFront() {
        Span* front = _head->_next;
        Erase(front);
        return front;
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

        //1.�����ϵ�
        //2.�鿴ջ֡
       /* if (pos == _head) {
            int x = 0;
        }*/

        Span* prev = pos->_prev;
        Span* next = pos->_next;

        prev->_next = next;
        next->_prev = prev;


    }

private:
    Span* _head;
public:
    std::mutex _mtx;     //Ͱ��
};