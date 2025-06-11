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
#include <sys/wait.h>

#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <algorithm> 
#include <cctype>
#include <locale>

#include "napi/native_api.h"


// aria2c.so 里的 main 函数
int (*g_aria2c_main)(int argc, char** argv);


// trim from start (in place)
inline void ltrim(std::string &s) {
    s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](unsigned char ch) {
        return !std::isspace(ch);
    }));
}

// trim from end (in place)
inline void rtrim(std::string &s) {
    s.erase(std::find_if(s.rbegin(), s.rend(), [](unsigned char ch) {
        return !std::isspace(ch);
    }).base(), s.end());
}

// trim from both ends (in place)
inline void trim(std::string &s) {
    rtrim(s);
    ltrim(s);
}

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

void handle_argv(
    const std::string &arg,
    std::vector<std::string> &argv,
    std::string &exitCodeFile,
    std::string &pidFile,
    std::string &stdOutFile,
    std::string &stdErrFile
) {
    if (arg.starts_with("ENV\2\2")) {
        std::string envVar = arg.substr(5); // 跳过 "ENV\2\2"
        size_t pos = envVar.find('=');
        if (pos != std::string::npos) {
            std::string key = envVar.substr(0, pos);
            std::string value = envVar.substr(pos + 1);
            setenv(key.c_str(), value.c_str(), 1); // 设置环境变量
        } else {
            std::cerr << "Invalid environment variable format: " << envVar << std::endl;
        }
    }
    else if (arg.starts_with("ARG\2\2")) {
        argv.push_back(arg.substr(5)); // 跳过 "ARG\2\2"
    }
    else if (arg.starts_with("PWD\2\2")) {
        std::string pwd = arg.substr(5); // 跳过 "PWD\2\2"
        if (chdir(pwd.c_str()) != 0) {
            perror("Failed to change directory");
        }
    }
    else if (arg.starts_with("PID\2\2")) {
        pidFile = arg.substr(5); // 跳过 "PID\2\2"
    }
    else if (arg.starts_with("EXT\2\2")) { // EXiT 退出代码文件
        exitCodeFile = arg.substr(5); // 跳过 "EXT\2\2"
    }
    else if (arg.starts_with("CIN\2\2")) {
        // 处理标准输入重定向
        std::string inputFile = arg.substr(5); // 跳过 "CIN\2\2"
        int fd = open(inputFile.c_str(), O_RDONLY);
        if (fd < 0) {
            perror("Failed to open input file");
            return;
        }
        if (dup2(fd, STDIN_FILENO) < 0) {
            perror("Failed to redirect stdin");
            close(fd);
            return;
        }
        close(fd); // 关闭原始文件描述符，因为已经重定向到 stdin
    }
    else if (arg.starts_with("OUT\2\2")) {
        // 处理标准输出重定向
        stdOutFile = arg.substr(5); // 跳过 "COUT\2\2"
        unlink(stdOutFile.c_str());
        int fd = open(stdOutFile.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0666);
        if (fd < 0) {
            perror("Failed to open output file");
            return;
        }
        if (dup2(fd, STDOUT_FILENO) < 0) {
            perror("Failed to redirect stdout");
            close(fd);
            return;
        }
        close(fd); // 关闭原始文件描述符，因为已经重定向到 stdout
    }
    else if (arg.starts_with("ERR\2\2")) {
        // 处理标准错误重定向
        stdErrFile = arg.substr(5); // 跳过 "ERR\2\2"
        unlink(stdErrFile.c_str());
        int fd = open(stdErrFile.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0666);
        if (fd < 0) {
            perror("Failed to open error file");
            return;
        }
        if (dup2(fd, STDERR_FILENO) < 0) {
            perror("Failed to redirect stderr");
            close(fd);
            return;
        }
        close(fd); // 关闭原始文件描述符，因为已经重定向到 stderr
    }
    else {
        std::cerr << "Unknown jobFile arg: " << arg << std::endl;
    }
}

