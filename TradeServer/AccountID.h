#pragma once

#include <qstring.h>

//��¼�˻��Ļ�����Ϣ���û�ID�������˺ţ��˺����룬�����̴��룬ǰ�û���ַ
class AccountID{
public:
	AccountID() = default;
	AccountID(const AccountID&) = delete;
	AccountID& operator=(const AccountID&) = delete;
	void setInvestorID(const QString &investorID);
	void setPassword(const QString &password);
	void setBrokerID(const QString &brokerID);
	void setFrontAddress(const QString &frontAddress);
	const QString &getInvestorID() const;
	const QString &getPassword() const;
	const QString &getBrokerID() const;
	const QString &getFrontAddress() const;
private:
	QString investorID;
	QString password;
	QString brokerID;
	QString frontAddress;
};