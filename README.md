# Parallel SFTP Test

## Usage

1. Install dependencies

    ```
    dnf install make gcc-c++ libssh2-devel
    ```

2. Check configurations in `test.cpp`

3. Build

    ```
    make
    ```

4. Prepare temporary files

    ```
    make prepare
    ```

4. Run

    ```
    ./test
    ```

## SSH SFTP Configuration

In `/etc/ssh/sshd_config`,

```
#Subsystem      sftp    /usr/libexec/openssh/sftp-server
Subsystem       sftp    internal-sftp
```

systemctl restart sshd

## Reference

https://blog.csdn.net/CarryMee/article/details/130697889

https://blog.csdn.net/weixin_43954810/article/details/135385864