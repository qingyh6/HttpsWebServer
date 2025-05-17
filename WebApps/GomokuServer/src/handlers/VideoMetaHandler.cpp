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

      
        LOG_INFO<<"进入视频解析页面";
         // 解析body(json格式)
        json parsed = json::parse(req.getBody());
        std::string videoName = parsed["videoName"];
        LOG_INFO<<"MetaVideo PAGE"<<videoName;

        std::string videoPath="/root/uploads/videos/"+videoName+".mp4";
        std::string thumbPath = "/root/uploads/videos/thumbnails/" + videoName + ".jpg";
        std::string jsonPath = "/root/uploads/videos/metadata/" + videoName + ".json";


        // 生成缩略图
        std::string ffmpegCmd = "ffmpeg -y -i " + videoPath + " -frames:v 1 -q:v 2 " + thumbPath + " > /dev/null 2>&1";
        system(ffmpegCmd.c_str());

        // 获取视频时长
        std::string durationCmd = "ffprobe -v error -show_entries format=duration -of default=noprint_wrappers=1:nokey=1 " + videoPath;
        FILE* pipe = popen(durationCmd.c_str(), "r");
        if (!pipe) return;

        char buffer[128];
        std::string duration;
        while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
            duration += buffer;
        }
        pclose(pipe);

        double durationSec = std::stod(duration);

        // 读取缩略图为二进制数据
        std::ifstream file(thumbPath, std::ios::binary);
        std::ostringstream oss;
        oss << file.rdbuf();
        std::string thumbnailData = oss.str();

        // Base64 编码
        std::string thumbnailBase64 = base64_encode(thumbnailData);

        json respJson_;
        respJson_["name"]=videoName;
        respJson_["duration"]=durationSec;
        respJson_["thumbnail"]="data:image/jpeg;base64," + thumbnailBase64;
        respJson_["video_url"]="/videos/"+videoName+".mp4";
        std::string respBody = respJson_.dump(4);
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