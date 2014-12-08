#pragma once

#include <qstring.h>
#include <qdatetime.h>
#include <qtimer.h>
#include "ThostFtdcUserApiStruct.h"

class Order:public QObject{
	Q_OBJECT
public:
	Order(QObject *parent = 0);
	//getter
	const QString& getInvestorId() const;
	const QString& getStrategyId() const;
	const QString& getInstructionId() const;
	const QDate& getDate() const;
	const int& getOrderRef() const;
	const QString& getSystemId() const;
	const int& getSequenceId() const;
	const QString& getInstrumentId() const;
	const char& getDirection() const;
	const char& getOpenCloseFlag() const;
	const double& getPrice() const;
	const int& getOriginalVolume() const;
	const int& getTradedVolume() const;
	const int& getRestVolume() const;
	const char& getOrderStatus() const;
	//setter
	void setInvestorId(const QString &);
	void setDate(const QDate &);
	void setOrderRef(const int &);
	void setStrategyId(const QString &);
	void setInstructionId(const QString &);
	void setSystemId(const QString &);
	void setSequenceId(const int &);
	void setInstrumentId(const QString &);
	void setDirection(const char &);
	void setOpenCloseFlag(const char &);
	void setPrice(const double &);
	void setOriginalVolume(const int &);
	void setTradedVolume(const int &);
	void setRestVolume(const int &);
	void setOrderStatus(const char &);
	//���ݱ����ر����²�ִ����ض���
	void update(CThostFtdcOrderField *pOrder);
	//��ø��±�־
	bool getUpdateFlag(){
		return updateFlag;
	}
	//���ø��±�־
	void recoverUpdateFlag(){
		updateFlag = false;
	}
signals:
	void refreshTimer(int msec);
	void stopTimer();
private slots:
	void cancelThisOrder();
private:
	/*******************************����***********************************************/
	//������־ȷ��Ψһ�ı���
	QString investorId;
	QDate date;
	int orderRef;

	QString strategyId;		//����Id
	QString instructionId;	//ָ��Id

	QString systemId;	//���Ͻ����������ģ�����ϵͳ���,����ʱ����ʹ��
	int sequenceId = -1;	//��������˳��ı��

	//������������Ϣ
	QString instrumentId;
	char direction;		//'b'��������,'s'��������
	/*	���� '0'
		ƽ�� '1'
		ǿƽ '2'
		ƽ�� '3'
		ƽ�� '4'
		ǿ�� '5'
		����ǿƽ '6'	*/
	char openCloseFlag;	
	double price;		
	int originalVolume;
	int tradedVolume;
	int restVolume;
	/*	'f' ȫ�����
		'a' ��Ծ״̬
		'c' ����
		'w' ��*/
	char orderStatus;
	/*******************************����***********************************************/
	QTimer *timer;
	bool updateFlag = false;
};