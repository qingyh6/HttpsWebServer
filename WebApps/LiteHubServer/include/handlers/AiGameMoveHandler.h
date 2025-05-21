#pragma once
#include "../../../../HttpServer/include/router/RouterHandler.h"
#include "../LiteHubServer.h"

class AiGameMoveHandler : public http::router::RouterHandler
{
public:
    explicit AiGameMoveHandler(LiteHubServer* server) : server_(server) {}
    void handle(const http::HttpRequest& req, http::HttpResponse* resp) override;
private:
    LiteHubServer* server_;
};