#pragma once
#include "../../../../HttpServer/include/router/RouterHandler.h"
#include "../GomokuServer.h"

class VideoMetaHandler : public http::router::RouterHandler 
{
public:
    explicit VideoMetaHandler(GomokuServer* server) : server_(server) {}

    void handle(const http::HttpRequest& req, http::HttpResponse* resp) override;

private:
    std::string base64_encode(const std::string &in);
    GomokuServer* server_;
};