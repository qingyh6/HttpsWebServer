#include "../include/handlers/VideoStreamHandler.h"
#include <fstream>
#include <sstream>
#include <filesystem>

using namespace std;
namespace fs = std::filesystem;

void VideoStreamHandler::handle(const http::HttpRequest &req, http::HttpResponse *resp)
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
    

        std::string path = url_decode(req.path()); // e.g. /video/sample.mp4
        std::string filename = path.substr(strlen("/video/")); // 截取文件名
        std::string videoPath = "/root/uploads/videos" + filename;
        LOG_INFO<<"进入视频播放流"<<videoPath;
        if (!fs::exists(videoPath)) {
            resp->setStatusLine(req.getVersion(), http::HttpResponse::k404NotFound, "Not Found");
            resp->setCloseConnection(true);
            resp->setContentType("text/plain");
            resp->setBody("File not found");
            return;
        }

        std::ifstream file(videoPath, std::ios::binary | std::ios::ate);
        if (!file) {
            resp->setStatusLine(req.getVersion(), http::HttpResponse::k500InternalServerError, "Internal Error");
            resp->setCloseConnection(true);
            resp->setContentType("text/plain");
            resp->setBody("Failed to open file");
            return;
        }

        std::streamsize fileSize = file.tellg();
        file.seekg(0, std::ios::beg);

        // Range: bytes=START-END
        std::string rangeHeader = req.getHeader("Range");
        std::streamsize start = 0, end = fileSize - 1;
        bool isPartial = false;

        if (!rangeHeader.empty()) {
            isPartial = true;
            sscanf(rangeHeader.c_str(), "bytes=%ld-%ld", &start, &end);
            if (end >= fileSize) end = fileSize - 1;
        }

        std::streamsize chunkSize = end - start + 1;
        std::vector<char> buffer(chunkSize);

        file.seekg(start, std::ios::beg);
        file.read(buffer.data(), chunkSize);

        std::string contentType = "video/mp4";

        if (isPartial) {
            resp->setStatusLine(req.getVersion(), http::HttpResponse::k206PartialContent, "Partial Content");
            char rangeHeaderValue[128];
            snprintf(rangeHeaderValue, sizeof(rangeHeaderValue),
                    "bytes %ld-%ld/%ld", start, end, fileSize);
            resp->addHeader("Content-Range", rangeHeaderValue);
        } else {
            resp->setStatusLine(req.getVersion(), http::HttpResponse::k200Ok, "OK");
        }

        resp->addHeader("Accept-Ranges", "bytes");
        resp->setContentType(contentType);
        resp->setContentLength(buffer.size());
        resp->setBody(std::string(buffer.begin(), buffer.end()));
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

std::string VideoStreamHandler::url_decode(std::string encoded) {
    std::string result;
    char ch;
    int i, ii;
    for (i = 0; i < encoded.length(); i++) {
        if (int(encoded[i]) == int('%')) {
            sscanf(encoded.substr(i + 1, 2).c_str(), "%x", &ii);
            ch = static_cast<char>(ii);
            result += ch;
            i += 2;
        }
        else if (encoded[i] == '+') {
            result += ' ';
        }
        else {
            result += encoded[i];
        }
    }
    return result;
}
