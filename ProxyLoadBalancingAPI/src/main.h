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

void handle_request(web::http::http_request request, const std::vector<web::uri>& backends) {
    // 轮询选择一个后端服务器
    int index = backend_index++ % backends.size();

    // 创建一个HTTP客户端
    web::http::client::http_client client(backends[index]);

    // 转发请求
    client.request(request.method(), request.relative_uri().to_string())
        .then([=](web::http::http_response response) {
        // 将后端服务器的响应转发回客户端
            request.reply(response.status_code(), response.extract_string().get());
            });
}

// 启动项
inline void start(void) {
    uint16_t port{ Config::config_yaml["other"]["port"].as<uint16_t>() };
    

    std::vector<web::http::uri> backends = {
        web::http::uri_builder(U("http://150.158.89.12:8299/phi/all"))
            .append_query(U("id"), U("315"))
            .to_uri(),
        web::http::uri_builder(U("http://150.158.89.12/api/pgr/findBySong"))
            .append_query(U("id"), U("39"))
            .to_uri()
    };
    /*
    // 为每个后端服务器创建http_client
    std::vector<web::http::client::http_client> clients;
    for (const auto& uri : backends) {
        clients.emplace_back(uri);
    }

    // 循环以轮询方式向每个后端服务器发送请求
    for (int i = 0; i < 10; ++i) {
        auto& client = clients[i % clients.size()];
        web::http::http_request request(web::http::methods::GET);
        request.headers().add("Authorization", "Bearer GPwXIOrICHgSWB6rUXhSLmWQ");
        request.headers().add("SessionToken", "yc443mp6cea7xozb3e0kxvvid");
        auto response = client.request(request).get();
        concurrency::streams::stringstreambuf buffer;
        response.body().read_to_end(buffer).get();
        std::string body = buffer.collection();
        std::cout << body << std::endl;
        buffer.close();
    }*/


    auto proxy_count{ Global::proxy_count };
    crow::SimpleApp app;

    CROW_ROUTE(app, "/phi/best")([&](const crow::request& req) {
        crow::response resp;
        resp.set_header("Content-Type", "application/json");
        std::vector<web::http::uri> backends;
        try {
            uint8_t retry{ 0 };
            size_t max_retry{ (proxy_count + 1) / 2 };

            //for (size_t n{ 0 }; n < proxy_count;++n) {

            for(const auto& info : Global::process_info){
                std::string s_url{ "http://127.0.0.1:"s + std::to_string(info.p_port) + "/phi/all"s};
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
                auto& client = clients[Global::cyclic_query_value % clients.size()];
                ++Global::cyclic_query_value;
                web::http::http_request request(web::http::methods::GET);

                for (const auto& header : req.headers) {
                    request.headers().add(header.first, header.second);
                }

                web::http::http_response response{ client.request(request).get() };

                if (response.status_code() != web::http::status_codes::OK && retry <= max_retry){
                    spdlog::warn(std::format("connect fail, code: {}, retry: {}, err_port: {}",
                        response.status_code(), retry, client.base_uri().port()));
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
            resp.code = 500;
            resp.write(HTTPUtil::StatusCodeHandle::getSimpleJsonResult(e.getCode(), e.getMessage()).dump(2));
        }catch(const std::runtime_error& e){
            resp.code = 500;
            resp.write(HTTPUtil::StatusCodeHandle::getSimpleJsonResult(500, e.what()).dump(2));
        }catch (const std::exception& e) {
            resp.code = 500;
            resp.write(HTTPUtil::StatusCodeHandle::getSimpleJsonResult(500).dump(2));
        }

        --Global::cyclic_query_value;
        return resp;
        });

    app.port(port).run();
}

#endif