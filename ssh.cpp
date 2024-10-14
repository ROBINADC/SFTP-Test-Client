#include "ssh.h"

#include <arpa/inet.h>
#include <libssh2.h>
#include <libssh2_sftp.h>

#include <fstream>
#include <iostream>

int sshInit() {
    int rc = libssh2_init(0);
    if (rc != 0) {
        fprintf(stderr, "libssh2 initialization failed\n");
    }
    return 0;
}

int sshConn(SshArg &arg) {
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

    // Turn on debug info when necessary (also need support from libssh2 library built with ENABLE_DEBUG_LOGGING=on)
    // libssh2_trace(session, LIBSSH2_TRACE_SOCKET | LIBSSH2_TRACE_TRANS | LIBSSH2_TRACE_KEX | LIBSSH2_TRACE_AUTH | LIBSSH2_TRACE_CONN | LIBSSH2_TRACE_ERROR);

    // Try different algorithms
    // libssh2_session_method_pref(session, LIBSSH2_METHOD_KEX, "diffie-hellman-group-exchange-sha256");
    // libssh2_session_method_pref(session, LIBSSH2_METHOD_HOSTKEY, "ssh-rsa");
    // libssh2_session_method_pref(session, LIBSSH2_METHOD_MAC_CS, "hmac-sha1");
    // libssh2_session_method_pref(session, LIBSSH2_METHOD_MAC_SC, "hmac-sha1");

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

    if (arg.numSftpPerSsh > 0) {
        sftpChannel(sock, session, arg);
    }

    if (arg.numCmdPerSsh > 0) {
        cmdChannel(sock, session, arg);
    }

    // Close SSH resources
    libssh2_session_disconnect(session, "Normal Shutdown");
    libssh2_session_free(session);
    close(sock);
    libssh2_exit();
    return 0;
}

int sftpChannel(int sock, LIBSSH2_SESSION *session, SshArg &arg) {
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

int cmdChannel(int sock, LIBSSH2_SESSION *session, SshArg &arg) {
    LIBSSH2_CHANNEL *channel;
    int rc;

    for (int i = 0; i < arg.numCmdPerSsh; ++i) {

        // Open channel
        while ((channel = libssh2_channel_open_session(session)) == NULL &&
               libssh2_session_last_error(session, NULL, NULL, 0) == LIBSSH2_ERROR_EAGAIN) {
            waitSocket(sock, session);
        }
        if (!channel) {
            fprintf(stderr, "libssh2_channel_open_session failed\n");
            return 1;
        }

        // Execute command on remote server
        while ((rc = libssh2_channel_exec(channel, arg.command.c_str())) == LIBSSH2_ERROR_EAGAIN) {
            waitSocket(sock, session);
        }
        if (rc) {
            fprintf(stderr, " libssh2_channel_exec failed\n");
            return 1;
        }

        // Read remote output
        do {
            char buffer[1024];
            rc = libssh2_channel_read(channel, buffer, sizeof(buffer));
            if (rc > 0) {
                // Postive return value means the actual number of bytes read
                if (arg.renderOutput) {
                    fprintf(stdout, "Read from remote:\n");
                    for (int i = 0; i < rc; ++i)
                        fputc(buffer[i], stdout);
                    fflush(stdout);
                }
            } else if (rc < 0) {
                // Negative return value means an error
                if (rc == LIBSSH2_ERROR_EAGAIN) {
                    waitSocket(sock, session);
                } else {
                    fprintf(stderr, "libssh2_channel_read returned %d\n", rc);
                    break;
                }
            }
        } while (rc > 0); // Exit on 0 (no payload data was read) or negative (failure)

        // Close channel
        int exitCode = 127;
        char *exitSignal = (char *)"none";

        while ((rc = libssh2_channel_close(channel)) == LIBSSH2_ERROR_EAGAIN) {
            waitSocket(sock, session);
        }
        if (rc == 0) {
            exitCode = libssh2_channel_get_exit_status(channel);
            libssh2_channel_get_exit_signal(channel, &exitSignal, NULL, NULL, NULL, NULL, NULL);
        }
        if (exitSignal) {
            fprintf(stdout, "\nGot signal: %s\n", exitSignal);
        }

        libssh2_channel_free(channel);
    }
    return 0;
}

static int waitSocket(int sock, LIBSSH2_SESSION *session) {
    fd_set fd;
    fd_set *fdWrite = NULL;
    fd_set *fdRead = NULL;
    int dir;

    FD_ZERO(&fd);
    FD_SET(sock, &fd);

    // Direction
    dir = libssh2_session_block_directions(session);

    if (dir & LIBSSH2_SESSION_BLOCK_INBOUND) {
        fdRead = &fd;
    }

    if (dir & LIBSSH2_SESSION_BLOCK_OUTBOUND) {
        fdWrite = &fd;
    }

    struct timeval timeout = {
        .tv_sec = 10,
        .tv_usec = 0,
    };
    int rc = select(sock + 1, fdRead, fdWrite, NULL, &timeout);

    return rc;
}
