/*
 * @File	  : phigros_service.hpp
 * @Coding	  : utf-8
 * @Author    : Bing
 * @Time      : 2023/05/17 21:39
 * @Introduce : Phigros API的接口类
*/

#pragma once

#ifndef PHIGROS_SERVICE_HPP
#define PHIGROS_SERVICE_HPP  
#include "configuration/config.hpp"

class PhigrosService {
public:
	virtual ~PhigrosService() = default;
	virtual Json getAll(const UserData&, std::string_view) = 0;
	virtual Json getBest(const UserData&, std::string_view, const std::string& , unsigned char) = 0;
private:
};


#endif // !PHIGROS_SERVICE_HPP
