#ifndef MY_SSH_H
#define MY_SSH_H

#include <libssh2.h>

#include <string>

/**
 * All arguments related to an SSH task.
 */
using SshArg = struct _SshArg {
    std::string ipaddr;         // IP Address of SFTP server
    const int port;             // port that the SFTP server listen to
    std::string username;       // username for SSH connection to SFTP server
    std::string password;       // password for SSH connection to SFTP server
    int numSftpPerSsh;          // the number of sequential SFTP connections within a single SSH session
    bool enableDownload;        // whether to download files in each SFTP connection
    std::string localFilePath;  // directory to store files downloaded by SFTP client
    std::string remoteFilePath; // directory to store files in SFTP server
    int numCmdPerSsh;           // the number of remote commands to execute within a single SSH session
    std::string command;        // the command to be executed
    bool renderOutput;          // whether to render remote output in local stdout
};

/**
 * Initialize the libssh2 functions.
 *
 * @return Return 0 if success, other values for error exit.
 */
int sshInit();

/**
 * Perform an SSH conenction, and do SFTP or execute commands specified by the argument.
 *
 * @param SshArg SSH task arguments.
 * @return Return 0 if success, other values for error exit.
 */
int sshConn(SshArg &);

#endif // MY_SSH_H
