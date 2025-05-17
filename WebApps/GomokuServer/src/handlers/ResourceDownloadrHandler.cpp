#include "../include/handlers/ResourceDownloadHandler.h"
#include "../../../HttpServer/include/utils/FileUtil.h"
#include <fstream>
#include <iostream>


void ResourceDownloadHandler::handle(const http::HttpRequest &req, http::HttpResponse *resp)
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
        std::string filePath = parsed["resourceId"];
        LOG_INFO<<"DOWNLOAD PAGE"<<filePath;

        
        FileUtil file(filePath);
        if (!file.isValid())
        {
            LOG_WARN << filePath << "not exist.";
            resp->setStatusLine(req.getVersion(), http::HttpResponse::k404NotFound, "Not Found");
            resp->setContentType("text/plain");
            std::string resp_info="File not found";
            resp->setContentLength(resp_info.size());
            resp->setBody(resp_info);
        }

        resp->setStatusLine(req.getVersion(), http::HttpResponse::k200Ok, "OK");
        resp->setCloseConnection(false);
        resp->setContentType("application/octet-stream");

        std::string filename = std::filesystem::path(filePath).filename().string();
        LOG_INFO<<"filename:"<<filename;
        resp->addHeader("Content-Disposition", "attachment; filename=\"" + filename + "\"");
        resp->setContentLength(file.size());
        resp->setisFileResponse(filePath);
       
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