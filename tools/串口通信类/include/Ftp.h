#pragma once
/*
实现FTP功能类 目录均已‘/’分割
文件名均为全路径
*/



class CFtp
{
public:
	CFtp(CString strServer, CString strUserName, CString strUserPwd, UINT uPort=21, BOOL bPassive=FALSE);
	~CFtp(void);

	void SetParam(CString strServer, CString strUserName, CString strUserPwd, UINT uPort=21, BOOL bPassive=FALSE);
	// 上传文件(远端目录自动创建)
	BOOL PutFile(CString strLocalFile, CString strRemoteFile);
	// 下载文件(本地存在文件删除)
	BOOL GetFile(CString strRemoteFile, CString strLocalFile);
	// 检查(创建)目录
	BOOL CheckDir(CString strRemotePath, BOOL bCreate=TRUE);
	// 删除文件
	BOOL DeleteFile(CString strRemoteFile);

public:
	CString m_strServer;//IP
	UINT m_uPort;//端口
	BOOL m_bPassive;//被动模式
	CString m_strUserName;//用户名
	CString m_strUserPwd;//密码

};
