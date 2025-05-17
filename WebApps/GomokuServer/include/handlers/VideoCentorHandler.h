#pragma once
#include "../../../../HttpServer/include/router/RouterHandler.h"
#include "../GomokuServer.h"

class VideoCentorHandler : public http::router::RouterHandler 
{
public:
    explicit VideoCentorHandler(GomokuServer* server) : server_(server) {}

    void handle(const http::HttpRequest& req, http::HttpResponse* resp) override;

private:
    GomokuServer* server_;
};