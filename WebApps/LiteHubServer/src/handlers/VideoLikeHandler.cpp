#include "../include/handlers/VideoLikeHandler.h"


void VideoLikeHandler::handle(const http::HttpRequest &req, http::HttpResponse *resp)
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

        LOG_INFO<<"进入点赞";
        // 解析 JSON
        json body = json::parse(req.getBody());
        std::string option = body["option"];
        const std::string videoName = body["videoName"];
        int userId = std::stoi(session->getValue("userId"));  // 确保登录后 userId 存在 session 中

        if (option == "query") {
            // 查询视频点赞总数和当前用户是否点赞
            const std::string statsSql = "SELECT like_count FROM video_stats WHERE video_name = ?";
            auto statsRes = mysqlUtil_.executeQuery(statsSql, videoName);
            int likeCount = statsRes->next() ? statsRes->getInt("like_count") : 0;

            const std::string likedSql = "SELECT 1 FROM video_likes WHERE user_id = ? AND video_name = ?";
            auto likedRes = mysqlUtil_.executeQuery(likedSql, userId, videoName);
            bool liked = likedRes->next();

            json data = {
                {"status", "ok"},
                {"likeCount", likeCount},
                {"liked", liked}
            };
            std::string body = data.dump();
            resp->setStatusLine(req.getVersion(), http::HttpResponse::k200Ok, "OK");
            resp->setContentType("application/json");
            resp->setContentLength(body.size());
            resp->setBody(body);
            return;
        }

        else if (option == "like") {
            // 添加点赞记录，更新点赞数
            const std::string checkSql = "SELECT 1 FROM video_likes WHERE user_id = ? AND video_name = ?";
            auto checkRes = mysqlUtil_.executeQuery(checkSql, userId, videoName);
            if (!checkRes->next()) {
                mysqlUtil_.executeUpdate("INSERT INTO video_likes (user_id, video_name) VALUES (?, ?)", userId, videoName);
                mysqlUtil_.executeUpdate("UPDATE video_stats SET like_count = like_count + 1 WHERE video_name = ?", videoName);
            }

            json success = { {"status", "ok"} };
            std::string body = success.dump();
            resp->setStatusLine(req.getVersion(), http::HttpResponse::k200Ok, "OK");
            resp->setContentType("application/json");
            resp->setContentLength(body.size());
            resp->setBody(body);
            return;
        }

        else if (option == "unlike") {
            // 删除点赞记录，减少点赞数
            mysqlUtil_.executeUpdate("DELETE FROM video_likes WHERE user_id = ? AND video_name = ?", userId, videoName);
            mysqlUtil_.executeUpdate("UPDATE video_stats SET like_count = like_count - 1 WHERE video_name = ? AND like_count > 0", videoName);

            json success = { {"status", "ok"} };
            std::string body = success.dump();
            resp->setStatusLine(req.getVersion(), http::HttpResponse::k200Ok, "OK");
            resp->setContentType("application/json");
            resp->setContentLength(body.size());
            resp->setBody(body);
            return;
        }

        else {
            throw std::invalid_argument("Invalid option");
        }
        
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

