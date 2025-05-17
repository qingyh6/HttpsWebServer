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

        std::string uploadDir = "/root/uploads";  // 资源上传目录
        json historyJson = getUploadHistoryJson(uploadDir);
        std::string body = historyJson.dump(4);
        LOG_INFO<<"Files List"<<body;

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

std::string ResourceListHandler::formatFileTime(std::filesystem::file_time_type ftime) {
    auto sctp = std::chrono::time_point_cast<std::chrono::system_clock::duration>(
        ftime - fs::file_time_type::clock::now() + std::chrono::system_clock::now()
    );//将文件时间转换成系统时间
    std::time_t cftime = std::chrono::system_clock::to_time_t(sctp); //将系统时钟时间点转换为C标准库的time_t类型
    std::stringstream ss;
    //将time_t转换为本地时间的tm结构,并按照指定格式（"%Y-%m-%d %H:%M"格式化时间
    ss << std::put_time(std::localtime(&cftime), "%Y-%m-%d %H:%M");
    return ss.str();
}

json ResourceListHandler::getUploadHistoryJson(const std::string& rootDir) {
    json result;
    result["status"] = "success";
    result["files"] = json::array();

    for (const auto& entry : fs::recursive_directory_iterator(rootDir)) {
        if (fs::is_regular_file(entry)) {
            std::string filename = entry.path().filename().string();
            std::string uploadTime = formatFileTime(fs::last_write_time(entry));

            result["files"].push_back({
                {"filename", filename},
                {"uploadTime", uploadTime},
                {"relativePath", entry.path().string()}  // 如果需要下载时路径
            });
        }
    }

    // 可选：按时间降序排序
    std::sort(result["files"].begin(), result["files"].end(),
        [](const json& a, const json& b) {
            return a["uploadTime"] > b["uploadTime"];
        });

    return result;
}