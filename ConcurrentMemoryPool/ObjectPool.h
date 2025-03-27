#pragma once
#include "Common.h"


#ifdef _WIN32
#include <windows.h>
#else
#include <sys/mman.h>
#endif

// inline static void* SystemAlloc(size_t kpage) {
// #ifdef _WIN32
//     void* ptr = VirtualAlloc(0, kpage << 13, MEM_COMMIT | MEM_RESERVE, PAGE_READWR)
// #else
//     //Linux下brk mmap等
// #endif
//     if (ptr == nullptr) {
//         throw bad_alloc();
//     }
//     return ptr;
// }
/*
inline static void* SystemAlloc(size_t kpage) {
    void* ptr = nullptr;

#ifdef _WIN32
    // Windows平台使用VirtualAlloc，每页8KB（2^13）
    ptr = VirtualAlloc(0, kpage << 13, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
#else
    // Linux平台使用mmap分配匿名内存块
    size_t total_size = kpage * (1 << 13); // 计算总字节数（8KB/page）
    ptr = mmap(nullptr,
        total_size,
        PROT_READ | PROT_WRITE,
        MAP_PRIVATE | MAP_ANONYMOUS,
        -1, 0);  // 参数说明：匿名映射不需要文件描述符

    if (ptr == MAP_FAILED) {  // mmap失败返回特殊值
        ptr = nullptr;
    }
#endif

    if (ptr == nullptr) {
        throw std::bad_alloc();  // 修正异常命名空间
    }
    return ptr;  // 删除多余的return语句
}

*/

//定长内存池
//template<size_t N>
//class ObjectPool
//{};

template<class T>
class ObjectPool {
public:
    T* New() {
        T* obj = nullptr;
        //优先把还回来的内存块对象重复利用
        if (_freeList) {
            void* next = *((void**)_freeList);
            obj = (T*)_freeList;
            _freeList = next;
            return obj;
        }
        else {
            //如果申请一个对象都不够，重新申请大块内存
            if (_remainBytes < sizeof(T)) {
                _remainBytes = 128 * 1024;
                //_memory = (char*)malloc(_remainBytes);
                _memory = (char*)SystemAlloc(_remainBytes >> 13);
                if (_memory == nullptr) {
                    throw bad_alloc();
                }
            }



            obj = (T*)_memory;
            size_t objSize = sizeof(T) < sizeof(void*) ? sizeof(void*) : sizeof(T);

            _memory += objSize;
            _remainBytes -= objSize;

        }
        //定位new，显示调用T的构造函数初始化
        new(obj)T;

        return obj;
    }
    void Delete(T* obj) {
        //显示调用T的析构函数清理对象
        obj->~T();
        //头插
        *(void**)obj = _freeList;//32位下void*（或者其他类型的指针）4字节大小，64位下8字节大小；
        _freeList = obj;
    }
private:
    char* _memory = nullptr;  //指向大块内存的指针，char刚好一个字节，作为单位，好切内存
    size_t _remainBytes = 0;    //大块内存在切分内存过程中剩余字节数
    void* _freeList = nullptr;    //还回来过程中连接自由链表的头指针
};

struct TreeNode {
    int _val;
    TreeNode* _left;
    TreeNode* _right;

    TreeNode()
        :_val(0)
        , _left(nullptr)
        , _right(nullptr)
    {
    }
};


static void TestObjectPool() {
    //申请释放的轮次
    const size_t Rounds = 3;

    //每轮申请释放多少次
    const size_t N = 1000000;
    std::vector<TreeNode*> v1;
    v1.reserve(N);
    size_t begin1 = clock();

    for (size_t j = 0; j < Rounds; ++j) {
        for (int i = 0; i < N; ++i) {
            v1.push_back(new TreeNode);
        }
        for (int i = 0; i < N; ++i) {
            delete v1[i];
        }
        v1.clear();
    }
    size_t end1 = clock();
    cout << "new/delete time:" << end1 - begin1 << endl;

    std::vector<TreeNode*> v2;
    v2.reserve(N);
    size_t begin2 = clock();
    ObjectPool<TreeNode> TNpool;
    for (size_t j = 0; j < Rounds; ++j) {
        for (int i = 0; i < N; ++i) {
            v2.push_back(TNpool.New());
        }
        for (int i = 0; i < N; ++i) {
            TNpool.Delete(v2[i]);
        }
        v2.clear();
    }
    size_t end2 = clock();
    cout << "ObjectPool time:" << end2 - begin2 << endl;
}