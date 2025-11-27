#pragma once
#include <time.h>
#include <string>

namespace Log{
    static const char* time_fmt = "%Y-%m-%d %H:%M:%S";
    static std::string getCurTime()
    {
        time_t t = time(nullptr);
        char time_str[50];
        strftime(time_str, sizeof(time_str), time_fmt, localtime(&t));
        return std::string(time_str);
    }
}

//  __FILE__ 获取源文件的相对路径和名字
//  __LINE__ 获取该行代码在文件中的行号
//  __func__ 或 __FUNCTION__ 获取函数名
#define LOGI(format, ...) fprintf(stdout, "[INFO]%s [%s:%d] " format "\n",  Log::getCurTime().c_str(), __FILE__, __LINE__, ##__VA_ARGS__)
#define LOGE(format, ...) fprintf(stdout, "[ERROR]%s [%s:%d] " format "\n", Log::getCurTime().c_str(), __FILE__, __LINE__, ##__VA_ARGS__)
// q: fprintf写入日志文件应该如何做？a: 可以将stdout替换为文件指针FILE*，并在程序开始时打开日志文件，结束时关闭日志文件。
