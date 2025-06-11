#!/bin/sh

# CA证书路径
export SSL_CERT_FILE="/dev/shm/aria2.ca.crt"

# 创建下载文件夹
DOWNLOAD_DIR="$HOME/Download/cn.hu60.aria2"
mkdir -p "$DOWNLOAD_DIR"

# 如果 $PWD 不在下载文件夹内，就跳转到下载文件夹
if ! echo "$PWD/" | grep -q "^$DOWNLOAD_DIR/"; then
    cd "$DOWNLOAD_DIR"
fi

JOB_INDEX_FILE="/dev/shm/cn.hu60.aria2.job.index"

PID_FILE="/dev/shm/cn.hu60.aria2.$$.pid"
EXIT_CODE_FILE="/dev/shm/cn.hu60.aria2.$$.exitCode"
KILL_FLAG_FILE="/dev/shm/cn.hu60.aria2.$$.killFlag"

STDIN_FILE="/dev/shm/cn.hu60.aria2.$$.stdin"
STDOUT_FILE="/dev/shm/cn.hu60.aria2.$$.stdout"
STDERR_FILE="/dev/shm/cn.hu60.aria2.$$.stderr"

cat > "$STDIN_FILE" &
STDIN_PID="$!"

trap 'kill "$STDIN_PID" 2>/dev/null; rm "$STDIN_FILE"; touch "$KILL_FLAG_FILE"; sleep 1; rm "$KILL_FLAG_FILE" 2>/dev/null; exit 1' SIGINT SIGTERM

read_line() {
    # 检查行尾是否为换行符
    while ! [ -f "$1" ] || ! [ "$(tail -c1 "$1" | xxd -p)" = "0a" ]; do
        sleep 0.1
    done
    cat "$1"
}

JOB_FILE="$(read_line "$JOB_INDEX_FILE")"

{
    echo -ne "ERR\x02\x02$STDERR_FILE\x03\x03\x03\x03"
    echo -ne "OUT\x02\x02$STDOUT_FILE\x03\x03\x03\x03"
    echo -ne "CIN\x02\x02$STDIN_FILE\x03\x03\x03\x03"

    echo -ne "PID\x02\x02$PID_FILE\x03\x03\x03\x03"
    echo -ne "EXT\x02\x02$EXIT_CODE_FILE\x03\x03\x03\x03"
    echo -ne "KIL\x02\x02$KILL_FLAG_FILE\x03\x03\x03\x03"

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

trap 'kill "$STDIN_PID" 2>/dev/null; rm "$STDIN_FILE"; rm "$JOB_FILE"; touch "$KILL_FLAG_FILE"; sleep 1; rm "$KILL_FLAG_FILE" 2>/dev/null; exit 1' SIGINT SIGTERM

# 等待进程启动
while ! [ -f "$PID_FILE" ]; do
    sleep 0.1
done

JOB_PID="$(read_line "$PID_FILE")"

trap 'kill "$JOB_PID" 2>/dev/null; kill "$STDIN_PID" 2>/dev/null; touch "$KILL_FLAG_FILE"; sleep 1; rm "$KILL_FLAG_FILE" 2>/dev/null' SIGINT SIGTERM

dd_tail() {
    local offset=0
    while ! [ -f "$EXIT_CODE_FILE" ]; do
        dd if="$1" bs=1 skip="$offset" 2>"$1.dd"
        outputBytes="$(cat "$1.dd" | tail -n1 | awk '{print $1}')"
        offset=$((offset + outputBytes))
        sleep 0.1
    done
    if [ -f "$1.dd" ]; then
        rm "$1.dd"
    fi
    sleep 0.1
    dd if="$1" bs=1 skip="$offset" 2>/dev/null
}

dd_tail "$STDOUT_FILE" &
STDOUT_PID="$!"

dd_tail "$STDERR_FILE" >&2 &
STDERR_PID="$!"

# 等待任务进程退出
while ! [ -f "$EXIT_CODE_FILE" ]; do
    sleep 0.1
done

# 等待tail进程退出
wait "$STDOUT_PID"
wait "$STDERR_PID"

kill "$STDIN_PID" 2>/dev/null

rm "$STDIN_FILE"
rm "$JOB_FILE"

EXIT_CODE="$(read_line "$EXIT_CODE_FILE")"

exit "$EXIT_CODE"
