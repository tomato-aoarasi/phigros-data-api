#pragma once

// 设置1为开启跨域访问(想要性能问题的话建议关闭,使用反向代理)
#include "httplib.h"
#include <iostream>
#include <thread>
#include <chrono> 
#include <cstdlib>
#include <filesystem>
#include <memory>
#include <array>
#include <vector>
#include "crow.h"

#include "spdlog/spdlog.h"
#include "fmt/format.h"

namespace std {
    using fmt::format;
    using fmt::format_error;
    using fmt::formatter;
}
#include <configuration/config.hpp>
#include "common/utils.hpp"
#include "common/self_exception.hpp"
#include "common/http_util.hpp"
#include "common/crow_middleware.hpp"

#include <restbed>
#include <ctime>
#include <cpprest/http_client.h>

#ifndef MAIN_H
#define MAIN_H

// O3优化
#pragma GCC optimize(3)
#pragma G++ optimize(3)

using namespace std::chrono_literals;

//初始化
inline void init(void) {
    self::DB::LocalDB << "DELETE FROM ProcessInfo;";

    //const uint32_t PID = system(command.c_str());   // 运行命令   
    //std::cout << PID << std::endl;
    

    auto port_min { Config::config_yaml["other"]["exec-port-min"].as<uint16_t>() };
    auto port_max { Config::config_yaml["other"]["exec-port-max"].as<uint16_t>() };

    uint16_t unified_threads{ Config::config_yaml["other"]["unified-threads"].as<uint16_t>() };

    for (uint16_t port{ port_min }; port <= port_max; ++port) {
        std::string command{ std::format("{}/CrowAPI --port={} --concurrency={} --sid={} &",std::filesystem::current_path().c_str(),port,unified_threads,Global:: proxy_count) };
        auto status_code { std::system(command.c_str()) };

        if (status_code != 0)
        {
            throw std::runtime_error("Startup failed with error code "s + std::to_string(status_code) + " .");
        }

        std::this_thread::sleep_for(50ms);
        ++Global::proxy_count;
    }

    self::DB::LocalDB << "select sid,pid,port,path from ProcessInfo;"
        >> [&](uint32_t sid, uint32_t pid, uint16_t port, std::string path) {
        ProcessInfo pi;
        pi.p_pid = pid;
        pi.p_path = path;
        pi.p_port = port;
        pi.p_sid = sid;
        
        Global::process_info.emplace_back(pi);
    };
}

std::atomic<int> backend_index(0);

namespace fs = std::filesystem;

