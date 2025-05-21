#pragma once
#include "../../../../HttpServer/include/router/RouterHandler.h"
#include "../LiteHubServer.h"

class VideoStreamHandler : public http::router::RouterHandler 
{
public:
    explicit VideoStreamHandler(LiteHubServer* server) : server_(server) {}

    void handle(const http::HttpRequest& req, http::HttpResponse* resp) override;
    std::string url_decode(std::string encoded) ;
private:
    LiteHubServer* server_;
};