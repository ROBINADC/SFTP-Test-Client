#include <arpa/inet.h>
#include <libssh2.h>
#include <libssh2_sftp.h>

#include <fstream>
#include <iostream>

#include "sftp.h"

int sshInit() {
    int rc = libssh2_init(0);
    if (rc != 0) {
        fprintf(stderr, "libssh2 initialization failed\n");
    }
    return 0;
}

int sshConn(SftpArg &arg) {
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

    // add print
    // rc = libssh2_session_supported_algs(session,
    //                                     LIBSSH2_METHOD_CRYPT_CS,
    //                                     &algorithms);
    // if ()
    // if (rc > 0) {
    //     /* the call succeeded, do sth. with the list of algorithms
    //        (e.g. list them)... */
    //     printf("Supported symmetric algorithms:\n");
    //     for (i = 0; i < rc; i++)
    //         printf("\t%s\n", algorithms[i]);

    //     /* ... and free the allocated memory when not needed anymore */
    //     libssh2_free(session, algorithms);
    // } else {
    //     /* call failed, error handling */
    // }

    // add modify
    libssh2_session_method_pref(session, LIBSSH2_METHOD_MAC_CS, "hmac-sha1");
    libssh2_session_method_pref(session, LIBSSH2_METHOD_MAC_SC, "hmac-sha1");

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

    // add print
    std::cout << "LIBSSH2_METHOD_KEX: " << libssh2_session_methods(session, LIBSSH2_METHOD_KEX) << std::endl;
    std::cout << "LIBSSH2_METHOD_HOSTKEY: " << libssh2_session_methods(session, LIBSSH2_METHOD_HOSTKEY) << std::endl;
    std::cout << "LIBSSH2_METHOD_CRYPT_CS: " << libssh2_session_methods(session, LIBSSH2_METHOD_CRYPT_CS) << std::endl;
    std::cout << "LIBSSH2_METHOD_CRYPT_SC: " << libssh2_session_methods(session, LIBSSH2_METHOD_CRYPT_SC) << std::endl;
    std::cout << "LIBSSH2_METHOD_MAC_CS: " << libssh2_session_methods(session, LIBSSH2_METHOD_MAC_CS) << std::endl;
    std::cout << "LIBSSH2_METHOD_MAC_SC: " << libssh2_session_methods(session, LIBSSH2_METHOD_MAC_SC) << std::endl;
    std::cout << "LIBSSH2_METHOD_COMP_CS: " << libssh2_session_methods(session, LIBSSH2_METHOD_COMP_CS) << std::endl;
    std::cout << "LIBSSH2_METHOD_COMP_SC: " << libssh2_session_methods(session, LIBSSH2_METHOD_COMP_SC) << std::endl;
    std::cout << "LIBSSH2_METHOD_LANG_CS: " << libssh2_session_methods(session, LIBSSH2_METHOD_LANG_CS) << std::endl;
    std::cout << "LIBSSH2_METHOD_LANG_SC: " << libssh2_session_methods(session, LIBSSH2_METHOD_LANG_SC) << std::endl;

    sftpSession(sock, session, arg);

    // Close SSH resources
    libssh2_session_disconnect(session, "Normal Shutdown");
    libssh2_session_free(session);
    close(sock);
    libssh2_exit();
    return 0;
}

int sftpSession(int sock, LIBSSH2_SESSION *session, SftpArg &arg) {
    if (arg.numSftpPerSsh <= 0) {
        return 0;
    }

    for (int i = 0; i < arg.numSftpPerSsh; ++i) {
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
    }

    return 0;
}
