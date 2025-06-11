#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <dlfcn.h>
#include <limits.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/un.h>
#include "napi/native_api.h"


// aria2c.so 里的 main 函数
int (*g_aria2c_main)(int argc, char** argv);


void aria2_load(const char* libDir)
{
    g_aria2c_main = nullptr;

    // 设置 openssl legacy.so 加载路径
    setenv("OPENSSL_MODULES", libDir, 1);

    void* handle = dlopen("aria2c.so", RTLD_LAZY);
    if (!handle) {
        fprintf(stderr, "Failed to load aria2c.so: %s\n", dlerror());
        _exit(1);
    }

    g_aria2c_main = (int (*)(int, char**))dlsym(handle, "main");
    if (!g_aria2c_main) {
        fprintf(stderr, "Failed to find main function in aria2c.so: %s\n", dlerror());
        dlclose(handle);
        _exit(1);
    }
}

void aria2_run()
{
    int argc = 2;
    const char* argv[] = {
        "aria2c",
        "--help",
    };
    g_aria2c_main(argc, (char**)argv);
}

void aria2_init(const char* libDir)
{
    int fd;
    
    
    // 先重定向 stderr，以便发生错误时及时输出
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
    
    
    // 打开文件 /dev/shm/cn.hu60.aria2.stdin，把 stdin 重定向到这个文件
    for (;;) {
        fd = open("/dev/shm/cn.hu60.aria2.stdin", O_RDONLY);
        if (fd > 0) {
            // 打开成功
            break;
        }
        // 打开失败，等待1秒后重试
        sleep(1);
    }
    // 重定向 stdin 到 fd
    if (dup2(fd, STDIN_FILENO) < 0) {
        perror("Failed to redirect stdin");
        close(fd);
        _exit(1);
    }
    close(fd); // 关闭原始文件描述符，因为已经重定向到 stdin


    aria2_load(libDir);
    aria2_run();

    
    _exit(0);
}

static napi_value ets_aria2_init(napi_env env, napi_callback_info info)
{
    size_t argc = 1;
    napi_value args[1] = {nullptr};

    napi_get_cb_info(env, info, &argc, args , nullptr, nullptr);

    napi_valuetype valuetype0;
    napi_typeof(env, args[0], &valuetype0);

    char *libDir = (char*)malloc(PATH_MAX);
    napi_get_value_string_utf8(env, args[0], libDir, PATH_MAX, nullptr);
    
    napi_value success;
    
    int pid = fork();
    if (pid < 0) {
        // fork 失败
        napi_get_boolean(env, false, &success);
    }
    else if (pid == 0) {
        // 子进程
        aria2_init(libDir);
        // fork后的进程执行完后应该直接退出，不能返回
        _exit(0);
    }
    else {
        // fork 成功
        napi_get_boolean(env, true, &success);
    }
    
    return success;
}

EXTERN_C_START
static napi_value Init(napi_env env, napi_value exports)
{
    napi_property_descriptor desc[] = {
        { "aria2_init", nullptr, ets_aria2_init, nullptr, nullptr, nullptr, napi_default, nullptr }
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
