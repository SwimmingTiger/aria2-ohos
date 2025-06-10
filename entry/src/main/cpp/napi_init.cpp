#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/un.h>
#include "napi/native_api.h"


void aria2_init();


static napi_value Add(napi_env env, napi_callback_info info)
{
    size_t argc = 2;
    napi_value args[2] = {nullptr};

    napi_get_cb_info(env, info, &argc, args , nullptr, nullptr);

    napi_valuetype valuetype0;
    napi_typeof(env, args[0], &valuetype0);

    napi_valuetype valuetype1;
    napi_typeof(env, args[1], &valuetype1);

    double value0;
    napi_get_value_double(env, args[0], &value0);

    double value1;
    napi_get_value_double(env, args[1], &value1);

    napi_value sum;
    napi_create_double(env, value0 + value1, &sum);

    
    aria2_init();
    
    
    return sum;

}

void aria2_run()
{
    // 从 stdin 读取行，然后写入 stdout
    char buffer[1024];
    while (true) {
        ssize_t bytes_read = read(STDIN_FILENO, buffer, sizeof(buffer) - 1);
        if (bytes_read <= 0) {
            break; // 读取失败或 EOF
        }
        buffer[bytes_read] = '\0'; // 确保字符串以 null 结尾

        // 在这里可以处理读取到的命令
        // 例如，调用 aria2c 命令行工具来处理下载任务
        // 这里只是简单地将读取到的内容写入 stdout
        write(STDOUT_FILENO, buffer, bytes_read);
    }
}

void aria2_init()
{
    int pid = fork();
    if (pid != 0) {
        // 父进程
        return;
    }
    

    // 打开文件 /dev/shm/cn.hu60.aria2.stdin，把 stdin 重定向到这个文件
    int fd = open("/dev/shm/cn.hu60.aria2.stdin", O_RDONLY);
    if (fd < 0) {
        // 打开失败，可能是文件不存在
        // 这里可以选择创建文件或处理错误
        perror("Failed to open /dev/shm/cn.hu60.aria2.stdin");
        _exit(1);
    }
    // 重定向 stdin 到 fd
    if (dup2(fd, STDIN_FILENO) < 0) {
        perror("Failed to redirect stdin");
        close(fd);
        _exit(1);
    }
    close(fd); // 关闭原始文件描述符，因为已经重定向到 stdin

    // 打开文件 /dev/shm/cn.hu60.aria2.stdout，把 stdout 重定向到这个文件
    fd = open("/dev/shm/cn.hu60.aria2.stdout", O_WRONLY | O_CREAT | O_TRUNC, 0666);
    if (fd < 0) {
        // 打开失败，可能是文件不存在
        // 这里可以选择创建文件或处理错误
        perror("Failed to open /dev/shm/cn.hu60.aria2.stdout");
        _exit(1);
    }
    // 重定向 stdout 到 fd
    if (dup2(fd, STDOUT_FILENO) < 0) {
        perror("Failed to redirect stdout");
        close(fd);
        _exit(1);
    }
    close(fd); // 关闭原始文件描述符，因为已经重定向到 stdout

    // 打开文件 /dev/shm/cn.hu60.aria2.stderr，把 stderr 重定向到这个文件
    fd = open("/dev/shm/cn.hu60.aria2.stderr", O_WRONLY | O_CREAT | O_TRUNC, 0666);
    if (fd < 0) {
        // 打开失败，可能是文件不存在
        // 这里可以选择创建文件或处理错误
        perror("Failed to open /dev/shm/cn.hu60.aria2.stderr");
        _exit(1);
    }
    // 重定向 stderr 到 fd
    if (dup2(fd, STDERR_FILENO) < 0) {
        perror("Failed to redirect stderr");
        close(fd);
        _exit(1);
    }
    close(fd); // 关闭原始文件描述符，因为已经重定向到 stderr


    aria2_run();


    // fork后的进程一定要退出，不能返回
    _exit(0);
}

EXTERN_C_START
static napi_value Init(napi_env env, napi_value exports)
{
    napi_property_descriptor desc[] = {
        { "add", nullptr, Add, nullptr, nullptr, nullptr, napi_default, nullptr }
    };
    napi_define_properties(env, exports, sizeof(desc) / sizeof(desc[0]), desc);
    return exports;
}
EXTERN_C_END

static napi_module demoModule = {
    .nm_version = 1,
    .nm_flags = 0,
    .nm_filename = nullptr,
    .nm_register_func = Init,
    .nm_modname = "entry",
    .nm_priv = ((void*)0),
    .reserved = { 0 },
};

extern "C" __attribute__((constructor)) void RegisterEntryModule(void)
{
    napi_module_register(&demoModule);
}
