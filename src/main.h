#pragma once

#define CPPHTTPLIB_OPENSSL_SUPPORT
#ifndef MAIN_H
#define MAIN_H

// 设置1为开启跨域访问(想要性能问题的话建议关闭,使用反向代理)
#define CORS_OPEN  0  

#include "httplib.h"
#include <iostream>
#include <memory>

#include "spdlog/spdlog.h"
#include "spdlog/sinks/daily_file_sink.h"
#include "fmt/format.h"

namespace std {
    using fmt::format;
    using fmt::format_error;
    using fmt::formatter;
}

#include <common/utils/sql_util.hpp>
#include <common/utils/log_system.hpp>
#include <configuration/config.hpp>

#include "crow/middlewares/cors.h"
#include "crow.h"

/* =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */
#include "service/arcaea_service.hpp"
#include "service/impl/arcaea_service_impl.hpp"
#include "controller/arcaea_controller.hpp"
#include <service/phigros_service.hpp>
#include <service/impl/phigros_service_impl.hpp>
#include <controller/phigros_controller.hpp>
/* =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

// O3优化
#pragma GCC optimize(3)
#pragma G++ optimize(3)

#if OLD 
#define CROW_ENABLE_SSL  
#endif

#define CROW_ENFORCE_WS_SPEC  

//初始化
inline void init(void) {
    Config::initialized();
    LogSystem::initialized();
    SQL_Util::initialized();

    // 初始化phigros
    SQL_Util::PhiDB << "select sid,id,title,song_illustration_path,rating_ez,rating_hd,rating_in,rating_at,rating_lg from phigros;"
        >> [&](std::string sid, int id, std::string title, std::string song_illustration_path,
            float rating_ez, float rating_hd, float rating_in, float rating_at, float rating_lg) {
                DefinedStruct::PhiSongInfo phi;
                if (!sid.empty())
                {
                    phi.sid = sid;
                    phi.id = id;
                    phi.title = title;
                    phi.song_illustration_path = song_illustration_path;
                    phi.rating[0] = rating_ez;
                    phi.rating[1] = rating_hd;
                    phi.rating[2] = rating_in;
                    phi.rating[3] = rating_at;
                    phi.rating[4] = rating_lg;

                    Global::PhigrosSongInfo[sid] = std::move(phi);
                }
    };
}

// 启动项
inline void start(void){
    const std::string
        secret{ Config::Parameter::getSecret()},
        issuer{ Config::Parameter::getIssuer()};
    const ushort port{ Config::Parameter::getPort()};
    const ubyte concurrency{ Config::Parameter::getConcurrency() };

    /* =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

    CrowApp app; 

    // 日志等级
    crow::logger::setLogLevel(crow::LogLevel::Critical);

#if CORS_OPEN
    // 跨域访问
    if (has_cors) {
        auto& cors = app.get_middleware<crow::CORSHandler>();
        cors
            .global()
            .headers("origin, x-requested-with, accept, access-control-allow-origin, authorization, content-type")
            .methods("POST"_method, "GET"_method, "PUT"_method, "DELETE"_method, "PATCH"_method, "OPTIONS"_method)
            //.prefix("/cors")
            .origin("*");
    }
#endif
    
    app.bindaddr("0.0.0.0").port(port);
    
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    // ArcaeaService
    std::unique_ptr<ArcaeaService> arcaea{ new ArcaeaServiceImpl() };
    ArcaeaController arcaea_controller(app, secret, issuer, std::move(arcaea));
    arcaea_controller.controller();
    // PhigrosService
    std::unique_ptr<PhigrosService> phigros{ new PhigrosServiceImpl() };
    PhigrosController phigros_controller(app, std::move(phigros));
    phigros_controller.controller();

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    if (concurrency != 0)
        app.concurrency(concurrency).run();
    else
        app.multithreaded().run();
}

#endif