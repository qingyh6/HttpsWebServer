#pragma once

#include "SessionStorage.h"
#include "../http/HttpRequest.h"
#include "../http/HttpResponse.h"
#include <memory>
#include <random>
#include <thread>
#include <atomic>

namespace http
{
namespace session
{

class SessionManager
{
public:
    explicit SessionManager(std::unique_ptr<SessionStorage> storage);
    ~SessionManager();
    // 从请求中获取或创建会话
    std::shared_ptr<Session> getSession(const HttpRequest& req, HttpResponse* resp);
    
     // 销毁会话
    void destroySession(const std::string& sessionId);

    // 清理过期会话
    void cleanExpiredSessions();

    // 更新会话
    void updateSession(std::shared_ptr<Session> session)
    {
        storage_->save(session);
    }
private:
    std::string generateSessionId();
    std::string getSessionIdFromCookie(const HttpRequest& req);
    void setSessionCookie(const std::string& sessionId, HttpResponse* resp);

private:
    std::unique_ptr<SessionStorage> storage_;
    std::mt19937 rng_; // 用于生成随机会话id
    std::thread cleanerThread_;      // 定时清理会话的后台线程
    std::atomic<bool> stopCleaner_;  // 线程停止标志
};

} // namespace session
} // namespace http
