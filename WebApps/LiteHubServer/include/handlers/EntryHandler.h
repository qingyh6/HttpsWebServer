#pragma once
#include "../../../../HttpServer/include/router/RouterHandler.h"
#include "../LiteHubServer.h"

class EntryHandler : public http::router::RouterHandler 
{
public:
    explicit EntryHandler(LiteHubServer* server) : server_(server) {}

    void handle(const http::HttpRequest& req, http::HttpResponse* resp) override;

private:
    LiteHubServer* server_;
};