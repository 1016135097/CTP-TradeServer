#include "CommandQueue.h"
#include <qdebug.h>
#include <thread>
#include <chrono>
using std::lock_guard;
using std::mutex;

CommandQueue::~CommandQueue(){
	working = false;		//ֹͣ�̹߳���
	this->quit();
	this->wait();
}

void CommandQueue::addCommand(std::shared_ptr<ApiCommand> newCommand){
	lock_guard<mutex> lock(queueMutex);
	commandQueue.push(newCommand);
}

//һ��֮�ڲ�����������ָ��
void CommandQueue::run(){
	while (working){
		if (!commandQueue.empty()){
			int NOT_ZERO = 100;
			int result = NOT_ZERO;
			{
				lock_guard<mutex> lock(queueMutex);
				auto command = commandQueue.front();
				result = command->execute();
				if (result == 0){
					//resultΪ0��ζ�ŷ���ָ��ɹ�
					commandQueue.pop();
				}
			}
			//���������ָ����Ϣһ�룬����δ���ų�����Ϊ���ͷ�����ʹ�ó�˯��һ���ڼ�����������Է����ٽ���Դ
			if (result == 0){
				std::this_thread::sleep_for(std::chrono::seconds(1));
				//qDebug() << "command thread ID:" << QThread::currentThreadId();
			}
		}
	}
}