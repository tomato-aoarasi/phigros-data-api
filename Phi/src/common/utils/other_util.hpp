/*
 * @File	  : other_util.hpp
 * @Coding	  : utf-8
 * @Author    : Bing
 * @Time      : 2023/04/22 15:33
 * @Introduce : 其他工具
*/

#pragma once

#include <string>
#include <string_view>
#include <vector>
#include <chrono>
#include <future>
#include <cryptopp/aes.h>
#include <cryptopp/modes.h>
#include <cryptopp/filters.h>
#include "configuration/config.hpp"
#include "crow.h"
#include "httplib.h"

#ifndef OTHER_UTIL
#define OTHER_UTIL  

using namespace std::chrono_literals;
using uchar = unsigned char;

class OtherUtil final {
public:
    /// <summary>
    /// 保留浮点数后N位
    /// </summary>
    /// <param name="f">数字</param>
    /// <param name="n">保留多少位</param>
    /// <returns></returns>
    inline static std::string retainDecimalPlaces(double f, int n = 2) {
        std::stringstream ss;
        ss << std::fixed << std::setprecision(n) << f;
        return ss.str();
    };

    /// <summary>
    /// 判断vector<string>是否存在特定值
    /// </summary>
    /// <param name="keys">crow param的列表</param>
    /// <param name="val">是否含有特定字符串</param>
    /// <returns>true为存在,false为不存在</returns>
    inline static bool hasParam(const std::vector<std::string>& keys, std::string_view val) {
        return std::find(keys.cbegin(), keys.cend(), val) != keys.cend();
    }

    /// <summary>
    /// 效验request参数
    /// </summary>
    /// <param name="req">req丢进去就完事了</param>
    /// <param name="val">是否存在的字符串</param>
    /// <returns>true返回长度不为0的内容</returns>
    inline static bool verifyParam(const crow::request& req, std::string_view val) {
        bool has_value{ hasParam(req.url_params.keys(), val) };
        if (has_value)
        {
            return !std::string(req.url_params.get(val.data())).empty();
        }
        else return false;
    }

    /// <summary>
    /// 异步获取API(超时)
    /// </summary>
    /// <param name="data">传入的JSON</param>
    /// <param name="timeout">超时时间</param>
    /// <param name="domain">域名</param>
    /// <param name="uri">uri</param>
    inline static void asyncGetAPI(Json& data ,const std::chrono::milliseconds& timeout,const std::string& domain, const std::string& uri) {
        std::future<Json> future{ std::async(std::launch::async,[&]()->Json {

            httplib::Client client(domain);
            httplib::Result res = client.Get(uri);

            // 到时候加一个超时
            if (res && res->status == 200) return json::parse(res->body);
            throw std::runtime_error("Failed to get API data");
        }) };

        std::future_status status{ future.wait_for(timeout) };

        if (status == std::future_status::timeout)throw self::TimeoutException();
        data = future.get();
    }

    /// <summary>
    /// 对数字进行补位
    /// </summary>
    /// <param name="num"></param>
    /// <param name="symbol"></param>
    /// <returns></returns>
    inline static std::string digitSupplementHandle(auto num, char&& symbol ='0', int length = 7) {
        std::stringstream ss;
        ss << std::setw(length) << std::setfill(symbol) << num;
        return ss.str();
    };

    // AES-CBC解码
    inline static std::vector<unsigned char> decrypt_AES_CBC(const std::vector<unsigned char>& data,
        const std::vector<unsigned char>& key,
        const std::vector<unsigned char>& iv)
    {
        std::vector<unsigned char> decrypted_data;

        CryptoPP::AES::Decryption aesDecryption(&key[0], key.size());
        CryptoPP::CBC_Mode_ExternalCipher::Decryption cbcDecryption(aesDecryption, &iv[0]);

        CryptoPP::StreamTransformationFilter stfDecryptor(cbcDecryption, new CryptoPP::VectorSink(decrypted_data));

        stfDecryptor.Put(&data[0], data.size());
        stfDecryptor.MessageEnd();

        return decrypted_data;
    }

    // base64解码的工具
    inline static std::vector<uchar> base64Decode(const std::string& base64Str) {
        static const std::string base64Chars =
            "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
            "abcdefghijklmnopqrstuvwxyz"
            "0123456789+/";

        std::vector<uchar> data;
        size_t i = 0;
        uint32_t n = 0;
        int padding = 0;

        while (i < base64Str.length()) {
            char c = base64Str[i++];
            if (c == '=') {
                padding++;
                continue;
            }
            size_t index = base64Chars.find(c);
            if (index == std::string::npos) {
                continue;
            }
            n = (n << 6) | index;
            if (i % 4 == 0) {
                data.push_back((n >> 16) & 0xFF);
                data.push_back((n >> 8) & 0xFF);
                data.push_back(n & 0xFF);
                n = 0;
            }
        }
        if (padding > 0) {
            n <<= padding * 6;
            data.push_back((n >> 16) & 0xFF);
            if (padding == 1) {
                data.push_back((n >> 8) & 0xFF);
            }
        }
        return data;
    }

    /// <summary>
    /// 小端大端转换
    /// </summary>
    /// <typeparam name="T">数据类型</typeparam>
    /// <param name="value">参数</param>
    /// <returns>返回大端的数据</returns>
    template <typename T>
    inline static T littleBigEndianConversion(T value) {
        int8_t* bytes{ reinterpret_cast<int8_t*>(&value)};

        size_t length{ sizeof(T) - 1 };
        for (size_t i{ 0 }; i <= length / 2; i++)
        {
            std::swap(bytes[i], bytes[length - i]);
        }

        return value;
    }
private:
};

#endif // OTHER_UTIL