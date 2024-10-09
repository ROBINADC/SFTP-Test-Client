#!/bin/bash

# Simulate an SFTP request
#
# Usage
# Install Dependencies
#   dnf install expect
#
# When the test include uploading file, do the following:
# 1. Execute following commands to prepare temporary directory and file
#   dd if=/dev/zero of=1m.file bs=1M count=1
#   mkdir -p /tmp/sftp
# 2. Uncomment SFTP_PATH and SFTP_FILE
# 3. Uncomment two `send` instructions with the `expect` block

SFTP_HOST=127.0.0.1
SFTP_PORT=10022
SFTP_USER=root
SFTP_PASSWORD=${SFTP_PASSWORD:-password}
# SFTP_PATH=/tmp/sftp
# SFTP_FILE=1m.file

COUNT=10

for i in $(seq $COUNT); do
/usr/bin/expect <<-EOF
spawn sftp -o PreferredAuthentications=password -o PubKeyAuthentication=no -o PasswordAuthentication=yes -o Port=$SFTP_PORT $SFTP_USER@$SFTP_HOST
expect {
    "*yes/no" { send "yes\n"; exp_continue }
    "*password:" { send "$SFTP_PASSWORD\n" }
}
# expect "sftp>"
# send "cd $SFTP_PATH\n"
# expect "sftp>"
# send "put $SFTP_FILE\n"
expect "sftp>"
send "quit\n"
EOF
done
