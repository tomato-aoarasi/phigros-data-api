/*
 * @File	  : phigros_controller.hpp
 * @Coding	  : utf-8
 * @Author    : Bing
 * @Time      : 2023/05/17 21:35
 * @Introduce : Phigros的controller
*/

#ifndef PHIGROS_CONTROLLER_HPP
#define PHIGROS_CONTROLLER_HPP  
#include <configuration/config.hpp>
#include <service/phigros_service.hpp>

//TODO 待补全
class PhigrosController {
public:
	explicit PhigrosController(CrowApp& app, std::unique_ptr<PhigrosService> phigros) :
		m_app{ app }, m_phigros{ std::move(phigros)} {};
	virtual ~PhigrosController() = default;

	const inline void controller(void) {
        CROW_ROUTE(m_app, "/phi/best")
            .methods("GET"_method)([](const crow::request& req) {

            Json data;
            data["message"] = "Hello World!";

            // 设置响应头为 application/json
            crow::response resp;
            resp.set_header("Content-Type", "application/json");

            // 将 JSON 数据作为响应体返回
            resp.write(data.dump(2));


            return resp;
                });
	};
private:
    PhigrosController() = delete;
    CrowApp& m_app;
    std::unique_ptr<PhigrosService> m_phigros;
};


#endif // !PHIGROS_CONTROLLER_HPP
