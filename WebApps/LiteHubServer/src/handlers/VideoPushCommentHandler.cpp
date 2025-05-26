#include "../include/handlers/VideoPushCommentHandler.h"
#include <iomanip>


void VideoPushCommentHandler::handle(const http::HttpRequest &req, http::HttpResponse *resp)
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

        LOG_INFO<<"视频评论";
        
         // 解析body(json格式)
        json body = json::parse(req.getBody());
        const std::string videoName = body["videoName"];
        // int userId = body["user_id"];
        const std::string user_name = session->getValue("username");
        int parentId = body["parent_id"];
        const std::string content = body["content"];
        const std::string now = currentTimeStr();  // 获取当前时间字符串
        LOG_INFO<<videoName;
        // LOG_INFO<<userId;
        LOG_INFO<<parentId;
        LOG_INFO<<content;
        

        const std::string sql = R"(INSERT INTO comments (video_name, user_name, parent_id, content, created_at)
                             VALUES (?, ?, ?, ?, ?))";

        int affected = mysqlUtil_.executeUpdate(sql, videoName, user_name, parentId, content, now);
        if (affected <=0) {
            throw std::runtime_error("数据库写入失败");
        } 

       
        
        resp->setStatusLine(req.getVersion(), http::HttpResponse::k200Ok, "OK");
        resp->setCloseConnection(false);
         resp->setContentType("application/json");
        std::string resp_body=R"({"status": "success"})";
        resp->setContentLength(resp_body.size());
        resp->setBody(resp_body);
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


// 获取当前时间字符串函数
std::string VideoPushCommentHandler::currentTimeStr() {
    auto now = std::chrono::system_clock::now();
    auto now_time_t = std::chrono::system_clock::to_time_t(now);
    std::ostringstream oss;
    oss << std::put_time(std::localtime(&now_time_t), "%Y-%m-%d %H:%M:%S");
    return oss.str();
}