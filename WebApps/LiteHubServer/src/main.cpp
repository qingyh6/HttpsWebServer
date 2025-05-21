#include <string>
#include <iostream>
#include <muduo/net/TcpServer.h>
#include <muduo/base/Logging.h>
#include <muduo/net/EventLoop.h>
#include <muduo/base/AsyncLogging.h>
#include <muduo/base/TimeZone.h>
#include "LiteHubServer.h"

// 日志文件滚动大小为1MB (1*1024*1024字节)
static const off_t kRollSize = 1*1024*1024;

muduo::AsyncLogging* g_asyncLog = NULL;
muduo::AsyncLogging * getAsyncLog(){
    return g_asyncLog;
}
void asyncLog(const char* msg, int len)
{
    muduo::AsyncLogging* logging = getAsyncLog();
    if (logging)
    {
      logging->append(msg, len);
    }
}

int main(int argc, char* argv[])
{

  muduo::TimeZone chinaTime(8 * 3600, "CST");
  muduo::Logger::setTimeZone(chinaTime);
  const std::string LogDir="../logs";
  mkdir(LogDir.c_str(),0755);
  std::ostringstream LogfilePath;
  LogfilePath << LogDir << "/" << ::basename(argv[0]); // 完整的日志文件路径

  muduo::AsyncLogging log(LogfilePath.str(), kRollSize);
  g_asyncLog = &log;

  muduo::Logger::setOutput(asyncLog); // 为Logger设置输出回调, 重新配接输出位置
  log.start(); // 开启日志后端线程
  

  LOG_INFO << "pid = " << getpid();


  std::string serverName = "HttpServer";
  int port = 80;
  
  // 参数解析
  int opt;
  const char* str = "p:";
  while ((opt = getopt(argc, argv, str)) != -1)
  {
    switch (opt)
    {
      case 'p':
      {
        port = atoi(optarg);
        break;
      }
      default:
        break;
    }
  }


 
  muduo::Logger::setLogLevel(muduo::Logger::INFO);
  LiteHubServer server(port, serverName);
  server.setThreadNum(4);
  server.start();


  log.stop();
}
