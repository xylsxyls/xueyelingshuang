#include "MsgLinux.h"
#include <string.h>
#include <sys/msg.h>
#include <sys/stat.h>

#define MSG_BUFFER_SIZE 10240

struct msgtype
{
    long m_type;
    char m_buffer[MSG_BUFFER_SIZE];
};

MsgLinux::MsgLinux(const std::string& path, bool isCreate):
m_msgid(-1),
m_isCreate(false)
{
    m_isCreate = isCreate;
	key_t key = ftok(path.c_str(), 'a');
    if (key != -1)
    {
        m_msgid = msgget(key, m_isCreate ? (S_IRUSR | S_IWUSR | IPC_CREAT | IPC_EXCL) : (S_IRUSR | S_IWUSR));
        if (m_msgid == -1)
        {
            m_msgid = msgget(key, S_IRUSR | S_IWUSR);
            msqid_ds mds;
            msgctl(m_msgid, IPC_RMID, &mds);
            m_msgid = msgget(key, m_isCreate ? (S_IRUSR | S_IWUSR | IPC_CREAT | IPC_EXCL) : (S_IRUSR | S_IWUSR));
        }
    }
}

MsgLinux::~MsgLinux()
{
    destroy();
}

bool MsgLinux::send(const std::string& src, long type)
{
    struct msgtype msg;
    msg.m_type = type;
    
    int32_t sendSize = 0;
    int32_t remainSize = src.size();
    int32_t currentPos = 0;
    while (currentPos < src.size())
    {
        if (remainSize >= MSG_BUFFER_SIZE)
        {
            remainSize -= MSG_BUFFER_SIZE;
            sendSize = MSG_BUFFER_SIZE;
        }
        else
        {
            sendSize = remainSize;
        }

        ::memcpy(msg.m_buffer, (char*)&(src[currentPos]), sendSize);
        
        int res = msgsnd(m_msgid, &msg, sendSize, 0);
        if (res != 0)
        {
            return false;
        }
        currentPos += sendSize;
    }
    if (remainSize == 0)
    {
        int res = msgsnd(m_msgid, &msg, 0, 0);
        if (res != 0)
        {
            return false;
        }
    }
    return true;
}

bool MsgLinux::recv(std::string& receive, long type)
{
    struct msgtype msg;
    int32_t receiveSize = 0;

    receive.clear();
    do
    {
        receiveSize = msgrcv(m_msgid, &msg, MSG_BUFFER_SIZE, type, 0);
        if (errno == EIDRM || receiveSize == -1)
        {
            return false;
        }
        receive.append(msg.m_buffer, receiveSize);
    } while (receiveSize == MSG_BUFFER_SIZE);
    return true;
}

void MsgLinux::destroy()
{
    if (m_msgid != -1 && m_isCreate)
    {
        msqid_ds mds;
        int res = msgctl(m_msgid, IPC_RMID, &mds);
    	if (res != 0)
        {
            printf("msgctl error = %d, msgid = %d\n", res, m_msgid);
	    }
        m_msgid = -1;
    }
}