#include "../include/handlers/ResourceDeleteHandler.h"



void ResourceDeleteHandler::handle(const http::HttpRequest &req, http::HttpResponse *resp)
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

        LOG_INFO<<"解析body(json格式)";
        // 解析body(json格式)
        json parsed = json::parse(req.getBody());
        std::string filename = parsed["filename"];

        std::string fullPath = "/root/uploads/" + filename;
        LOG_INFO<<"删除路径："<<fullPath;

        json respjson;
        if (std::remove(fullPath.c_str()) == 0)
        {
            resp->setStatusLine(req.getVersion(), http::HttpResponse::k200Ok, "OK");
            respjson["status"]="success";
        } 
        else
        {   
            resp->setStatusLine(req.getVersion(), http::HttpResponse::k500InternalServerError, "InternalServerError");
            respjson["status"]="error";
            respjson["message"] = "文件删除失败";
        }
        std::string body = respjson.dump(4);
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