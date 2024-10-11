# Parallel SSH/SFTP Test

Test the response time of parallel SSH or SFTP connections.

## Usage

1. Install dependencies

    For openEuler 22.03 or other similar distributions:

    ```bash
    dnf install make gcc-c++ libssh2 libssh2-devel yaml-cpp yaml-cpp-devel
    ```

2. Check configurations in `config.yaml`

    |Key|Description|Default Value|
    |:---|:---|:---|
    |numWorkers|the number of parallel threads to perform SFTP operations|10|
    |workerRunSeconds|running time of each worker in seconds|10|
    |workerNumRequests|total number of SSH requests to make for a worker. Set to `-1` for no limit|-1|
    |ipaddr|IP Address of SFTP server, can be local machine|127.0.0.1|
    |port|port that the SFTP server listen to|22|
    |username|username for SSH connection to SFTP server|root|
    |password|password for SSH connection to SFTP server|password|
    |numSftpPerSsh|the number of sequential SFTP connections within a single SSH session|1|
    |enableDownload|whether to download files in each SFTP connection. If set to `false`, the worker simply establishs and closes SFTP connections|false|
    |localTempfileDir|directory to store files downloaded by SFTP client. Should end with `/`|`/tmp/sftp/local/`|
    |remoteTempfileDir|directory to store files in SFTP server. Should end with `/`|`/tmp/sftp/remote/`|

3. Build

    ```bash
    make
    ```

4. Prepare temporary files

    When the test includes downloading files (enableDownload set to `true`), generate related directories and files first. Please check the command in `Makefile` before execute it.

    - For testing in local machine, execute `make prepare` in the server.
    - For testing remote machine, execute `make prepare` in both local and remote machines.

4. Run the test

    ```bash
    ./test
    ```

## SSH SFTP Configuration

For performance testing, use `internal-sftp` as SFTP implementation.

In `/etc/ssh/sshd_config`, change the below line:

```bash
Subsystem      sftp    /usr/libexec/openssh/sftp-server
```

to:

```bash
Subsystem       sftp    internal-sftp
```

For concurrent test using multiple threads, change the below line:

```bash
MaxStartups     10:30:100
```

to

```bash
MaxStartups     500:30:1000
```

Then, restart sshd service.

```bash
systemctl restart sshd
```

## Reference

- https://blog.csdn.net/CarryMee/article/details/130697889
- https://blog.csdn.net/weixin_43954810/article/details/135385864
- https://carta.tech/man-pages/man3/libssh2_session_supported_algs.3.html
