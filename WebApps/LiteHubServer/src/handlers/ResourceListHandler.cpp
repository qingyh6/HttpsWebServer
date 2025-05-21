#include "../include/handlers/ResourceListHandler.h"



void ResourceListHandler::handle(const http::HttpRequest &req, http::HttpResponse *resp)
{
    // JSON 解析使用 try catch 捕获异常
    try
    {
        // 检查用户是否已登录
        auto session = server_->getSessionManager()->getSession(req, resp);
        LOG_INFO << "session->getValue(\"isLoggedIn\") = " << session->getValue("isLoggedIn");
        if (session->getValue("isLoggedIn") != "true")
        {
            // 用户未登录，返回未授权错误
            json errorResp;
            errorResp["status"] = "error";
            errorResp["message"] = "Unauthorized";
            std::string errorBody = errorResp.dump(4);

            server_->packageResp(req.getVersion(), http::HttpResponse::k401Unauthorized,
                                "Unauthorized", true, "application/json", errorBody.size(),
                                 errorBody, resp);
            return;
        }

        json historyJson = getUploadHistoryJson();
        std::string body = historyJson.dump(4);
        

        resp->setStatusLine(req.getVersion(), http::HttpResponse::k200Ok, "OK");
        resp->setCloseConnection(false);
        resp->setContentType("application/json");
        resp->setContentLength(body.size());
        resp->setBody(body);
    }
    catch (const std::exception &e)
    {
        // 捕获异常，返回错误信息
        json failureResp;
        failureResp["status"] = "error";
        failureResp["message"] = e.what();
        std::string failureBody = failureResp.dump(4);
        resp->setStatusLine(req.getVersion(), http::HttpResponse::k400BadRequest, "Bad Request");
        resp->setCloseConnection(true);
        resp->setContentType("application/json");
        resp->setContentLength(failureBody.size());
        resp->setBody(failureBody);
    }
}


json ResourceListHandler::getUploadHistoryJson() {
    json result;
    result["status"] = "success";
    result["files"] = json::array();

    try {
        // 按 uploadtime 降序获取所有文件记录
        std::string sql = "SELECT filename, username, uploadtime, duration FROM fileinfo ORDER BY uploadtime DESC";
        auto rows = mysqlUtil_.executeQueryAsVector(sql);

        for (const auto& row : rows) {
            json fileJson;
            fileJson["filename"] = row.at("filename");
            fileJson["username"] = row.at("username");
            fileJson["uploadTime"] = row.at("uploadtime");
            LOG_INFO<<"filename:    "<<row.at("filename")<<"username    :"<< row.at("username")<<"uploadTime:    "<<row.at("uploadtime");

            result["files"].push_back(fileJson);
        }

    } catch (const std::exception& e) {
        result["status"] = "error";
        result["message"] = e.what();
    }

    return result;

    // for (const auto& entry : fs::recursive_directory_iterator(rootDir)) {
    //     if (fs::is_regular_file(entry)) {
    //         std::string filename = entry.path().filename().string();
    //         std::string uploadTime = formatFileTime(fs::last_write_time(entry));
            
    //         result["files"].push_back({
    //             {"filename", filename},
    //             {"uploadTime", uploadTime},
    //             {"relativePath", entry.path().string()}  // 如果需要下载时路径,绝对路径


        
    //         });
    //     }
    // }

    // // 可选：按时间降序排序
    // std::sort(result["files"].begin(), result["files"].end(),
    //     [](const json& a, const json& b) {
    //         return a["uploadTime"] > b["uploadTime"];
    //     });

    // return result;
}