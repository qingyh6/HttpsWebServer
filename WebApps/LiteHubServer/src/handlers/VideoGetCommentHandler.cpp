#include "../include/handlers/VideoGetCommentHandler.h"


void VideoGetCommentHandler::handle(const http::HttpRequest &req, http::HttpResponse *resp)
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

        // 解析body(json格式)
        
        std::string path=req.path();
        int pos=path.find_last_of('/');
        const std::string videoName=path.substr(pos+1,path.size()-pos-1);
        LOG_INFO<<"加载评论"<<videoName;

        const std::string sql = "SELECT id, user_name, parent_id, content, created_at FROM comments WHERE video_name = ? ORDER BY created_at DESC";
        sql::ResultSet* res = mysqlUtil_.executeQuery(sql, videoName);
       
        // 先将所有评论放入 map，按 id 索引，这里是无序map
        std::unordered_map<int, json> commentMap;
        // json rootComments=json::array();

        while (res->next()) {
            
            int id = res->getInt("id");

            json comment;
            comment["id"] = id;
            comment["user_name"] = res->getString("user_name");
            comment["parent_id"] = res->getInt("parent_id");
            comment["content"] = res->getString("content");
            comment["created_at"] = res->getString("created_at");
            comment["replies"] = json::array();

            commentMap[id] = comment;
        }
        delete res;
        LOG_INFO<<"构建评论树结构";

        // 构建 commentMap 和填充数据同原来
        json rootComments = buildCommentTree(0, commentMap); // 从 parent_id = 0 开始递归构建

      
        // // 构建评论树（填充 replies）
        // for (auto& [id, comment] : commentMap) {
        //     int pid = comment["parent_id"];
        //     if (pid != 0 && commentMap.count(pid)) {
        //         commentMap[pid]["replies"].push_back(comment);
        //     }
        // }

        // // 再次提取顶层评论
        // for (const auto& [id, comment] : commentMap) {
        //     if (comment["parent_id"] == 0) {
        //         rootComments.push_back(comment);
        //     }
        // }
       
        std::string jsonStr = rootComments.dump();
        resp->setStatusLine(req.getVersion(), http::HttpResponse::k200Ok, "OK");
        resp->setCloseConnection(false);
         resp->setContentType("application/json");
        resp->setContentLength(jsonStr.size());
        resp->setBody(jsonStr);
        
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


json VideoGetCommentHandler::buildCommentTree(int parentId, std::unordered_map<int, json>& commentMap) {
    json children = json::array();
    for (auto& [id, comment] : commentMap) {
        if (comment["parent_id"] == parentId) {
            comment["replies"] = buildCommentTree(id, commentMap);  // 递归查找子评论
            children.push_back(comment);
        }
    }
    return children;
}