void aria2_run(const char* libDir, int jobFD)
{
    int pid = fork();
    if (pid > 0) {
        // fork 成功
        return;
    }
    else if (pid < 0) {
        // fork 失败
        perror("fail to fork()");
        return;
    }

    ////////////// 子进程 //////////////
    
    // 读取 jobFD 文件全部内容
    std::string jobFileContent;
    char buffer[4096];
    ssize_t bytesRead;
    for (;;) {
        while ((bytesRead = read(jobFD, buffer, sizeof(buffer))) > 0) {
            jobFileContent.append(buffer, bytesRead);
        }

        // 如果最后不是以 "\4\4\4\4" 结尾，就等待1秒后继续读取
        if (jobFileContent.ends_with("\4\4\4\4")) {
            break; // 读取完成
        }
        usleep(100000); // 等待0.1秒后继续读取
    }
    close(jobFD); // 关闭 jobFD 文件描述符
    if (jobFileContent.empty()) {
        perror("jobFile is empty");
        _exit(1);
    }

    // 以 "\3\3\3\3" 分割 jobFileContent
    std::string exitCodeFile;
    std::string pidFile;
    std::string stdOutFile;
    std::string stdErrFile;
    std::vector<std::string> argv;
    size_t pos = 0;
    size_t nextPos;
    // 所有参数都要以 "\3\3\3\3" 结尾，包括最后一个参数
    while ((nextPos = jobFileContent.find("\3\3\3\3", pos)) != std::string::npos) {
        std::string arg = jobFileContent.substr(pos, nextPos - pos);
        pos = nextPos + 4; // 跳过 "\3\3\3\3"
        handle_argv(arg, argv, exitCodeFile, pidFile, stdOutFile, stdErrFile);
    }

    // 把 argv 转换为 char** 数组
    int argc = argv.size();
    const char* argvArray[argc];
    for (int i = 0; i < argc; ++i) {
        argvArray[i] = argv[i].c_str();
    }

    // 设置 openssl legacy.so 加载路径
    setenv("OPENSSL_MODULES", libDir, 1);

    int exitCode = 0;
    int jobPid = fork();
    if (jobPid == 0) {
        // 调用 aria2c 的 main 函数
        _exit(g_aria2c_main(argc, (char**)argvArray));
    }
    else if (jobPid < 0) {
        // fork 失败
        perror("Failed to fork job process");
    }

    {
        // 把进程id写入pidFile
        unlink(pidFile.c_str());
        int fd = open(pidFile.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0666);
        if (fd <= 0) {
            // 打开失败
            perror("Failed to open pidFile");
            return;
        }
        std::string pidStr = std::to_string(jobPid) + '\n';
        if (write(fd, pidStr.c_str(), pidStr.size()) < 0) {
            perror("Failed to write pid to pidFile");
        }
        close(fd); // 关闭文件描述符
    }
    
    if (jobPid > 0) {
        waitpid(jobPid, &exitCode, 0);
    }

    // 把退出代码写到 exitCodeFile 文件
    if (!exitCodeFile.empty()) {
        unlink(exitCodeFile.c_str());
        int fd = open(exitCodeFile.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0666);
        if (fd <= 0) {
            // 打开失败
            perror("Failed to open exitCodeFile");
            _exit(1);
        }
        std::string exitCodeStr = std::to_string(exitCode) + '\n';
        write(fd, exitCodeStr.c_str(), exitCodeStr.size());
        close(fd); // 关闭文件描述符
    }

    // 30秒后清理临时文件
    sleep(30);
    if (!stdOutFile.empty()) {
        unlink(stdOutFile.c_str());
    }
    if (!stdErrFile.empty()) {
        unlink(stdErrFile.c_str());
    }
    if (!pidFile.empty()) {
        unlink(pidFile.c_str());
    }
    if (!exitCodeFile.empty()) {
        unlink(exitCodeFile.c_str());
    }

    _exit(exitCode);
}

void next_job_index(uint64_t &jobIndex, std::string &jobFile, const std::string &jobIndexFile)
{
    ++jobIndex;
    jobFile = "/dev/shm/cn.hu60.aria2.job." + std::to_string(jobIndex);
    
    for (;;) {
        // 写入 job index 文件
        unlink(jobIndexFile.c_str());
        int fd = open(jobIndexFile.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0666);
        if (fd <= 0) {
            // 打开失败
            perror("Failed to open jobIndexFile");
            usleep(100000);
            continue;
        }
        std::string fileContent = jobFile + '\n';
        if (write(fd, fileContent.c_str(), fileContent.size()) < 0) {
            perror("Failed to write jobIndexFile");
            close(fd); // 关闭文件描述符
            usleep(100000);
            continue;
        }
        close(fd); // 关闭文件描述符
        break;
    }
}

void aria2_job_loop(const char* libDir)
{
    std::string jobIndexFile = "/dev/shm/cn.hu60.aria2.job.index";
    std::string jobFile;
    uint64_t jobIndex = time(nullptr);
    next_job_index(jobIndex, jobFile, jobIndexFile);
    for (;;) {
        int jobFD = open(jobFile.c_str(), O_RDONLY);
        if (jobFD <= 0) {
            usleep(100000);
            continue;
        }
        
        next_job_index(jobIndex, jobFile, jobIndexFile);
        
        aria2_run(libDir, jobFD);

        close(jobFD); // 关闭 jobFD 文件描述符
    }
}

void aria2_init(const char* libDir)
{
    int fd;
    
    
    // 先重定向 stderr，以便发生错误时及时输出
    // 打开文件 /dev/shm/cn.hu60.aria2.stderr，把 stderr 重定向到这个文件
    unlink("/dev/shm/cn.hu60.aria2.stderr");
    fd = open("/dev/shm/cn.hu60.aria2.stderr", O_WRONLY | O_CREAT | O_TRUNC, 0666);
    if (fd <= 0) {
        // 打开失败
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
    unlink("/dev/shm/cn.hu60.aria2.stdout");
    fd = open("/dev/shm/cn.hu60.aria2.stdout", O_WRONLY | O_CREAT | O_TRUNC, 0666);
    if (fd <= 0) {
        // 打开失败
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
    

    aria2_load(libDir);
    aria2_job_loop(libDir);

    
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
