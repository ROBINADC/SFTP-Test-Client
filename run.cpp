#include <arpa/inet.h>
#include <libssh2.h>
#include <libssh2_sftp.h>

#include <atomic>
#include <chrono>
#include <future>
#include <iostream>
#include <numeric>
#include <thread>
#include <vector>

const int NUM_WORKERS = 10;
const int WORKER_RUN_SECONDS = 10;

const int ESTIMATED_RQ_PER_WORKER = static_cast<int>(static_cast<double>(WORKER_RUN_SECONDS) / 0.00001);



int main(int argc, char const *argv[]) {
    std::cout << "1" << std::endl;
    return 0;
}
