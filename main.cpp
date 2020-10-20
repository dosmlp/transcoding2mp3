#include <iostream>
#include <Global.h>
#include <EncodeTask.h>


int main(int argc, char* argv[]) {

    if (argc < 2) {
        return 0;
    }

    av_register_all();
    av_log_set_level(AV_LOG_QUIET);
    QThreadPool* thread_pool = QThreadPool::globalInstance();

    for (int i = 1; i < argc; ++i) {
        QString in_file_name = QString(argv[i]);
        EncodeTask* task = new EncodeTask(in_file_name);
        task->setAutoDelete(true);
        thread_pool->start(task);
    }

    thread_pool->waitForDone();
    return 0;
}

