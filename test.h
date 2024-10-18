#ifndef _TEST_H_
#define _TEST_H_

#include <string>

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
    double totalResponseTime;
    int numRequests;
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

#endif
