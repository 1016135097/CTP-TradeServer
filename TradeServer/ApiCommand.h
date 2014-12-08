#pragma once

#include "ThostFtdcTraderApi.h"

//��װapi����Ľӿ�

class ApiCommand{
public:
	virtual ~ApiCommand();
	virtual int execute() = 0;
protected:
	ApiCommand(int &requestID, CThostFtdcTraderApi *api);
	int &requestID;
	CThostFtdcTraderApi *api;
};