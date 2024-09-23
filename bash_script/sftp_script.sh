#!/bin/bash

# dnf install expect

# dd if=/dev/zero of=1m.file bs=1M count=1
# mkdir -p /tmp/sftp

SFTP_HOST=127.0.0.1
SFTP_PORT=2222
SFTP_USER=root
SFTP_PASSWORD=${SFTP_PASSWORD:-qweiop}
SFTP_PATH=/tmp/sftp
SFTP_FILE=1m.file

COUNT=10

for i in $(seq $COUNT); do
/usr/bin/expect <<-EOF
spawn sftp -oPort=$SFTP_PORT $SFTP_USER@$SFTP_HOST
expect {
    "*yes/no" { send "yes\n"; exp_continue }
    "*password:" { send "$SFTP_PASSWORD\n" }
}
expect "sftp>"
send "cd $SFTP_PATH\n"
expect "sftp>"
send "put $SFTP_FILE\n"
expect "sftp>"
send "quit\n"
EOF
done
