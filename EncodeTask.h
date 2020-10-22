#ifndef ENCODETASK_H
#define ENCODETASK_H

#include <Global.h>
#include <string>
#include <filesystem>
#include "align_malloc.h"

class EncodeTask : public QRunnable {
  public:
    explicit EncodeTask(const string &file_name);
    ~EncodeTask();
    void run();
  private:
    TaskContext* ctx;
    std::string source_file_name_;
};

#endif // ENCODETASK_H
