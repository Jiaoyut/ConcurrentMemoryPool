#include "ConcurrentAlloc.h"


////ntimes 一轮申请和释放内存的次数
////rounds 一共进行多少轮
//void BenchmarkMalloc(size_t ntimes, size_t nworks, size_t rounds) {
//	std::vector<std::thread> vthread(nworks);
//	std::atomic<size_t> malloc_costime = 0;
//	std::atomic<size_t> free_costime = 0;
//
//	for (size_t k = 0; k < nworks; ++k) {
//		vthread[k] = std::thread([&, k])() {
//			std::vector<void*> v;
//			v.revers(ntimes);
//
//			for (size_t j = 0; j < rounds; ++j) {
//				size_t begin1 = clock();
//				for (size_t i = 0; i < ntimes; ++i) {
//					v.push_back(malloc(16))
//				}
//			}
//		};
//}
		   