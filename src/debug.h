#include <iostream>

template<typename T>
T dbg(const char* file, int line, const char* expr, T value) {
    std::cout << "[" << file << ":" << line << "] " << expr << " = " << value << std::endl;
    return value;
}

// 这个宏会展开成调用上面的函数，并传入文件名、行号和表达式
#define dbg(x) dbg(__FILE__, __LINE__, #x, (x))