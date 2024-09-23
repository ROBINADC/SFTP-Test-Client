#ifndef _SFTP_H_
#define _SFTP_H_

typedef struct _SftpArg {
    const char *ipaddr;
    const int port;
    const char *username;
    const char *password;
    bool enableDownload;
    char *localFilePath;
    char *remoteFilePath;
} SftpArg;

int sftpInit();

int sftpConn(SftpArg &);

#endif