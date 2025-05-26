#pragma once
#include "../../../../HttpServer/include/router/RouterHandler.h"
#include "../../../HttpServer/include/utils/MysqlUtil.h"
#include "../../../HttpServer/include/utils/FormatUtil.h"
#include "../LiteHubServer.h"

class ResourceDeleteHandler : public http::router::RouterHandler 
{
public:
    explicit ResourceDeleteHandler(LiteHubServer* server) : server_(server) {}

    void handle(const http::HttpRequest& req, http::HttpResponse* resp) override;
    
private:
    LiteHubServer* server_;
    http::MysqlUtil     mysqlUtil_;
    FormatUtil    formatUtil_;
};