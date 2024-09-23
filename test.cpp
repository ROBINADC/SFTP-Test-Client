#include <atomic>
#include <chrono>
#include <future>
#include <iostream>
#include <numeric>
#include <thread>
#include <vector>

#include "test.h"
#include "sftp.h"

const int NUM_WORKERS = 10;
const int WORKER_RUN_SECONDS = 10;

SftpArg sftpArg = {
    .ipaddr = "127.0.0.1",
    .port = 2222,
    .username = "root",
    .password = "qweiop"
};

std::atomic_bool workerRun(true);

WorkerResult startWorker() {
    int count = 0;
    double rt = 0.0;

    while (workerRun.load()) {
        auto start = std::chrono::steady_clock::now();
        sftpConn(sftpArg);
        auto diff = std::chrono::steady_clock::now() - start;
        double elapse = std::chrono::duration<double, std::milli>(diff).count(); // ms

        rt += elapse;
        ++count;
    }
    std::cout << rt << " " << count << std::endl;

    return {rt, count};
}



int main(int argc, char const *argv[]) {
    int rc = sftpInit();
    if (rc != 0) {
        return 1;
    }

    std::vector<std::future<WorkerResult>> futures;
    for (int i = 1; i <= NUM_WORKERS; ++i) {
        futures.push_back(std::async(startWorker));
    }

    std::this_thread::sleep_for(std::chrono::seconds(WORKER_RUN_SECONDS));
    workerRun.store(false);

    int count = 0;
    double rt = 0.0;
    for (auto &f : futures) {
        auto result = f.get();
        count += result.numRequests;
        rt += result.totalResponseTime;
    }

    std::cout << "Number of SFTP requests: " << count << std::endl;
    
    double meanRt = rt / count;
    std::cout << "Average response time: " << meanRt << " ms" << std::endl;

    return 0;
}
