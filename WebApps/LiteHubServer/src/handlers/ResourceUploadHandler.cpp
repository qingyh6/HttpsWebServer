#include "../include/handlers/ResourceUploadHandler.h"
#include <fstream>
#include <iostream>


void ResourceUploadHandler::handle(const http::HttpRequest &req, http::HttpResponse *resp)
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

       if (req.get_parseMultipartData_state())
       {    

            std::string username;
            std::string filename;
            std::string uploadtime;
            float duration=0.0;
            bool isvideo=false;

            // ✅ 获取用户名
            username = session->getValue("username");
            LOG_INFO << "上传文件的用户名为: " << username;

            filename=req.get_filename();
            LOG_INFO<<"username:"<<username<<"filename:"<<filename;
            

            // 3. 当前时间字符串
            auto now = std::chrono::system_clock::now();
            std::time_t now_time_t = std::chrono::system_clock::to_time_t(now);
            std::stringstream time_ss;
            time_ss << std::put_time(std::localtime(&now_time_t), "%Y-%m-%d %H:%M");
            
            uploadtime = time_ss.str();

             // 返回成功信息
            json successResp;
            successResp["status"] = "success";
            successResp["message"] = "上传成功";
            successResp["uploadTime"] = time_ss.str();  // 添加时间字段
            

            std::string successBody = successResp.dump();

            resp->setStatusLine(req.getVersion(), http::HttpResponse::k200Ok, "OK");
            resp->setCloseConnection(false);
            resp->setContentType("application/json");
            resp->setContentLength(successBody.size());
            resp->setBody(successBody);

            generateMetaVideo(filename,duration,isvideo);

            // 3. 插入数据库
            if (!insertUploadRecord(filename, username, uploadtime, duration,isvideo)) {
                LOG_WARN << "上传记录写入数据库失败";
            }
            LOG_INFO << "上传记录写入数据库成功";
       }
       else
       {
              throw std::runtime_error("Multipart parsing failed");
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

bool ResourceUploadHandler::generateMetaVideo(const std::string &filename,float &duration,bool &isvideo)
{   
    if (filename.find(".avi")!= std::string::npos ||filename.find(".mp4")!= std::string::npos||filename.find(".mkv")!= std::string::npos)
    {   
        isvideo=true;
        std::string name = filename.substr(0, filename.find_last_of('.'));
        // std::string type = filename.substr(filename.find_last_of('.') + 1);
        std::string thumbnailFile ="/root/uploads/videos/thumbnails/" + name + ".jpg";
        std::string filepath = "/root/uploads/videos/" + filename;
        LOG_INFO<<"进入视频元数据提取成功！";


        // 使用 ffprobe 提取时长
        std::string cmd = "ffprobe -v error -select_streams v:0 -show_entries stream=duration "
                        "-of default=noprint_wrappers=1:nokey=1 \"" + filepath + "\"";
        FILE* pipe = popen(cmd.c_str(), "r");
        if (!pipe) return false;

        char buffer[128];
        std::string durationStr;
        while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
            durationStr += buffer;
        }
        pclose(pipe);

        try
        {
            duration = std::round(std::stof(durationStr)*100.f)/100.f;
        }
        catch (...)
        {
            duration= 0.0;
        }
        
        LOG_INFO<<"视频元数据时长提取成功！";


         // 使用 ffmpeg 生成缩略图（首帧）
        std::string thumbCmd = "ffmpeg -y -i \"" + filepath + "\" -ss 00:00:00 -frames:v 1 \"" + thumbnailFile + "\"";
        int ret = system(thumbCmd.c_str());
        if (ret != 0) {
            std::cerr << "生成缩略图失败\n";
            return false;
        }

        LOG_INFO<<"视频元数据首帧提取成功！";

    }
     return true;
   
}


bool ResourceUploadHandler::insertUploadRecord(const std::string& filename,
                        const std::string& username,
                        const std::string& uploadtime,
                        double duration,
                        bool   isvideo)
{
     std::string sql = R"(INSERT INTO fileinfo (filename, username, uploadtime, duration,isvideo)
                         VALUES (?, ?, ?, ?, ?)
                         ON DUPLICATE KEY UPDATE
                         username = VALUES(username),
                         uploadtime = VALUES(uploadtime),
                         duration = VALUES(duration),
                         isvideo = VALUES(isvideo))";
    try {
        int affected = mysqlUtil_.executeUpdate(sql, filename, username, uploadtime, duration,isvideo);
        return affected > 0;
    } catch (const std::exception& e) {
        LOG_ERROR << "数据库插入失败: " << e.what();
        return false;
    }
}
