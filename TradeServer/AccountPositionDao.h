#pragma once

#include "ThostFtdcUserApiStruct.h"

class AccountPositionDao{
public:
	//�����˻��ֲ����
	void updatePosition(CThostFtdcInvestorPositionField *positionInfo);
};