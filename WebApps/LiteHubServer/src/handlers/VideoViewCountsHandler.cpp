#include "../include/handlers/VideoViewCountsHandler.h"


void VideoViewCountsHandler::handle(const http::HttpRequest &req, http::HttpResponse *resp)
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

        LOG_INFO<<"播放量更新";
        json body = json::parse(req.getBody());
        const std::string videoName = body["videoName"];

        // 更新播放量
         LOG_INFO<<"更新播放量";
        const std::string updateSql = "UPDATE video_stats SET view_count = view_count + 1 WHERE video_name = ?";
        mysqlUtil_.executeUpdate(updateSql, videoName);

        LOG_INFO<<"查询更新后的播放量";
       // 查询更新后的播放量
        const std::string querySql = "SELECT view_count FROM video_stats WHERE video_name = ?";
        sql::ResultSet* result = mysqlUtil_.executeQuery(querySql, videoName);

        LOG_INFO<<"查询更新后的播放量下一步";
        int viewCount = 0;
        if (result->next())
        {
            viewCount=result->getInt("view_count");
        }
        delete result;
        // 构建返回体
        json respJson = {
            {"status", "ok"},
            {"view_count", viewCount}
        };
        std::string respBody = respJson.dump(4);  
        resp->setStatusLine(req.getVersion(), http::HttpResponse::k200Ok, "OK");
        resp->setCloseConnection(false);
        resp->setContentType("application/json");
        resp->setContentLength(respBody.size());
        resp->setBody(respBody);
        
    }

    catch (const std::exception &e)
    {
        // 捕获异常，返回错误信息
        LOG_ERROR<< e.what();
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

