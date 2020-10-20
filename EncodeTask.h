#ifndef ENCODETASK_H
#define ENCODETASK_H

#include <Global.h>

class EncodeTask : public QRunnable {
  public:
    explicit EncodeTask(const QString& file_name);
    ~EncodeTask();
    void run();
  private:
    TaskContext* ctx;
    QString source_file_name_;
};

#endif // ENCODETASK_H
