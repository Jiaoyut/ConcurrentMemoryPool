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
//     //Linux��brk mmap��
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

*/

//�����ڴ��
//template<size_t N>
//class ObjectPool
//{};

template<class T>
class ObjectPool {
public:
    T* New() {
        T* obj = nullptr;
        //���Ȱѻ��������ڴ������ظ�����
        if (_freeList) {
            void* next = *((void**)_freeList);
            obj = (T*)_freeList;
            _freeList = next;
            return obj;
        }
        else {
            //�������һ�����󶼲����������������ڴ�
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
        //��λnew����ʾ����T�Ĺ��캯����ʼ��
        new(obj)T;

        return obj;
    }
    void Delete(T* obj) {
        //��ʾ����T�����������������
        obj->~T();
        //ͷ��
        *(void**)obj = _freeList;//32λ��void*�������������͵�ָ�룩4�ֽڴ�С��64λ��8�ֽڴ�С��
        _freeList = obj;
    }
private:
    char* _memory = nullptr;  //ָ�����ڴ��ָ�룬char�պ�һ���ֽڣ���Ϊ��λ�������ڴ�
    size_t _remainBytes = 0;    //����ڴ����з��ڴ������ʣ���ֽ���
    void* _freeList = nullptr;    //�������������������������ͷָ��
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
    //�����ͷŵ��ִ�
    const size_t Rounds = 3;

    //ÿ�������ͷŶ��ٴ�
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