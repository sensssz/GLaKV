//
// Created by Jiamin Huang on 5/16/16.
//

#ifndef GLAKV_THREAD_POOL_H
#define GLAKV_THREAD_POOL_H

#include "task.h"
#include "worker_thread.h"

#include <mutex>
#include <condition_variable>
#include <vector>

#include <boost/lockfree/queue.hpp>

using std::mutex;
using std::condition_variable;
using std::vector;
using boost::lockfree::queue;

class thread_pool {
private:
    queue<task> task_queue;
    vector<worker_thread> workers;
public:
    thread_pool(DB &db);
    thread_pool(DB &db, size_t pool_size);
    ~thread_pool();
    void submit_task_ref(const task &db_task);
    void submit_task(task db_task);
};


#endif //GLAKV_THREAD_POOL_H
