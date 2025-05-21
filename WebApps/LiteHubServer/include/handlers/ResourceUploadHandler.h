#pragma once
#include "../../../../HttpServer/include/router/RouterHandler.h"
#include "../../../HttpServer/include/utils/MysqlUtil.h"
#include "../LiteHubServer.h"

class ResourceUploadHandler : public http::router::RouterHandler 
{
public:
    explicit ResourceUploadHandler(LiteHubServer* server) : server_(server) {}

    void handle(const http::HttpRequest& req, http::HttpResponse* resp) override;
    bool generateMetaVideo(const std::string &filename,float &duration,bool &isvideo);
    bool insertUploadRecord(const std::string& filename,
                        const std::string& username,
                        const std::string& uploadtime,
                        double duration,
                        bool   isvideo);
private:
    LiteHubServer* server_;
    http::MysqlUtil     mysqlUtil_;
};