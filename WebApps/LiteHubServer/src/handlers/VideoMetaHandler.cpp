#include "../include/handlers/VideoMetaHandler.h"



void VideoMetaHandler::handle(const http::HttpRequest &req, http::HttpResponse *resp)
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

        // 从数据库中查询 isvideo = 1 的所有文件记录
        std::string sql = "SELECT filename, duration FROM fileinfo WHERE isvideo = 1";
        std::unique_ptr<sql::ResultSet> rs(mysqlUtil_.executeQuery(sql));
        json result = json::array();  // 构建最终的 JSON 数组返回值


        while (rs->next()) {
            std::string videoName = rs->getString("filename");
            double duration = rs->getDouble("duration");

            LOG_INFO << "videoName: " << videoName;

            std::string name=videoName.substr(0, videoName.find_last_of('.'));
            // 拼接缩略图路径
            std::string thumbPath = "/root/uploads/videos/thumbnails/" + name + ".jpg";
            std::ifstream thumbFile(thumbPath, std::ios::binary);
            std::ostringstream oss;

            if (thumbFile) {
                oss << thumbFile.rdbuf();
            }

            std::string thumbnailBase64 = base64_encode(oss.str());

            LOG_INFO<<"进入视频元信息:name"<<name;
            // 构造 JSON 对象
            json videoInfo = {
                {"name", name},
                {"duration", duration},
                {"thumbnail", "data:image/jpeg;base64," + thumbnailBase64},
                {"video_url", videoName}
            };

            result.push_back(videoInfo);
        }


        std::string respBody = result.dump(4);
        resp->setStatusLine(req.getVersion(), http::HttpResponse::k200Ok, "OK");
        resp->setCloseConnection(false);
        resp->setContentType("application/json");
        resp->setContentLength(respBody.size());
        resp->setBody(respBody);
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


std::string VideoMetaHandler::base64_encode(const std::string &in) {
    std::string out;
    BIO *b64 = BIO_new(BIO_f_base64());
    BIO *bio = BIO_new(BIO_s_mem());
    b64 = BIO_push(b64, bio);
    BIO_set_flags(b64, BIO_FLAGS_BASE64_NO_NL);
    BIO_write(b64, in.data(), in.size());
    BIO_flush(b64);
    BUF_MEM *bufferPtr;
    BIO_get_mem_ptr(b64, &bufferPtr);
    out.assign(bufferPtr->data, bufferPtr->length);
    BIO_free_all(b64);
    return out;
}