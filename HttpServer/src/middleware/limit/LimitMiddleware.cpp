
#include "../../../include/middleware/limit/LimitMiddleware.h"
#include <algorithm>

namespace http
{
namespace middleware
{

using namespace std::chrono;

LimitMiddleware::LimitMiddleware(int rate, int capacity)
    : rate_(rate),
      capacity_(capacity),
      tokens_(capacity),  // 初始桶是满的
      lastRefillTime_(steady_clock::now())
{
}

// 令牌补充逻辑
void LimitMiddleware::refillTokens()
{
    auto now = steady_clock::now();
    auto elapsedMs = duration_cast<milliseconds>(now - lastRefillTime_).count();

    if (elapsedMs > 0)
    {
        double newTokens = (elapsedMs / 1000.0) * rate_;
        tokens_ = std::min((double)capacity_, tokens_ + newTokens);
        lastRefillTime_ = now;
    }
}

// 在处理请求前限流
void LimitMiddleware::before(HttpRequest& request)
{
    std::lock_guard<std::mutex> lock(mutex_);

    refillTokens();

    if (tokens_ >= 1.0)
    {
        tokens_ -= 1.0;
        // 有令牌，允许继续
    }
    else
    {   
        HttpResponse resp;
        resp.setStatusLine(request.getVersion(), http::HttpResponse::k429TooManyRequests, "Too Many Requests");
        resp.setCloseConnection(true);
        resp.setContentType("application/json");
        resp.setContentLength(0);
        resp.setBody("Rate limit exceeded. Please try again later.");

        throw resp;
    }
}

} // namespace middleware
} // namespace http
