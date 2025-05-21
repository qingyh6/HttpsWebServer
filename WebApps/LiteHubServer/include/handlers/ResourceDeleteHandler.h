#pragma once
#include "../../../../HttpServer/include/router/RouterHandler.h"
#include "../../../HttpServer/include/utils/MysqlUtil.h"
#include "../LiteHubServer.h"

class ResourceDeleteHandler : public http::router::RouterHandler 
{
public:
    explicit ResourceDeleteHandler(LiteHubServer* server) : server_(server) {}

    void handle(const http::HttpRequest& req, http::HttpResponse* resp) override;

    std::string trim(const std::string& str) ;

private:
    LiteHubServer* server_;
    http::MysqlUtil     mysqlUtil_;
};