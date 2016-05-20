//
// Created by Jiamin Huang on 5/20/16.
//

#ifndef GLAKV_TASK_QUEUE_H
#define GLAKV_TASK_QUEUE_H

#include "task.h"

#include <list>
#include <mutex>

using std::list;
using std::mutex;

class task_queue {
private:
    mutex queue_mutex;
    list<task> queue;
public:
    task &offer(task &&db_task);
    task poll();
};


#endif //GLAKV_TASK_QUEUE_H
