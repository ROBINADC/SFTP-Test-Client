#include <atomic>
#include <chrono>
#include <cstdio>
#include <future>
#include <numeric>
#include <string>
#include <thread>
#include <vector>
#include <yaml-cpp/yaml.h>

#include "sftp.h"
#include "test.h"

/**
 * Atomic variable to control the running state of all workers.
 */
std::atomic_bool workerRun(true);

WorkerResult startWorker(TestArg arg, int tid) {
    int count = 0;
    double rt = 0.0;

    SftpArg sftpArg = {
        .ipaddr = arg.ipaddr,
        .port = arg.port,
        .username = arg.username,
        .password = arg.password,
        .enableDownload = arg.enableDownload,
    };

    std::string localFpStr;
    std::string remoteFpStr;

    if (arg.enableDownload) {
        sftpArg.enableDownload = true;
        sftpArg.localFilePath = arg.localTempfileDir + std::to_string(tid) + ".txt";
        sftpArg.remoteFilePath = arg.remoteTempfileDir + std::to_string(tid) + ".txt";
    }

    while (workerRun.load()) {
        auto start = std::chrono::steady_clock::now();
        int rc = sftpConn(sftpArg);
        if (rc != 0) {
            printf("Thread-%d terminated with sftpConn exit code %d\n", tid, rc);
            return {0, 0};
        }
        auto diff = std::chrono::steady_clock::now() - start;
        double elapse = std::chrono::duration<double, std::milli>(diff).count(); // ms

        rt += elapse;
        ++count;
    }

    printf("tid:%-2d rt:%.0fms count:%d\n", tid, rt, count);

    return {rt, count};
}

TestArg parseArg(const std::string &fileName) {
    // Defualt testing arguments
    TestArg arg = {
        .numWorkers = 10,
        .workerRunSeconds = 10,
        .ipaddr = "127.0.0.1",
        .port = 22,
        .username = "root",
        .password = "password",
        .enableDownload = false,
        .localTempfileDir = "/tmp/sftp/local/",
        .remoteTempfileDir = "/tmp/sftp/remote/",
    };

    YAML::Node node = YAML::LoadFile(fileName);
    if (node["numWorkers"]) {
        arg.numWorkers = node["numWorkers"].as<int>();
    }
    if (node["workerRunSeconds"]) {
        arg.workerRunSeconds = node["workerRunSeconds"].as<int>();
    }
    if (node["ipaddr"]) {
        arg.ipaddr = node["ipaddr"].as<std::string>();
    }
    if (node["port"]) {
        arg.port = node["port"].as<int>();
    }
    if (node["username"]) {
        arg.username = node["username"].as<std::string>();
    }
    if (node["password"]) {
        arg.password = node["password"].as<std::string>();
    }
    if (node["enableDownload"]) {
        arg.enableDownload = node["enableDownload"].as<bool>();
    }
    if (node["localTempfileDir"]) {
        arg.localTempfileDir = node["localTempfileDir"].as<std::string>();
    }
    if (node["remoteTempfileDir"]) {
        arg.remoteTempfileDir = node["remoteTempfileDir"].as<std::string>();
    }

    return arg;
}

int main(int argc, char const *argv[]) {
    TestArg arg = parseArg("config.yaml");

    int rc = sftpInit();
    if (rc != 0) {
        return 1;
    }

    std::vector<std::future<WorkerResult>> futures;
    for (int i = 1; i <= arg.numWorkers; ++i) {
        futures.push_back(std::async(startWorker, arg, i));
    }

    std::this_thread::sleep_for(std::chrono::seconds(arg.workerRunSeconds));
    workerRun.store(false);

    int count = 0;
    double rt = 0.0;
    for (auto &f : futures) {
        auto result = f.get();
        count += result.numRequests;
        rt += result.totalResponseTime;
    }

    double meanRt = rt / count;
    double tps = static_cast<double>(count) / arg.workerRunSeconds;

    printf("\nTest result\n");
    printf("ENABLE_DOWNLOAD: %d\n", arg.enableDownload);
    printf("Number of SFTP requests: %d\n", count);
    printf("Average response time: %.2fms\n", meanRt);
    printf("TPS: %.2f\n", tps);

    return 0;
}
