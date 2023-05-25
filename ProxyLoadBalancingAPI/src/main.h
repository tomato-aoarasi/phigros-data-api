#pragma once

#ifndef MAIN_H
#define MAIN_H

// 设置1为开启跨域访问(想要性能问题的话建议关闭,使用反向代理)
#include "httplib.h"
#include <iostream>
#include <chrono>
#include <cstdlib>
#include <memory>
#include <array>
#include <vector>
#include <restbed>

#include "spdlog/spdlog.h"
#include "fmt/format.h"

namespace std {
    using fmt::format;
    using fmt::format_error;
    using fmt::formatter;
}
#include <configuration/config.hpp>

// O3优化
#pragma GCC optimize(3)
#pragma G++ optimize(3)

//初始化
inline void init(void) {

}

// 启动项
inline void start(void) {

}

#endif