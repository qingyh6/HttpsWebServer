#pragma once
#include "../../../../HttpServer/include/router/RouterHandler.h"
#include "../../../HttpServer/include/utils/FormatUtil.h"
#include "../LiteHubServer.h"

class VideoStreamHandler : public http::router::RouterHandler 
{
public:
    explicit VideoStreamHandler(LiteHubServer* server) : server_(server) {}

    void handle(const http::HttpRequest& req, http::HttpResponse* resp) override;

private:
    LiteHubServer* server_;
    FormatUtil    formatUtil_;
};