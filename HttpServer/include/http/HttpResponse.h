#pragma once

#include <muduo/net/TcpServer.h>

namespace http
{

class HttpResponse 
{
public:
    enum HttpStatusCode
    {
        kUnknown,
        k200Ok = 200,
        
        k204NoContent = 204,
        k206PartialContent=206,
        k301MovedPermanently = 301,
        k400BadRequest = 400,
        k401Unauthorized = 401,
        k403Forbidden = 403,
        k404NotFound = 404,
        k409Conflict = 409,
        k429TooManyRequests = 429,
        k500InternalServerError = 500,
    };

    HttpResponse(bool close = true)
        : statusCode_(kUnknown)
        , closeConnection_(close)
        , isFileResponse_(false)
    {}

    void setVersion(std::string version)
    { httpVersion_ = version; }
    void setStatusCode(HttpStatusCode code)
    { statusCode_ = code; }

    HttpStatusCode getStatusCode() const
    { return statusCode_; }

    void setStatusMessage(const std::string message)
    { statusMessage_ = message; }

    void setCloseConnection(bool on)
    { closeConnection_ = on; }

    bool closeConnection() const
    { return closeConnection_; }
    
    void setContentType(const std::string& contentType)
    { addHeader("Content-Type", contentType); }

    void setContentLength(uint64_t length)
    { addHeader("Content-Length", std::to_string(length)); }

    void addHeader(const std::string& key, const std::string& value)
    { headers_[key] = value; }
    
    void setBody(const std::string& body)
    { 
        body_ = body;
        // body_ += "\0";
    }

    //这是我自己定义的，专门用于中间件gzip压缩。
    std::string getBody() const
    { 
        return body_;
    }
    bool isShouldGzipCompress() 
    {   
        if (body_.size()<256)
            return false;
        std::string contentType=headers_["Content-Type"];
        return contentType.find("text/") != std::string::npos ||
               contentType.find("application/json") != std::string::npos ||
               contentType.find("application/javascript") != std::string::npos ||
               contentType.find("application/xml") != std::string::npos ||
               contentType.find(".js") != std::string::npos ||
               contentType.find(".css") != std::string::npos ||
               contentType.find(".html") != std::string::npos; 
    }

    void setStatusLine(const std::string& version,
                         HttpStatusCode statusCode,
                         const std::string& statusMessage);

    void setErrorHeader(){}

    void appendToBuffer(muduo::net::Buffer* outputBuf) const;


    bool isFileResponse() const {return isFileResponse_;}
    std::string getFilePath() {return filePath_;}
    void setisFileResponse(const std::string& path)
    {
         isFileResponse_ = true;
         filePath_ = path;
    }
private:
    std::string                        httpVersion_; 
    HttpStatusCode                     statusCode_;
    std::string                        statusMessage_;
    bool                               closeConnection_;
    std::map<std::string, std::string> headers_;
    std::string                        body_;
    bool                               isFileResponse_; //判断是否是文件，如果是，采用流式发送
    std::string                        filePath_;
};

} // namespace http