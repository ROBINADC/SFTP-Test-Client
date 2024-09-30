#include <arpa/inet.h>
#include <libssh2.h>
#include <libssh2_sftp.h>

#include <iostream>
#include <fstream>

#include "sftp.h"

int sftpInit() {
    int rc = libssh2_init(0);
    if (rc != 0) {
        fprintf(stderr, "libssh2 initialization failed\n");
    }
    return 0;
}

int sftpConn(SftpArg &arg) {
    int rc;

    // Establish SSH connection
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == -1) {
        fprintf(stderr, "socket failed\n");
        libssh2_exit();
        return 1;
    }

    struct sockaddr_in sin;
    sin.sin_family = AF_INET;
    sin.sin_port = htons(arg.port);
    sin.sin_addr.s_addr = inet_addr(arg.ipaddr.c_str());
    if (connect(sock, (struct sockaddr *)(&sin), sizeof(struct sockaddr_in)) != 0) {
        fprintf(stderr, "connect failed\n");
        close(sock);
        libssh2_exit();
        return 1;
    }

    // Setup SSH session
    LIBSSH2_SESSION *session = libssh2_session_init();
    if (!session) {
        fprintf(stderr, "libssh2_session_init failed\n");
        close(sock);
        libssh2_exit();
        return 1;
    }

    rc = libssh2_session_handshake(session, sock);
    if (rc != 0) {
        fprintf(stderr, "libssh2_session_handshake failed\n");
        close(sock);
        libssh2_session_free(session);
        libssh2_exit();
        return 1;
    }

    rc = libssh2_userauth_password(session, arg.username.c_str(), arg.password.c_str());
    if (rc != 0) {
        fprintf(stderr, "libssh2_userauth_password failed\n");
        close(sock);
        libssh2_session_free(session);
        libssh2_exit();
        return 1;
    }

    // Setup SFTP
    LIBSSH2_SFTP *sftp = libssh2_sftp_init(session);
    if (!sftp) {
        fprintf(stderr, "libssh2_sftp_init failed\n");
        close(sock);
        libssh2_session_free(session);
        libssh2_exit();
        return 1;
    }

    // Download remote file to local
    if (arg.enableDownload) {
        LIBSSH2_SFTP_HANDLE *handle = libssh2_sftp_open(sftp, arg.remoteFilePath.c_str(), LIBSSH2_FXF_READ, 0);
        if (!handle) {
            fprintf(stderr, "libssh2_sftp_open failed\n");
            libssh2_sftp_shutdown(sftp);
            close(sock);
            libssh2_session_free(session);
            libssh2_exit();
            return 1;
        }

        std::ofstream fout(arg.localFilePath, std::ios::out | std::ios::binary);
        if (!fout.good()) {
            fprintf(stderr, "Failed to open local file for writing\n");
            libssh2_sftp_close(handle);
            libssh2_sftp_shutdown(sftp);
            close(sock);
            libssh2_session_free(session);
            libssh2_exit();
            return 1;
        }
        char buffer[1048576]; // buffer size
        int len = 0;
        while ((len = libssh2_sftp_read(handle, buffer, sizeof(buffer))) > 0) {
            fout.write(buffer, len);
        }
        fout.close();
        libssh2_sftp_close(handle);
    }

    // Close SFTP session and other resources
    libssh2_sftp_shutdown(sftp);
    libssh2_session_disconnect(session, "Normal Shutdown");
    libssh2_session_free(session);
    close(sock);
    libssh2_exit();
    return 0;
}
