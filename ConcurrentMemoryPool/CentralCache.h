#pragma once

#include "Common.h"


//����ģʽ��������
class CentralCache {
public:
	CentralCache* GetInstance() {
		return &_sInst;
	}

private:
	SpanList _spanList[NFREELIST];

private:
	CentralCache(){}	  //���캯��˽�л�
	CentralCache(const CentralCache&) = delete;


private:
	static CentralCache _sInst;

};
