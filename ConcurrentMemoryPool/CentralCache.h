#pragma once

#include "Common.h"


//单例模式（饿汉）
class CentralCache {
public:
	CentralCache* GetInstance() {
		return &_sInst;
	}

private:
	SpanList _spanList[NFREELIST];

private:
	CentralCache(){}	  //构造函数私有化
	CentralCache(const CentralCache&) = delete;


private:
	static CentralCache _sInst;

};
