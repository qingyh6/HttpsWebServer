#include "../../../include/middleware/gzip/GzipMiddleware.h"
#include <algorithm>
#include <sstream>
#include <iostream>
#include <zlib.h>
#include <vector>
#include <muduo/base/Logging.h>

namespace http 
{
namespace middleware 
{

void GzipMiddleware::before(HttpRequest& request)
{
    std::string acceptEncoding=request.getHeader("Accept-Encoding");
    //判断前端是否支持gzip字段，如果不支持，就不进行gzip压缩
    acceptEncoding.find("gzip") != std::string::npos?setClientSupportGzip(true):setClientSupportGzip(false);

}
void GzipMiddleware::after(HttpResponse& response) 
{
    if (!isClinetSupportGzip())
        return;

    if (!response.isShouldGzipCompress()) //判断是否应该压缩，消息体大于256字节并且消息类型是文本、html等类型才可以
        return;
    const std::string& rawBody = response.getBody();

    std::string compressed;
    if (compressGzip(rawBody, compressed)) {
        
        response.addHeader("Content-Encoding", "gzip");
        response.setContentLength(compressed.size());
        response.setBody(compressed);
    }
}

bool GzipMiddleware::compressGzip(const std::string& input, std::string& output)
{
    constexpr int CHUNK = 16384; 
    z_stream strm{};    //zlib 用于压缩的状态结构体，记录输入、输出缓冲区状态等
    char out[CHUNK];    //输出缓冲区，用来暂存压缩后的数据块

    strm.zalloc = Z_NULL;
    strm.zfree = Z_NULL;
    strm.opaque = Z_NULL;

    if (deflateInit2(&strm,             //压缩状态
                     Z_BEST_COMPRESSION, //压缩等级（0~9），9 表示最高压缩比，牺牲性能
                     Z_DEFLATED,         //使用 DEFLATE 算法
                     15 + 16,           //15位窗口大小(32KB), +16启用 GZIP 格式输出（否则是 zlib）
                     8,                 //内部压缩缓冲区大小参数，一般为 8
                     Z_DEFAULT_STRATEGY) != Z_OK) //默认压缩策略
    {
        return false;
    }

    strm.avail_in = input.size();          // 待压缩数据长度
    strm.next_in = (Bytef*)input.data();   // 待压缩数据

    do {
        strm.avail_out = CHUNK;            //待压缩数据存储buffer 的长度，如果多次写，会覆盖之前的写的数据
                                            //当然，之前的数据已经被读走了
        strm.next_out = reinterpret_cast<Bytef*>(out); //待压缩数据存储的buffer
        deflate(&strm, Z_FINISH);            //如果输入和待输出的数据都被处理完，则返回 Z_STREAM_END
        size_t have = CHUNK - strm.avail_out;//总长度-当前可写=已经写的数据长度
        output.append(out, have);
    } while (strm.avail_out == 0);

    deflateEnd(&strm);                       //释放deflateInit2申请的空间
    LOG_INFO<<"原始的数据大小为:"<< input.size();
    LOG_INFO<<"GZIP压缩完成,压缩比例为:"<<(static_cast<double>(output.size()) / input.size());;
    return true;
}



} // namespace middleware
} // namespace http