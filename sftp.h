#ifndef _SFTP_H_
#define _SFTP_H_

#include <libssh2.h>

#include <string>

using SftpArg = struct _SftpArg {
    std::string ipaddr;
    const int port;
    std::string username;
    std::string password;
    int numSftpPerSsh;
    bool enableDownload;
    std::string localFilePath;
    std::string remoteFilePath;
};

int sftpInit();

int sftpConn(SftpArg &);

int sftpSession(int sock, LIBSSH2_SESSION *session, SftpArg &arg);

#endif
