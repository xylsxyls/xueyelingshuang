#include <QtGui> 
#include <QtCore> 

class Thread : public QThread
{
	Q_OBJECT
public:
	Thread(QObject *parent = 0);
	~Thread();

signals:
	void UpdateSignal(int num);
	void error(int dialogId);

public slots:
	void ResetSlot();

protected:
	void run();

private:
	int number;
};