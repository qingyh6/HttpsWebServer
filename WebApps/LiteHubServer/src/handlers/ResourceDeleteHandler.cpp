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


        std::string currentUser=formatUtil_.trim(session->getValue("username"));

         // 解析body(json格式)
        json parsed = json::parse(req.getBody());
        const std::string filename = parsed["filename"];

        // // 查询数据库中该文件的上传者
        std::string sql = "SELECT username FROM fileinfo WHERE filename = ?";
        sql::ResultSet *rs=mysqlUtil_.executeQuery(sql, filename);

        if (!rs->next()) {
            throw std::runtime_error("文件未找到");
        }

        std::string owner = formatUtil_.trim(rs->getString("username"));

        LOG_INFO << "owner: [" << owner << "] currentUser: [" << currentUser << "]";
        // 判断是否是当前用户
        if (owner != currentUser &&currentUser!="root") {
            throw std::runtime_error("无权限删除他人文件");
        }

        LOG_INFO<<"解析body(json格式)";
       

        std::string fullPath;
        bool isVideo=false;
         if (filename.find(".avi")!= std::string::npos ||filename.find(".mp4")!= std::string::npos||filename.find(".mkv")!= std::string::npos)
         {
            fullPath= "/root/uploads/videos/" + filename;
            isVideo=true;
         }
         else
         {
            fullPath= "/root/uploads/" + filename;
         }

         
        LOG_INFO<<"删除路径："<<fullPath;

        json respjson;
        if (std::remove(fullPath.c_str()) == 0)
        {   
            std::string sql = "DELETE FROM fileinfo WHERE filename = ?";
            mysqlUtil_.executeUpdate(sql,filename);
            LOG_INFO<<"执行数据库删除";

            // ✅ 删除缩略图文件
            if (isVideo) {
                std::string filenameNoExt = filename.substr(0, filename.find_last_of('.'));
                std::string thumbPath = "/root/uploads/videos/thumbnails/" + filenameNoExt + ".jpg";
                std::remove(thumbPath.c_str());

                // const std::string video_name=filename;
                sql = "DELETE FROM video_stats WHERE video_name = ?";
                mysqlUtil_.executeUpdate(sql,filename);

                sql = "DELETE FROM comments WHERE video_name = ?";
                mysqlUtil_.executeUpdate(sql,filename);

                sql = "DELETE FROM video_likes WHERE video_name = ?";
                mysqlUtil_.executeUpdate(sql,filename);
            }
            

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
        LOG_ERROR<<e.what();
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


