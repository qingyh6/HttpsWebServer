#pragma once
#include "../../../../HttpServer/include/router/RouterHandler.h"
#include "../../../HttpServer/include/utils/MysqlUtil.h"
#include "../LiteHubServer.h"
#include "../../../HttpServer/include/utils/JsonUtil.h"


class LoginHandler : public http::router::RouterHandler 
{
public:
    explicit LoginHandler(LiteHubServer* server) : server_(server) {}
    
    void handle(const http::HttpRequest& req, http::HttpResponse* resp) override;

private:
    int queryUserId(const std::string& username, const std::string& password);

private:
    LiteHubServer*       server_;
    http::MysqlUtil     mysqlUtil_;
};