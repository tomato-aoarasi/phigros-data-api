//#include "main.h"

//#define DEBUG
#ifdef DEBUG
#include <test/bing/test_project.hpp>

int main(int argc, char* argv[]) {
    std::cout << "\033[44m--* DEBUG START *--\033[0m\n";

    init();
    TestProject::SongAllData();

    std::cout << "\033[44m--*  DEBUG END  *--\033[0m" << std::endl;
    return 0;
}
#endif // DEBUG
#ifndef DEBUG

#include <cpprest/http_client.h>

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

int main(int argc, char* argv[])
{
    // 添加多个后端服务器
    std::vector<web::http::uri> backends = {
        web::http::uri_builder(U("http://150.158.89.12:8299/phi/all"))
            .append_query(U("id"), U("315"))
            .to_uri(),
        /*web::http::uri_builder(U("http://150.158.89.12/api/pgr/findBySong"))
            .append_query(U("id"), U("39"))
            .to_uri()*/
    };

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
    }

    //init();
    //start();
    return 0;
}

#endif // !DEBUG

