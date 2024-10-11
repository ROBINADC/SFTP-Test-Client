#include <atomic>
#include <chrono>
#include <csignal>
#include <cstdio>
#include <future>
#include <iostream>
#include <numeric>
#include <pthread.h>
#include <string>
#include <thread>
#include <vector>
#include <yaml-cpp/yaml.h>

#include "sftp.h"
#include "test.h"

using namespace std::chrono;

/**
 * Atomic variable to control the running state of all workers.
 */
std::atomic_bool workerRun(true);

WorkerResult runWorker(TestArg arg, int tid) {
    // Block SIGINT in worker thread
    sigset_t mask;
    sigaddset(&mask, SIGINT);
    pthread_sigmask(SIG_SETMASK, &mask, NULL);

    SftpArg sftpArg = {
        .ipaddr = arg.ipaddr,
        .port = arg.port,
        .username = arg.username,
        .password = arg.password,
        .numSftpPerSsh = arg.numSftpPerSsh,
        .enableDownload = arg.enableDownload,
    };

    if (arg.enableDownload) {
        sftpArg.enableDownload = true;
        sftpArg.localFilePath = arg.localTempfileDir + std::to_string(tid) + ".txt";
        sftpArg.remoteFilePath = arg.remoteTempfileDir + std::to_string(tid) + ".txt";
    }

    int count = 0;
    double rt = 0.0;

    while (workerRun.load() && count != arg.workerNumRequests) {
        auto start = steady_clock::now();
        int rc = sshConn(sftpArg);
        if (rc != 0) {
            printf("Thread-%d terminated with sshConn exit code %d\n", tid, rc);
            return {0, 0};
        }
        auto diff = steady_clock::now() - start;
        double elapse = duration<double, std::milli>(diff).count(); // ms

        rt += elapse;
        ++count;
    }

    printf("tid:%-2d rt:%.0fms ssh-count:%d sftp-count:%d\n", tid, rt, count, count * arg.numSftpPerSsh);

    return {rt, count};
}

TestArg parseArg(const std::string &fileName) {
    // Defualt testing arguments
    TestArg arg = {
        .numWorkers = 10,
        .workerRunSeconds = 10,
        .workerNumRequests = -1,
        .ipaddr = "127.0.0.1",
        .port = 22,
        .username = "root",
        .password = "password",
        .numSftpPerSsh = 1,
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
    if (node["workerNumRequests"]) {
        arg.workerNumRequests = node["workerNumRequests"].as<int>();
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
    if (node["numSftpPerSsh"]) {
        arg.numSftpPerSsh = node["numSftpPerSsh"].as<int>();
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

void sigIntHandler(int sigNum) {
    printf("Main thread received SIGINT\n");
    workerRun.store(false);
}

int main(int argc, char const *argv[]) {
    TestArg arg = parseArg("config.yaml");

    printf("Test configuration\n");
    std::cout << "Remote Info: " << arg.username << "@" << arg.ipaddr << ":" << arg.port << std::endl;
    std::cout << "Numer of workers: " << arg.numWorkers << std::endl;
    std::cout << "Worker run seconds: " << arg.workerRunSeconds << "s" << std::endl;
    std::cout << "Maximum number of requests per worker: " << arg.workerNumRequests << std::endl;
    std::cout << "Number of SFTP per SSH session: " << arg.numSftpPerSsh << std::endl;
    std::cout << "Enable download: " << std::boolalpha << arg.enableDownload << std::endl
              << std::endl;

    // Register singal handler
    std::signal(SIGINT, sigIntHandler);

    int rc = sshInit();
    if (rc != 0) {
        return 1;
    }

    std::vector<std::future<WorkerResult>> futures;
    for (int i = 1; i <= arg.numWorkers; ++i) {
        futures.push_back(std::async(runWorker, arg, i));
    }

    auto endTime = system_clock::now().time_since_epoch() + seconds(arg.workerRunSeconds);
    while (workerRun.load()) {
        std::this_thread::sleep_for(seconds(1));
        if (system_clock::now().time_since_epoch() >= endTime) {
            workerRun.store(false);
        }

        // Cancel the waiting loop if all workers finished their jobs
        bool ready = true;
        for (auto &f : futures) {
            if (f.wait_for(duration<double, std::milli>(1)) != std::future_status::ready) {
                ready = false;
                break;
            }
        }
        if (ready) {
            workerRun.store(false);
        }
    }

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
    printf("Number of SSH requests: %d\n", count);
    printf("Average SSH response time: %.2fms\n", meanRt);
    printf("SSH TPS: %.2f\n", tps);

    printf("\nNumber of SFTP requests: %d\n", count * arg.numSftpPerSsh);
    printf("Average SFTP response time: ");
    if (arg.numSftpPerSsh > 0) {
        printf("%.2fms\n", meanRt / arg.numSftpPerSsh);
    } else {
        printf("NaN\n");
    }
    printf("SFTP TPS: %.2f\n", tps * arg.numSftpPerSsh);
    return 0;
}
