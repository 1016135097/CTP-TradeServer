#pragma once

#include <qobject.h>
#include <qthread.h>
#include <qtimer.h>
#include <queue>
#include <memory>
#include <mutex>
#include "ApiCommand.h"

//ָ�����
class CommandQueue :public QThread{
	Q_OBJECT
public:
	CommandQueue() = default;
	~CommandQueue();
	void addCommand(std::shared_ptr<ApiCommand> newCommand);
private:
	std::queue<std::shared_ptr<ApiCommand>> commandQueue;
	std::mutex queueMutex;
	bool working = true;				//����״̬
private:
	void run() Q_DECL_OVERRIDE;
};