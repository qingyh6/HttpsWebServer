#pragma once
#include "../../../../HttpServer/include/router/RouterHandler.h"
#include "../../../HttpServer/include/utils/MysqlUtil.h"
#include "../LiteHubServer.h"

class VideoPushCommentHandler : public http::router::RouterHandler 
{
public:
    explicit VideoPushCommentHandler(LiteHubServer* server) : server_(server) {}

    void handle(const http::HttpRequest& req, http::HttpResponse* resp) override;

    std::string currentTimeStr();
private:
    LiteHubServer* server_;
    http::MysqlUtil     mysqlUtil_;
};