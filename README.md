# Parallel SFTP Test

Test the response time of parallel SFTP connections.

## Usage

1. Install dependencies

    ```
    dnf install make gcc-c++ libssh2-devel yaml-cpp
    ```

2. Check configurations in `test.cpp`

    - NUM_WORKERS: the number of parallel threads to perform SFTP operations
    - WORKER_RUN_SECONDS: running time of each worker
    - IPADDR: IP Address of SFTP server, can be local machine
    - PORT: port that the SFTP server listen to
    - USERNAME: username for SSH connection to SFTP server
    - PASSWORD: password for SSH connection to SFTP server
    - ENABLE_DOWNLOAD: whether to download files in each SFTP connection. If set to `false`, the worker simply establishs and closes SFTP connections
    - LOCAL_TEMPFILE_DIR: directory to store files downloaded by SFTP client. Should end with `/`
    - REMOTE_TEMPFILE_DIR: directory to store files in SFTP server. Should end with `/`

3. Build

    ```
    make
    ```

4. Prepare temporary files

    When the test includes downloading files (`ENABLE_DOWNLOAD` set to `true`), generate temporary files in SFTP server first.

    For testing in local machine, use below provision command. Please check the command in `Makefile` first before execute it.

    ```
    make prepare
    ```

4. Run the test

    ```
    ./test
    ```

## SSH SFTP Configuration

For performance testing, use `internal-sftp` as SFTP implementation.

In `/etc/ssh/sshd_config`, change the below line:

```
Subsystem      sftp    /usr/libexec/openssh/sftp-server
```

to:

```
Subsystem       sftp    internal-sftp
```

Then, restart sshd service.

```
systemctl restart sshd
```

## Reference

- https://blog.csdn.net/CarryMee/article/details/130697889
- https://blog.csdn.net/weixin_43954810/article/details/135385864