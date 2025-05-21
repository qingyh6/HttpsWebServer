#pragma once
#include "../../../../HttpServer/include/router/RouterHandler.h"
#include "../../../HttpServer/include/utils/MysqlUtil.h"
#include "../LiteHubServer.h"
#include <filesystem>
#include <vector>
#include <map>
#include <string>
#include <chrono>
#include <sstream>
#include <iomanip>
#include <nlohmann/json.hpp>

namespace fs = std::filesystem;
using json = nlohmann::json;
class ResourceListHandler : public http::router::RouterHandler 
{
public:
    explicit ResourceListHandler(LiteHubServer* server) : server_(server) {}

    void handle(const http::HttpRequest& req, http::HttpResponse* resp) override;

    std::string formatFileTime(std::filesystem::file_time_type ftime) ;
    json getUploadHistoryJson() ;

private:
    LiteHubServer* server_;
    http::MysqlUtil     mysqlUtil_;
};