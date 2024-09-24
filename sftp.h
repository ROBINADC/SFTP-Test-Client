#ifndef _SFTP_H_
#define _SFTP_H_

using SftpArg = struct _SfrpArg {
    const char *ipaddr;
    const int port;
    const char *username;
    const char *password;
    bool enableDownload;
    const char *localFilePath;
    const char *remoteFilePath;
};

int sftpInit();

int sftpConn(SftpArg &);

#endif