#pragma once
#include <QObject>

class TypeManager : public QObject
{
	Q_OBJECT
public:
	static TypeManager& instance();

	void sendPeopleId(int32_t peopleId);
	void sendPid(int32_t pid);
	void sendThreadId(int32_t threadId);
	void sendPidThreadId(int32_t pid, int32_t threadId);
	void sendLostPid(int32_t pid);

Q_SIGNALS:
	void receivePeopleId(int32_t peopleId);
	void receivePid(int32_t pid);
	void receiveThreadId(int32_t threadId);
	void receivePidThreadId(int32_t pid, int32_t threadId);
	void receiveLostPid(int32_t pid);
};