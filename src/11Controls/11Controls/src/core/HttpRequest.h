#pragma once
#include <QObject>
#include <QMap>

class QNetworkReply;
class QNetworkAccessManager;

//* http�����࣬����post��get
class HttpRequest : public QObject
{
	Q_OBJECT
protected:
	//* ���캯��
	HttpRequest();

public:
	//* ��һʵ��
	static HttpRequest& instance();

public:
	//* ����һ��http get ����,url�����������
	void httpGet(const QString& url);

	//* ����һ��http post ����,url�����������
	//* index: url+data ��md5
	//* ����ֵΪ�����md5(url+data) ������ʾ��һ��Ψһ������
	QString httpPost(const QString& url, const QByteArray &data);

Q_SIGNALS:
	//* http get ����õ��ظ�url������ĵ�ַ��data�ǻظ������ݣ����isEmpty���ʾ��������ˣ�
	void httpGetRspReady(QString url, QByteArray data);

	//* http post ����õ��ظ�postMD5��post��ʱ��url+���ݵ�md5��data�ǻظ������ݣ����isEmpty���ʾ��������ˣ�
	void httpPostRspReady(QString url, QString postMD5, QByteArray data);

protected:
	void onHttpGetRspProgress(qint64 bytesReceived, qint64 bytesTotal); //* http get �ظ�����
	void onHttpGetRspFinished(); //* http get �������

	void onHttpPostRspProgress(qint64 bytesReceived, qint64 bytesTotal); //* http post �ظ�����
	void onHttpPostRspFinished(); //* http post �������

private:
	void clearRp(QNetworkReply* rp);

private:
	QNetworkAccessManager*    mNetAccessManager;
	QMap<QString, bool>       mProcessingRq;                 //* ��ǰ���ڴ��������url ���Ƿ����ڴ���
	QMap<QString, QByteArray> mDownloadDataCache;            //* ���ݻ���url -> data
	QMap<QString, QString>    mRedirectMap;                  //* �ض���Ĺ�ϵ
};