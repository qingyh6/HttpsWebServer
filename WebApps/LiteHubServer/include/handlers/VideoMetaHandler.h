#pragma once
#include "../../../../HttpServer/include/router/RouterHandler.h"
#include "../../../HttpServer/include/utils/MysqlUtil.h"
#include "../LiteHubServer.h"

class VideoMetaHandler : public http::router::RouterHandler 
{
public:
    explicit VideoMetaHandler(LiteHubServer* server) : server_(server) {}

    void handle(const http::HttpRequest& req, http::HttpResponse* resp) override;

private:
    std::string base64_encode(const std::string &in);
    LiteHubServer* server_;
    http::MysqlUtil     mysqlUtil_;
};