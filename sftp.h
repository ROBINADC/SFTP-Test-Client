#ifndef _SFTP_H_
#define _SFTP_H_

#include <string>

using SftpArg = struct _SfrpArg {
    std::string ipaddr;
    const int port;
    std::string username;
    std::string password;
    bool enableDownload;
    std::string localFilePath;
    std::string remoteFilePath;
};

int sftpInit();

int sftpConn(SftpArg &);

#endif