# 鸿蒙 PC /dev/shm 共享内存文件系统的特点

以下是 @SwimmingTiger 观察到的特点，可能不准确，欢迎补充：

1. /dev/shm 似乎是鸿蒙 PC 内唯一一个可以跨应用互相访问且不需要特殊权限的文件夹。

2. 这是个 tmpfs 文件系统，所有文件都位于内存，所以不用担心 I/O 性能。

3. 由 mount 命令输出可知，/dev/shm 中所有文件的总大小不能超过 1GB。

4. 文件夹不支持列目录操作，`ls /dev/shm`会得到`Permission denied`，所以只能打开预先知道名称的文件。

5. 文件的默认权限是`-rw-rw-r--`（0664），只有打开文件的app可以修改或删除文件，其他app只有只读权限。

   备注：和安卓一样，不同鸿蒙app的运行uid不同，所以一个app创建的文件无法被另一个以不同uid运行的app删除。

6. 一个鸿蒙用户无法打开另一个鸿蒙用户在此处创建的文件。

   用户A写入`/dev/shm/test`后注销，再由用户B登录后尝试打开，会得到`Permission denied`。

   切换用户时文件没有被清理掉，如果被清理掉，得到的应该是`No such file or directory`。

7. 文件似乎不支持以追加（`O_APPEND`）方式打开，所以要用 shell 写入多行内容得用这种语法：

    ```
    {
        echo 123
        echo 456
        date
    } > /dev/shm/test
    ```

    不能用这种语法：

    ```
    echo 123 > /dev/shm/test
    echo 456 >> /dev/shm/test
    date >> /dev/shm/test
    ```

    后者在用 >> 追加内容时会得到 `Permission denied`。

8. 虽然不支持`O_APPEND`，但`O_TRUNC`是支持的，所以重写文件时不需要先删除文件。

   但是因为只有创建文件的应用有权限删除文件，所以在文件不用后创建者应该尽快将其删除。
