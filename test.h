#ifndef _TEST_H_
#define _TEST_H_

#include <string>

using WorkerResult = struct _WorkerResult {
    double totalResponseTime;
    int numRequests;
};

using TestArg = struct _TestArg {
    int numWorkers;
    int workerRunSeconds;
    int workerNumRequests;
    std::string ipaddr;
    int port;
    std::string username;
    std::string password;
    int numSftpPerSsh;
    bool enableDownload;
    std::string localTempfileDir;
    std::string remoteTempfileDir;
    int numCmdPerSsh;
    std::string command;
    bool renderOutput;
};

TestArg parseArg(const std::string &fileName);

WorkerResult runWorker(TestArg arg, int tid);

void sigIntHandler(int sigNum);

#endif
