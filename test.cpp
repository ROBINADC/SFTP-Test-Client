#include <atomic>
#include <chrono>
#include <future>
#include <numeric>
#include <string>
#include <thread>
#include <vector>
#include <cstdio>

#include "sftp.h"
#include "test.h"

const int NUM_WORKERS = 10;
const int WORKER_RUN_SECONDS = 10;
const char *IPADDR = "127.0.0.1";
const int PORT = 2222;
const char *USERNAME = "root";
const char *PASSWORD = "qweiop";
const bool ENABLE_DOWNLOAD = false;
const char *LOCAL_TEMPFILE_DIR = "/tmp/sftp/local/";
const char *REMOTE_TEMPFILE_DIR = "/tmp/sftp/remote/";

/**
 * Atomic variable to control the running state of all workers.
 */
std::atomic_bool workerRun(true);

std::mutex mtx;

WorkerResult startWorker(int tid) {
    int count = 0;
    double rt = 0.0;

    SftpArg sftpArg = {
        .ipaddr = IPADDR,
        .port = PORT,
        .username = USERNAME,
        .password = PASSWORD,
        .enableDownload = ENABLE_DOWNLOAD,
    };

    std::string localFpStr;
    std::string remoteFpStr;

    if (ENABLE_DOWNLOAD) {
        sftpArg.enableDownload = true;
        localFpStr = std::string(LOCAL_TEMPFILE_DIR) + std::to_string(tid) + std::string(".txt");
        remoteFpStr = std::string(REMOTE_TEMPFILE_DIR) + std::to_string(tid) + std::string(".txt");
        sftpArg.localFilePath = localFpStr.data();
        sftpArg.remoteFilePath = remoteFpStr.data();
    }

    while (workerRun.load()) {
        auto start = std::chrono::steady_clock::now();
        sftpConn(sftpArg);
        auto diff = std::chrono::steady_clock::now() - start;
        double elapse = std::chrono::duration<double, std::milli>(diff).count(); // ms

        rt += elapse;
        ++count;
    }

    printf("tid:%-2d rt:%.0fms count:%d\n", tid, rt, count);

    return {rt, count};
}

int main(int argc, char const *argv[]) {
    int rc = sftpInit();
    if (rc != 0) {
        return 1;
    }

    std::vector<std::future<WorkerResult>> futures;
    for (int i = 1; i <= NUM_WORKERS; ++i) {
        futures.push_back(std::async(startWorker, i));
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

    double meanRt = rt / count;
    double tps = static_cast<double>(count) / WORKER_RUN_SECONDS;

    printf("\nTest result\n");
    printf("ENABLE_DOWNLOAD: %d\n", ENABLE_DOWNLOAD);
    printf("Number of SFTP requests: %d\n", count);
    printf("Average response time: %.2fms\n", meanRt);
    printf("TPS: %.2f\n", tps);

    return 0;
}
