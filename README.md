# Parallel SSH/SFTP Test

Client code to test the response time of parallel SSH or SFTP connections.

## Usage

1. Install dependencies

    For openEuler 22.03 or other similar distributions:

    ```bash
    dnf install make gcc-c++ libssh2 libssh2-devel yaml-cpp yaml-cpp-devel
    ```

2. Check client configuration in `config.yaml`

3. Build

    ```bash
    make
    ```

4. Prepare temporary files

    When the test includes downloading files (enableDownload set to `true`), generate related directories and files first. Please check the command in `Makefile` before execute it.

    - For testing in local machine, execute `make prepare` in the server.
    - For testing remote machine, execute `make prepare` in both local and remote machines.

5. Run the test

    ```bash
    ./test
    ```

## Client Configuration

|Class|Key|Description|Default Value|
|:---|:---|:---|:---|
|worker|numWorkers|the number of parallel threads to perform SFTP operations|10|
|worker|workerRunSeconds|running time of each worker in seconds|10|
|worker|workerNumRequests|total number of SSH requests to make for a worker. Set to `-1` for no limit|-1|
|ssh|ipaddr|IP Address of SFTP server, can be local machine|127.0.0.1|
|ssh|port|port that the SFTP server listen to|22|
|ssh|username|username for SSH connection to SFTP server|root|
|ssh|password|password for SSH connection to SFTP server|password|
|sftp|numSftpPerSsh|the number of sequential SFTP connections within a single SSH session|1|
|sftp|enableDownload|whether to download files in each SFTP connection. If set to `false`, the worker simply establishs and closes SFTP connections|false|
|sftp|localTempfileDir|directory to store files downloaded by SFTP client. Should end with `/`|`/tmp/sftp/local/`|
|sftp|remoteTempfileDir|directory to store files in SFTP server. Should end with `/`|`/tmp/sftp/remote/`|
|cmd|numCmdPerSsh|the number of remote commands to execute within a single SSH session|0|
|cmd|command|the command to be executed|`echo ABC`|
|cmd|renderOutput|whether to render remote output in local stdout. Notice the multi-threaded rendering is not guaranteed thread-safe|false|

## SSH/SFTP Server Configuration

For performance testing, use `internal-sftp` as SFTP implementation.

In `/etc/ssh/sshd_config`, change the below line:

```bash
Subsystem      sftp    /usr/libexec/openssh/sftp-server
```

to:

```bash
Subsystem       sftp    internal-sftp
```

For concurrent test using multiple threads, to avoid probabilistic connect rejection, change the below line:

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

- [libssh2 SFTP example 1](https://blog.csdn.net/CarryMee/article/details/130697889)
- [libssh2 SFTP example 2](https://blog.csdn.net/weixin_43954810/article/details/135385864)
- [libssh2_session_supported_algs](https://carta.tech/man-pages/man3/libssh2_session_supported_algs.3.html)
- [libssh2 execute remote command example](https://blog.csdn.net/yanghangwww/article/details/113071436)
