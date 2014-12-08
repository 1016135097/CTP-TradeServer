#pragma once

class AccountFund{
public:
	AccountFund() = default;
	AccountFund(const AccountFund&) = delete;
	AccountFund& operator=(const AccountFund&) = delete;

	//��˽�б�����getter��setter
	void setPreBalance(const double &preBalance);
	const double & getPreBalance();

	void setDeposit(const double &deposit);
	const double & getDeposit();

	void setWithdraw(const double &withdraw);
	const double & getWithdraw();

	void setAvalible(const double &avalible);
	const double & getAvalible();

	void setCurrrentMargin(const double &currentMargin);
	const double & getCurrentMargin();

	void setFrozenMargin(const double &frozenMargin);
	const double & getFrozenMargin();

	void setCommission(const double &commission);
	const double & getCommission();

	void setCloseProfit(const double &closeProfit);
	const double & getCloseProfit();

	void setPositionProfit(const double &positionProfit);
	const double & getPositionProfit();
private:
	double preBalance = 0.0;		//�ϴν���׼����
	double deposit = 0.0;			//�����
	double withdraw = 0.0;		//������
	double avalible = 0.0;		//�����ʽ�
	double currentMargin = 0.0;	//��ǰ��֤���ܶ�
	double frozenMargin = 0.0;	//���ᱣ֤��
	double commission = 0.0;		//������
	double closeProfit = 0.0;		//ƽ��ӯ��
	double positionProfit = 0.0;	//�ֲ�ӯ��
};