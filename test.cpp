#include <pthread.h>
#include <yaml-cpp/yaml.h>

#include <atomic>
#include <chrono>
#include <csignal>
#include <cstdio>
#include <future>
#include <numeric>
#include <string>
#include <thread>
#include <vector>

#include "ssh.h"

using namespace std::chrono;

/**
 * All arguments related to the test.
 */
using TestArg = struct _TestArg {
    int numWorkers;                // the number of worker threads
    int workerRunSeconds;          // running time of each worker in seconds
    int workerNumRequests;         // maximum number of SSH requests to make for a worker
    std::string ipaddr;            // IP Address of SFTP server
    int port;                      // port that the SFTP server listen to
    std::string username;          // username for SSH connection to SFTP server
    std::string password;          // password for SSH connection to SFTP server
    int numSftpPerSsh;             // the number of sequential SFTP connections within a single SSH session
    bool enableDownload;           // whether to download files in each SFTP connection
    std::string localTempfileDir;  // directory to store files downloaded by SFTP client
    std::string remoteTempfileDir; // directory to store files in SFTP server
    int numCmdPerSsh;              // the number of remote commands to execute within a single SSH session
    std::string command;           // the command to be executed
    bool renderOutput;             // whether to render remote output in local stdout
};

/**
 * Result returned by workers.
 */
using WorkerResult = struct _WorkerResult {
    double totalResponseTime; // total elaspe time of all SSH connections
    int numRequests;          // total number of SSH requests
};

/**
 * Task to run in each thread: establish SSH connection with servers, perform SFTP requests.
 *
 * @param arg TestArg
 * @param tid thread identity
 * @return WorkerResult
 */
WorkerResult runWorker(TestArg arg, int tid);

/**
 * Parse testing arguments from a local YAML file.
 *
 * @param fileName path to YAML config file
 * @return TestArg for testing arguments
 */
TestArg parseArg(const std::string &fileName);

/**
 * Signal handler callback function.
 *
 * @param sigNum caught signal number
 */
void sigIntHandler(int sigNum);

/**
 * Atomic variable to control the running state of all workers.
 */
static std::atomic_bool workerRun(true);

WorkerResult runWorker(TestArg arg, int tid) {
    // Block SIGINT in worker thread
    sigset_t mask;
    sigaddset(&mask, SIGINT);
    pthread_sigmask(SIG_SETMASK, &mask, NULL);

    SshArg sshArg = {
        .ipaddr = arg.ipaddr,
        .port = arg.port,
        .username = arg.username,
        .password = arg.password,
        .numSftpPerSsh = arg.numSftpPerSsh,
        .enableDownload = arg.enableDownload,
        .localFilePath = "",
        .remoteFilePath = "",
        .numCmdPerSsh = arg.numCmdPerSsh,
        .command = arg.command,
        .renderOutput = arg.renderOutput,
    };

    if (arg.enableDownload) {
        sshArg.enableDownload = true;
        sshArg.localFilePath = arg.localTempfileDir + std::to_string(tid) + ".txt";
        sshArg.remoteFilePath = arg.remoteTempfileDir + std::to_string(tid) + ".txt";
    }

    int count = 0;   // number of SSH connections
    double rt = 0.0; // total elapse time of SSH connections

    while (workerRun.load() && count != arg.workerNumRequests) {
        auto start = steady_clock::now();
        int rc = sshConn(sshArg);
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
        .numCmdPerSsh = 0,
        .command = "echo ABC",
        .renderOutput = false,
    };

    auto root = YAML::LoadFile(fileName);
    auto workerNode = root["worker"];
    if (workerNode) {
        if (workerNode["numWorkers"]) {
            arg.numWorkers = workerNode["numWorkers"].as<int>();
        }
        if (workerNode["workerRunSeconds"]) {
            arg.workerRunSeconds = workerNode["workerRunSeconds"].as<int>();
        }
        if (workerNode["workerNumRequests"]) {
            arg.workerNumRequests = workerNode["workerNumRequests"].as<int>();
        }
    }

    auto sshNode = root["ssh"];
    if (sshNode) {
        if (sshNode["ipaddr"]) {
            arg.ipaddr = sshNode["ipaddr"].as<std::string>();
        }
        if (sshNode["port"]) {
            arg.port = sshNode["port"].as<int>();
        }
        if (sshNode["username"]) {
            arg.username = sshNode["username"].as<std::string>();
        }
        if (sshNode["password"]) {
            arg.password = sshNode["password"].as<std::string>();
        }
    }

    auto sftpNode = root["sftp"];
    if (sftpNode) {
        if (sftpNode["numSftpPerSsh"]) {
            arg.numSftpPerSsh = sftpNode["numSftpPerSsh"].as<int>();
        }
        if (sftpNode["enableDownload"]) {
            arg.enableDownload = sftpNode["enableDownload"].as<bool>();
        }
        if (sftpNode["localTempfileDir"]) {
            arg.localTempfileDir = sftpNode["localTempfileDir"].as<std::string>();
        }
        if (sftpNode["remoteTempfileDir"]) {
            arg.remoteTempfileDir = sftpNode["remoteTempfileDir"].as<std::string>();
        }
    }

    auto cmdNode = root["cmd"];
    if (cmdNode) {
        if (cmdNode["numCmdPerSsh"]) {
            arg.numCmdPerSsh = cmdNode["numCmdPerSsh"].as<int>();
        }
        if (cmdNode["command"]) {
            arg.command = cmdNode["command"].as<std::string>();
        }
        if (cmdNode["renderOutput"]) {
            arg.renderOutput = cmdNode["renderOutput"].as<bool>();
        }
    }

    return arg;
}

void sigIntHandler(int sigNum) {
    // When main thread receives SIGINT, tell other threads to stop
    printf("Main thread received SIGINT\n");
    workerRun.store(false);
}

int main(int argc, char const *argv[]) {
    TestArg arg = parseArg("config.yaml");

    printf("Test configuration\n");
    printf("Worker:\n");
    printf("    Numer of workers: %d\n", arg.numWorkers);
    printf("    Worker run seconds: %ds\n", arg.workerRunSeconds);
    printf("    Maximum number of requests per worker: %d\n", arg.workerNumRequests);
    printf("SSH:\n");
    printf("    Remote Info: %s@%s:%d\n", arg.username.c_str(), arg.ipaddr.c_str(), arg.port);
    printf("SFTP:\n");
    printf("    Number of SFTP per SSH session: %d\n", arg.numSftpPerSsh);
    printf("    Enable download: %s\n", arg.enableDownload ? "true" : "false");
    if (arg.numCmdPerSsh > 0) {
        printf("CMD:\n"
               "    Number of remote commands per SSH session: %d\n"
               "    Command: %s\n"
               "    Render remote output: %s\n",
               arg.numCmdPerSsh, arg.command.c_str(), arg.renderOutput ? "true" : "false");
    }

    // Register singal handler
    std::signal(SIGINT, sigIntHandler);

    int rc = sshInit();
    if (rc != 0) {
        return 1;
    }

    // Start all workers
    std::vector<std::future<WorkerResult>> futures;
    for (int i = 1; i <= arg.numWorkers; ++i) {
        futures.push_back(std::async(runWorker, arg, i));
    }

    // Run until specified time
    auto endTime = system_clock::now().time_since_epoch() + seconds(arg.workerRunSeconds);
    while (workerRun.load()) {
        std::this_thread::sleep_for(seconds(1));
        if (system_clock::now().time_since_epoch() >= endTime) {
            workerRun.store(false);
        }

        // Also, cancel the waiting loop if all workers finished their jobs
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

    // Summarize and print running statistics
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
