/*
 * @File	  : config.hpp
 * @Coding	  : utf-8
 * @Author    : Bing
 * @Time      : 2023/03/05 21:14
 * @Introduce : 配置类(解析yaml)
*/

#pragma once

#ifndef CONFIG_HPP
#define CONFIG_HPP  
#include <fstream>
#include <string>
#include <filesystem>
#include "nlohmann/json.hpp"
#include "yaml-cpp/yaml.h"
#include "fmt/format.h"
#include <limits>


using namespace std::string_literals;
using ubyte = unsigned char;
using ushort = unsigned short;

namespace std {
	using fmt::format;
	using fmt::format_error;
	using fmt::formatter;
}

#define Json nlohmann::json
#define Ubyte ubyte

class Global final {
	friend class Config;
private:
	Global() = delete;
	~Global() = delete;
	Global(const Global&) = delete;
	Global(Global&&) = delete;
	Global& operator=(const Global&) = delete;
	Global& operator=(Global&&) = delete;
public:
	
};

class Config final{
private:
	Config() = delete;
	~Config() = delete;
	Config(const Config&) = delete;
	Config(Config&&) = delete;
	Config& operator=(const Config&) = delete;
	Config& operator=(Config&&) = delete;
public:
	static void initialized(){

	}
};

#endif