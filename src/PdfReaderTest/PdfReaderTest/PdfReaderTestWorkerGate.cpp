#include "PdfReaderTestWorkerGate.h"
#include "../../PdfReader/PdfReader/PdfReaderTaskManager.h"

PdfReaderTestWorkerGate::PdfReaderTestWorkerGate() :
m_entered(new std::atomic<bool>(false)), m_timedOut(new std::atomic<bool>(false)),
m_release(new Semaphore())
{
    const std::shared_ptr<std::atomic<bool>> entered = m_entered;
    const std::shared_ptr<std::atomic<bool>> timedOut = m_timedOut;
    const std::shared_ptr<Semaphore> wake = m_release;
    std::shared_ptr<CTask> task(new PdfReaderTask(
        [entered, timedOut, wake](const std::atomic<bool>& exit) {
            entered->store(true);
            if (!exit.load() && !wake->wait(5000))
            {
                timedOut->store(true);
            }
        }, [entered, timedOut](const std::string&) {
            entered->store(true);
            timedOut->store(true);
        }));
    PdfReaderTaskManager::instance().postLogic(std::shared_ptr<CTask>(new PdfReaderTask(
        [task](const std::atomic<bool>&) { PdfReaderTaskManager::instance().postWork(task); },
        [entered, timedOut](const std::string&) {
            entered->store(true);
            timedOut->store(true);
        })));
}

PdfReaderTestWorkerGate::~PdfReaderTestWorkerGate()
{
    release();
}

void PdfReaderTestWorkerGate::release()
{
    m_release->signal();
}