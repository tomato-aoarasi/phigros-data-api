/*
 * @File	  : utils.hpp
 * @Coding	  : utf-8
 * @Author    : Bing
 * @Time      : 2023/05/28 14:11
 * @Introduce : 各种工具
*/

#pragma once

#include <sqlite_modern_cpp.h>
#include <vector>
#include <unistd.h>
#include <thread>

#ifndef UTILS_HPP
#define UTILS_HPP

namespace self {
	struct DB {
		inline static sqlite::database LocalDB{ sqlite::database("./localDB.db")};
	};

	struct Tools {
		inline static bool exec_simple(const char* cmd) {
			FILE* pipe{ popen(cmd, "r") };
			if (!pipe) {
				pclose(pipe);
				return false;
			}
			pclose(pipe);
			return true;
		}

		// 运行shell脚本并获取字符串
		inline static const std::string exec(const char* cmd) {
			FILE* pipe = popen(cmd, "r");
			if (!pipe) {
				pclose(pipe);
				return "ERROR";
			}
			char buffer[128];
			std::string result = "";
			while (!feof(pipe)) {
				if (fgets(buffer, 128, pipe) != NULL)
					result += buffer;
			}
			pclose(pipe);
			return result;
		}
	};
}

#endif // !UTILS_HPP

