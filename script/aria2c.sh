#!/bin/sh
JOB_INDEX_FILE="/dev/shm/cn.hu60.aria2.job.index"

PID_FILE="/dev/shm/cn.hu60.aria2.$$.pid"
EXIT_CODE_FILE="/dev/shm/cn.hu60.aria2.$$.exitCode"

STDIN_FILE="/dev/shm/cn.hu60.aria2.$$.stdin"
STDOUT_FILE="/dev/shm/cn.hu60.aria2.$$.stdout"
STDERR_FILE="/dev/shm/cn.hu60.aria2.$$.stderr"

touch "$STDIN_FILE"

JOB_FILE="$(cat "$JOB_INDEX_FILE")"
{
    echo -ne "ERR\x02\x02$STDERR_FILE\x03\x03\x03\x03"
    echo -ne "OUT\x02\x02$STDOUT_FILE\x03\x03\x03\x03"
    echo -ne "CIN\x02\x02$STDIN_FILE\x03\x03\x03\x03"

    echo -ne "PID\x02\x02$PID_FILE\x03\x03\x03\x03"
    echo -ne "EXT\x02\x02$EXIT_CODE_FILE\x03\x03\x03\x03"

    echo -ne "PWD\x02\x02$PWD\x03\x03\x03\x03"

    # 写入参数
    echo -ne "ARG\x02\x02sh $0\x03\x03\x03\x03"
    for arg in "$@"; do
    echo -ne "ARG\x02\x02$arg\x03\x03\x03\x03"
    done

    # 写入环境变量
    env | while read line; do
        echo -ne "ENV\x02\x02$line\x03\x03\x03\x03"
    done

    # 写入结束标志
    echo -ne "\x04\x04\x04\x04"
} > "$JOB_FILE"

sleep 2

cat "$STDOUT_FILE"
cat "$STDERR_FILE"
