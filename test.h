#ifndef _TEST_H_
#define _TEST_H_

using WorkerResult = struct _WorkerResult {
    double totalResponseTime;
    int numRequests;
};

WorkerResult startWorker(int tid);

#endif