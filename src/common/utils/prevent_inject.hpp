/*
 * @File	  : prevent_inject.hpp
 * @Coding	  : utf-8
 * @Author    : Bing
 * @Time      : 2023/03/07 22:42
 * @Introduce : ���SQLע��
*/

#pragma once

#ifndef PREVENT_INJECT_HPP
#define PREVENT_INJECT_HPP  
#include <string_view>

namespace self {
    // ���SQL
    bool CheckSQL(std::string_view sql)
    {
        std::string key[9] = { "%","/","union","|","&","^" ,"#","/*","*/" };
        for (int i{ 0 }; i < 9; i++)
        {
            if (sql.find(key[i]) != std::string::npos)
            {
                return false;
            }
        }
        return true;
    }

    // ������
    bool CheckParameter(std::string_view Parameter)
    {
        std::string key[14] = { "and","*","="," ","%0a","%","/","union","|","&","^" ,"#","/*","*/" };
        for (int i{ 0 }; i < 14; i++)
        {
            if (Parameter.find(key[i]) != std::string::npos)
            {
                return false;
            }
        }
        return true;
    }
}

#endif
