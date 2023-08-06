/*
 * @File	  : phigros_service.hpp
 * @Coding	  : utf-8
 * @Author    : Bing
 * @Time      : 2023/05/17 21:39
 * @Introduce : Phigros API的接口类
*/

#pragma once

#include "configuration/config.hpp"

#ifndef PHIGROS_SERVICE_HPP
#define PHIGROS_SERVICE_HPP  
class PhigrosService {
public:
	virtual ~PhigrosService() = default;
	virtual Json getAll(const UserData&, std::string_view) = 0;
	virtual Json getBest(const UserData&, std::string_view, const std::string& , unsigned char) = 0;
	virtual Json getRecords(const UserData&, std::string_view) = 0;

	virtual Json getAlias(const UserData&, int32_t) = 0;
	virtual Json documentSongidByAlias(const UserData&, std::string) = 0;

	virtual Json matchAlias(const defined::PhiMatchAlias&) = 0;

	virtual Json getSongInfo(const UserData&, const defined::PhiInfoParamStruct&) = 0;
	
	virtual std::string addAlias(const UserData&, const defined::PhiAliasAddParam&) = 0;
	virtual std::string delAlias(const UserData&, const defined::PhiAliasAddParam&) = 0;

	virtual std::string asyncMatch(void) = 0;
private:
};


#endif // !PHIGROS_SERVICE_HPP
