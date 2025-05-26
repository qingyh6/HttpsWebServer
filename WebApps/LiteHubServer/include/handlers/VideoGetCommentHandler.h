#pragma once
#include "../../../../HttpServer/include/router/RouterHandler.h"
#include "../../../HttpServer/include/utils/MysqlUtil.h"
#include "../../../HttpServer/include/utils/JsonUtil.h"
#include "../LiteHubServer.h"

class VideoGetCommentHandler : public http::router::RouterHandler 
{
public:
    explicit VideoGetCommentHandler(LiteHubServer* server) : server_(server) {}

    void handle(const http::HttpRequest& req, http::HttpResponse* resp) override;
    json buildCommentTree(int parentId, std::unordered_map<int, json>& commentMap);
private:
    LiteHubServer* server_;
    http::MysqlUtil     mysqlUtil_;
};