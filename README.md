# Parallel SFTP Test

Test the response time of parallel SFTP connections.

## Usage

1. Install dependencies

    For openEuler 22.03 or other similar distributions:

    ```bash
    dnf install make gcc-c++ libssh2 libssh2-devel yaml-cpp yaml-cpp-devel
    ```

2. Check configurations in `config.yaml`

    |Key|Description|
    |:---|:---|
    |numWorkers|the number of parallel threads to perform SFTP operations|
    |workerRunSeconds|running time of each worker|
    |ipaddr|IP Address of SFTP server, can be local machine|
    |port|port that the SFTP server listen to|
    |username|username for SSH connection to SFTP server|
    |password|password for SSH connection to SFTP server|
    |enableDownload|whether to download files in each SFTP connection. If set to `false`, the worker simply establishs and closes SFTP connections|
    |localTempfileDir|directory to store files downloaded by SFTP client. Should end with `/`|
    |remoteTempfileDir|directory to store files in SFTP server. Should end with `/`|

3. Build

    ```bash
    make
    ```

4. Prepare temporary files

    When the test includes downloading files (`ENABLE_DOWNLOAD` set to `true`), generate related directories and files first. Please check the command in `Makefile` before execute it.

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
