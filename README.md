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