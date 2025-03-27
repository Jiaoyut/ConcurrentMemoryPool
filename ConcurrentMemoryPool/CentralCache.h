#pragma once

#include "Common.h"


//����ģʽ��������
class CentralCache {
public:
	static CentralCache* GetInstance() {
		return &_sInst;
	}

	//��ȡһ���ǿյ�Span
	Span* GetOneSpan(SpanList& list, size_t size);

	//�����Ļ����ȡһ�������Ķ����threadcache
	size_t FetchRangeObj(void*& start, void*& end, size_t batchNum, size_t size);

	//��һ�������Ķ����ͷŵ�span���
	void ReleaseListToSpans(void* start, size_t byte_size);
private:
	SpanList _spanList[NFREELIST];

private:
	CentralCache(){}	  //���캯��˽�л�
	CentralCache(const CentralCache&) = delete;


private:
	static CentralCache _sInst;

};
