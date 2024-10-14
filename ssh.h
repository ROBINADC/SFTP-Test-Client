#ifndef _SSH_H_
#define _SSH_H_

#include <libssh2.h>

#include <string>

using SshArg = struct _SshArg {
    std::string ipaddr;
    const int port;
    std::string username;
    std::string password;
    int numSftpPerSsh;
    bool enableDownload;
    std::string localFilePath;
    std::string remoteFilePath;
    int numCmdPerSsh;
    std::string command;
    bool renderOutput;
};

int sshInit();

int sshConn(SshArg &);

int sftpChannel(int sock, LIBSSH2_SESSION *session, SshArg &arg);

int cmdChannel(int sock, LIBSSH2_SESSION *session, SshArg &arg);

static int waitSocket(int sock, LIBSSH2_SESSION *session);

#endif