// 启动项
inline void start(void) {
    uint16_t port{ Config::config_yaml["other"]["port"].as<uint16_t>() };

    std::string proxy_url{ Config::config_yaml["other"]["proxy-uri"].as<std::string>() };

    auto proxy_count{ Global::proxy_count };
    crow::SimpleApp app;

    // 负载均衡
    CROW_ROUTE(app, "/proxy/<path>").methods(crow::HTTPMethod::Get, crow::HTTPMethod::Post, 
        crow::HTTPMethod::Patch, crow::HTTPMethod::Delete, crow::HTTPMethod::Head)
        ([&](const crow::request req,const std::string& uri) {;
        crow::response resp;
        resp.set_header("Content-Type", "application/json");
        std::vector<web::http::uri> backends;
        try {
            uint8_t retry{ 0 };
            size_t max_retry{ (proxy_count + 1) / 2 };

            // 负载均衡url
            for(const auto& info : Global::process_info){
                std::string s_url{ proxy_url + ":"s + std::to_string(info.p_port) + '/' + uri};

                auto url{ web::http::uri_builder(U(s_url)) };
                for (const auto& key : req.url_params.keys())
                {
                    url.append_query(U(key),U(req.url_params.get(key)));
                }
                backends.emplace_back(url.to_uri());

            }

            std::vector<web::http::client::http_client> clients;
            for (const auto& uri : backends) {
                clients.emplace_back(uri);
            }

            std::string body;
            Json data_body;
            while(1){
                ++retry;
                auto& client = clients.at(Global::cyclic_query_value % clients.size());
                auto& u = backends.at(Global::cyclic_query_value % backends.size());
                ++Global::cyclic_query_value;
                web::http::http_request request;

                // 选择请求方式
                switch (req.method)
                {
                case crow::HTTPMethod::Get:
                    request.set_method(web::http::methods::GET);
                    break;
                case crow::HTTPMethod::Post:
                    request.set_method(web::http::methods::POST);
                    break;
                case crow::HTTPMethod::Patch:
                    request.set_method(web::http::methods::PATCH);
                    break;
                case crow::HTTPMethod::Delete:
                    request.set_method(web::http::methods::DEL);
                    break;
                case crow::HTTPMethod::Head:
                    request.set_method(web::http::methods::HEAD);
                    break;
                default:
                    throw self::HTTPException("", 405);
                    break;
                }
                for (const auto& header : req.headers) {
                    request.headers().add(header.first, header.second);
                }
                // 设置request的body
                request.set_body(U(req.body));

                web::http::http_response response{ client.request(request).get() };
                spdlog::info(std::format("Proxy server: {}", client.base_uri().to_string()));
                if (response.status_code() != web::http::status_codes::OK && retry <= max_retry){
                    spdlog::error(std::format("connect fail, code: {}, retry: {}, err_port: {}",
                        response.status_code(), retry, client.base_uri().port()));

                    if (response.status_code() < 500 && response.status_code() >= 400) {
                        try {
                            concurrency::streams::stringstreambuf buffer;
                            response.body().read_to_end(buffer).get();
                            body = buffer.collection();

                            data_body = Json::parse(body);
                            if (data_body.contains("detail")) {
                                throw self::HTTPException(data_body.at("detail").get<std::string>(), response.status_code());
                            }
                        }
                        catch (const self::HTTPException&) {
                            throw;
                        }
                        catch (const std::exception&){}
                        
                        throw self::HTTPException("", response.status_code());
                    }

                    continue;
                }
                else if (retry > max_retry) {
                    concurrency::streams::stringstreambuf buffer;
                    response.body().read_to_end(buffer).get();
                    body = buffer.collection();

                    data_body = Json::parse(body);

                    if (data_body.contains("detail")) {
                        throw self::HTTPException(data_body.at("detail").get<std::string>(), response.status_code());
                    }
                    throw self::HTTPException("", response.status_code());
                }

                concurrency::streams::stringstreambuf buffer;
                response.body().read_to_end(buffer).get();
                body = buffer.collection();

                data_body = Json::parse(body);
                break;
            }
            resp.code = 200;
            resp.write(data_body.dump(2));
            return resp;
        }
        catch (const self::HTTPException& e) {
            resp.code = e.getCode();
            resp.write(HTTPUtil::StatusCodeHandle::getSimpleJsonResult(e.getCode(), e.getMessage()).dump(2));
        }catch(const std::runtime_error& e){
            resp.code = 500;
            resp.write(HTTPUtil::StatusCodeHandle::getSimpleJsonResult(500, e.what()).dump(2));
        }catch (const std::exception& e) {
            resp.code = 500;
            resp.write(HTTPUtil::StatusCodeHandle::getSimpleJsonResult(500).dump(2));
        }
        return resp;
        });

    CROW_ROUTE(app, "/<path>")([&](const std::string& p) {
        crow::response response;

        std::string path{ Global::resource_path + p };

        if (!std::filesystem::exists(path)) {
            response.set_header("Content-Type", "application/json");
            response.code = 404;
            response.write(HTTPUtil::StatusCodeHandle::getSimpleJsonResult(404).dump(2));
            return response;
        }

        // 获取当前时间
        auto now{ std::chrono::system_clock::now() };

        // 计算七天后的时间
        auto seven_days_later{ now + std::chrono::hours(24 * 7) };

        // 将时间转换为时间戳（秒数）
        auto timestamp{ std::chrono::system_clock::to_time_t(seven_days_later) };

        // 将时间戳转换为 Crow 框架中的 Expires 数值
        std::stringstream ss;
        ss << std::put_time(std::gmtime(&timestamp), "%a, %d %b %Y %H:%M:%S GMT");
        std::string expires{ ss.str() };
        response.set_static_file_info(path);
        response.set_header("Cache-Control", "public");
        response.set_header("Expires", expires);
        return response;
    });

    app.port(port).multithreaded().run_async();
}

#endif