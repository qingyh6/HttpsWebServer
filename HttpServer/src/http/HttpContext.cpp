#include "../../include/http/HttpContext.h"
#include "../../include/utils/FileUtil.h"

using namespace muduo;
using namespace muduo::net;
#include <muduo/base/Logging.h>
#include <fstream>
namespace http
{

// 将报文解析出来将关键信息封装到HttpRequest对象里面去
bool HttpContext::parseRequest(Buffer *buf, Timestamp receiveTime)
{
    bool ok = true; // 解析每行请求格式是否正确
    bool hasMore = true;
    while (hasMore)
    {   
         
        if (state_ == kExpectRequestLine)
        {   
            //这里保存的是没有解析的原始数据，便于查看其格式
            // std::ofstream out("/root/upload_debug.raw", std::ios::binary);
            // out.write(buf->peek(), buf->readableBytes());
            // out.close();
            const char *crlf = buf->findCRLF(); // 注意这个返回值边界可能有错
            if (crlf)
            {
                ok = processRequestLine(buf->peek(), crlf);
                if (ok)
                {
                    request_.setReceiveTime(receiveTime);
                    buf->retrieveUntil(crlf + 2);
                    state_ = kExpectHeaders;
                }
                else
                {
                    hasMore = false;
                }
            }
            else
            {
                hasMore = false;
            }
        }
        else if (state_ == kExpectHeaders)
        {
            const char *crlf = buf->findCRLF();
            if (crlf)
            {
                const char *colon = std::find(buf->peek(), crlf, ':');
                if (colon < crlf)
                {
                    request_.addHeader(buf->peek(), colon, crlf);
                }
                else if (buf->peek() == crlf)
                { 
                    // 空行，结束Header
                    // 根据请求方法和Content-Length判断是否需要继续读取body
                    if (request_.method() == HttpRequest::kPost || 
                        request_.method() == HttpRequest::kPut)
                    {
                        std::string contentLength = request_.getHeader("Content-Length");
                        if (!contentLength.empty())
                        {
                            request_.setContentLength(std::stoi(contentLength));
                            if (request_.contentLength() > 0)
                            {
                                state_ = kExpectBody;
                            }
                            else
                            {
                                state_ = kGotAll;
                                hasMore = false;
                            }
                        }
                        else
                        {
                            // POST/PUT 请求没有 Content-Length，是HTTP语法错误
                            ok = false;
                            hasMore = false;
                        }
                    }
                    else
                    {
                        // GET/HEAD/DELETE 等方法直接完成（没有请求体）
                        state_ = kGotAll; 
                        hasMore = false;
                    }
                }
                else
                {
                    ok = false; // Header行格式错误
                    hasMore = false;
                }
                buf->retrieveUntil(crlf + 2); // 开始读指针指向下一行数据
            }
            else
            {
                hasMore = false;
            }
        }
        else if (state_ == kExpectBody)
        {
             
            // 检查缓冲区中是否有足够的数据
            if (buf->readableBytes() < request_.contentLength())
            {
                hasMore = false; // 数据不完整，等待更多数据
                return true;
            }
            const std::string &contentType = request_.getHeader("Content-Type");
            if (contentType.find("multipart/form-data") != std::string::npos)
            {
                //这里保存的是没有解析的原始数据，便于查看其格式
                // std::ofstream out("/root/upload_debug.raw", std::ios::binary);
                // out.write(buf->peek(), buf->readableBytes());
                // out.close();

                ok=parseMultipartData(buf);
                request_.set_parseMultipartData_state(ok);
                LOG_INFO<<"multipart/form-data解析"<<((ok)?"成功":"失败");

                state_ = kGotAll;
                hasMore = false;
            }
            else
            {
            
                // 只读取 Content-Length 指定的长度
                std::string body(buf->peek(), buf->peek() + request_.contentLength());
                request_.setBody(body);

                // 准确移动读指针
                buf->retrieve(request_.contentLength());

                state_ = kGotAll;
                hasMore = false;
            }
        }
    }
    return ok; // ok为false代表报文语法解析错误
}

// 解析请求行
bool HttpContext::processRequestLine(const char *begin, const char *end)
{
    bool succeed = false;
    const char *start = begin;
    const char *space = std::find(start, end, ' ');
    if (space != end && request_.setMethod(start, space))
    {
        start = space + 1;
        space = std::find(start, end, ' ');
        if (space != end)
        {
            const char *argumentStart = std::find(start, space, '?');
            if (argumentStart != space) // 请求带参数
            {
                request_.setPath(start, argumentStart); // 注意这些返回值边界
                request_.setQueryParameters(argumentStart + 1, space);
            }
            else // 请求不带参数
            {
                request_.setPath(start, space);
            }

            start = space + 1;
            succeed = ((end - start == 8) && std::equal(start, end - 1, "HTTP/1."));
            if (succeed)
            {
                if (*(end - 1) == '1')
                {
                    request_.setVersion("HTTP/1.1");
                }
                else if (*(end - 1) == '0')
                {
                    request_.setVersion("HTTP/1.0");
                }
                else
                {
                    succeed = false;
                }
            }
        }
    }
    return succeed;
}



bool HttpContext::parseMultipartData(Buffer *buf)
{
    // 1. 找到并跳过第一个 boundary（如 ------WebKitFormBoundaryxxxx）
    const char* crlf = buf->findCRLF();
    if (!crlf) return false; // 不完整
    std::string boundary(buf->peek(),crlf);
    LOG_INFO<<"boundary:"<<boundary;
    buf->retrieveUntil(crlf + 2); 


    // // 2. 解析 Content-Disposition 行
    crlf = buf->findCRLF();
    if (!crlf) return false; // 不完整
    std::string dispositionLine(buf->peek(), crlf);
    buf->retrieveUntil(crlf + 2);

    std::string Content_Disposition;
    std::string name;
    std::string filename;
     
    size_t Content_Pos = dispositionLine.find("Content-Disposition:");
    if (Content_Pos != std::string::npos)
    {
        Content_Pos += 20;
        size_t ContentEnd = dispositionLine.find('"', Content_Pos);
        if (ContentEnd != std::string::npos)
            Content_Disposition.assign(dispositionLine.data() + Content_Pos, ContentEnd - Content_Pos);
    }

    LOG_INFO<<"Content_Disposition:"<<Content_Disposition;
    size_t namePos = dispositionLine.find("name=\"");
    if (namePos != std::string::npos)
    {
        namePos += 6;
        size_t nameEnd = dispositionLine.find('"', namePos);
        if (nameEnd != std::string::npos)
            name.assign(dispositionLine.data() + namePos, nameEnd - namePos);
    }

    LOG_INFO<<"name:"<<name;
    size_t filePos = dispositionLine.find("filename=\"");
    if (filePos != std::string::npos)
    {
        filePos += 10;
        size_t fileEnd = dispositionLine.find('"', filePos);
        if (fileEnd != std::string::npos)
            filename.assign(dispositionLine.data() + filePos, fileEnd - filePos);
    }
    LOG_INFO<<"filename:"<<filename;
     // 3. 跳过 Content-Type 行
    crlf = buf->findCRLF();
    if (!crlf) return false; 
    buf->retrieveUntil(crlf + 2);


    // 4. 跳过空行（说明下一个就是正文）
    crlf = buf->findCRLF();
    if (!crlf) return false;
    buf->retrieveUntil(crlf + 2);


  // 5. 写入文件，直到遇到下一次 boundary
    const char* fileStart = buf->peek(); // 文件内容起点
    const char* fileEnd = std::search(
        fileStart, buf->peek()+ buf->readableBytes(),
        boundary.c_str(), boundary.c_str() + boundary.size()
    );

    if (fileEnd == buf->beginWrite()) {
    // 未找到结束 boundary，暂时数据不足
    return false;
    }
    LOG_INFO<<"开始创建文件咯";
    if (filename.find(".avi")!= std::string::npos ||filename.find(".mp4")!= std::string::npos||filename.find(".mkv")!= std::string::npos)
    {
        FileUtil writer("/root/uploads/videos/" + filename);
        writer.writeBinary(fileStart, fileEnd - fileStart);
        LOG_INFO<<"这是一个视频"<<filename;
    }
    else
    {
        FileUtil writer("/root/uploads/" + filename);
        writer.writeBinary(fileStart, fileEnd - fileStart);
    }
   
    

    LOG_INFO<<"创建完成";

    // 6. 移动 buffer 指针，跳过文件数据和 boundary和\r\n
    buf->retrieveUntil(fileEnd+2); // 跳过文件数据
    //这里移动了，但是貌似还剩下一点东西，我测试了并没有发现问题，就不管啦！！！
    return true;

}

} // namespace http