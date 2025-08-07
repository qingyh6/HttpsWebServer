| 测试类型    | 工具             | 场景说明                           |
| ------- | -------------- | ------------------------------ |
| 接口功能测试  | Postman / curl/jmeter| 测试 Range 返回、鉴权校验等逻辑是否正确        |
| 单接口性能测试 | ab             | 测试大文件全量下载的带宽和吞吐极限（但不适合测 Range） |
| 场景并发测试  | JMeter         | 模拟播放器并发拉流场景，逐块下载视频，测 Range 逻辑  |

## 如何避免鉴权
在项目的功能模块中，是通过会话管理来确保用户登录了的。但是为了性能测试，不可能模拟每一个真实用户（创建不同的账号）来访问，实际上在本项目约定了一个字段，只有在请求头中带上字段**TestBypass=perf-token**，就可以有效地跳过鉴权，而专注于在服务器中比较容易出现瓶颈的地方。在本项目中，比较容易出现瓶颈的地方有视频的播放（同一时刻大量用户请求），大文件上传下载等，这些等都是可能引起性能瓶颈的地方，测试时也是主要关注这些部分的。
  ```c++
    auto session = server_->getSessionManager()->getSession(req, resp);
    bool isTestBypass = false;
    std::string bypassHeader = req.getHeader("TestBypass"); //如果是测试的话，在发送请求时带上这个字段就可以
    if (bypassHeader == "perf-token") {
        LOG_INFO << "[PerfTest] Bypassing login check.";
        isTestBypass = true;
    }

    if (session->getValue("isLoggedIn") != "true" && !isTestBypass) {
        json errorResp;
        errorResp["status"] = "error";
        errorResp["message"] = "Unauthorized";
        std::string errorBody = errorResp.dump();
        server_->packageResp(req.getVersion(), http::HttpResponse::k401Unauthorized,
                            "Unauthorized", true, "application/json",
                            errorBody.size(), errorBody, resp);
        return;
    }
  ```
## 接口功能测试
### 视频range字段请求测试
这里测试的是视频的Range字段请求是否有效，视频播放功能
分析如下，模拟了50个线程（模拟用户），每个线程发送20次请求，在10秒内发送所有请求（50*20=1000次请求）。
![单范围查询](https://github.com/user-attachments/assets/0345c1fa-4550-404d-8c8c-b6b84e1d2c65)

随后开了一个线程，依次请求0-3000，3000-8000，8000-16000，16000-20000，20000-30000的range范围的数据（设置字段的请求的范围不一样）
![多节点访问查询](https://github.com/user-attachments/assets/a54a5903-9914-4a5d-bd95-ebbefc0a9156)


## 接口性能测试

### 视频播放接口测试
📊 测试对比总览：
| 指标项           | 大视频 (86MB)                | 小视频 (7.9MB)             |
| ------------- | ------------------------- | ----------------------- |
| 请求总数 (`-n`)   | 1000                      | 1000                    |
| 并发数 (`-c`)    | 100                       | 100                     |
| 总耗时           | **556 秒**                 | **20 秒**                |
| 平均 RPS（请求数/秒） | **1.80 req/s**            | **49.60 req/s**         |
| 平均每个请求耗时（总视角） | **55654 ms / req**        | **2016 ms / req**       |
| 平均每个请求耗时（单请求） | **556.5 ms / req**        | **20.16 ms / req**      |
| 最大请求耗时        | **318,324 ms（超 5 分钟）**    | **3052 ms**             |
| 传输速率          | 151 MB/s                  | 375 MB/s                |
| 请求失败          | 0                         | 0                       |

对自研 HttpServer 框架的视频接口进行了性能压测，通过 Apache Benchmark 工具分别对小视频（8MB）与大视频（86MB）进行 100 并发、1000 请求的全流程下载测试，测试内容包括：\
高并发下的视频接口稳定性和资源利用；\
使用 Header 设计绕过鉴权逻辑，实现纯净性能测试；\
捕获视频文件读取、响应时间、吞吐量等关键指标；\
对大视频请求中暴露的 CPU 内存占用高、I/O 阻塞、请求长尾等瓶颈点进行逐步优化。\
\
✅ 测试结果：小文件接口处理速率约 49 req/s，总耗时 20s；大文件接口处理速率约 1.8 req/s，总耗时达 556s。

下面是通过Apache Bench (ab) 压测的具体截图
测试小视频文件的播放（7.9M）
  ```shell
  ab -n 1000 -c 100  -H "TestBypass:perf-token" http://192.168.218.128/videos/test.mp4
  ```
<img width="846" height="591" alt="image" src="https://github.com/user-attachments/assets/beade982-b420-4549-b838-036c8f30662a" />

测试大视频文件的播放(86M)
 ```shell
  ab -n 1000 -c 100  -H "TestBypass:perf-token" http://192.168.218.128/videos/safe%20and%20sound.mp4
  ```
<img width="672" height="697" alt="image" src="https://github.com/user-attachments/assets/4dc08af0-86f5-4877-9cf1-11ba956ccb24" />

###　视频下载性能测试
![多线程下载](https://github.com/user-attachments/assets/d2fe7c39-a709-4a2c-9524-a1b4168b412b)
|      | 请求数 | 平均响应时间(s) | 最小响应时间(s) | 最大响应时间(s) | 标准偏差（s） | 异常 % | 吞吐量 | 接收 MB/sec | 发送 KB/sec | 平均字节大小（MB/request） |
| ---- | :----: | :-------------: | :-------------: | :-------------: | :-----------: | :----: | :----: | :---------: | :---------: | :------------------------: |
| 下载 |  1000  |       1.6       |      0.022      |       12        |      1.6      |   0%   | 31.51  |     240     |    7.85     |            7.56            |

🔍 性能分析总结
1. 整体表现不错，未出现错误请求
1000 个请求全部成功。
“异常 %”为 0%，说明后端在压力下稳定性好。

3. 响应时间波动大
平均值为 1.6 秒，但最大值为 12 秒，标准差达 1.6 秒，说明部分请求可能被队列延迟、服务器忙、磁盘 IO 受限等影响。

4. 吞吐率适中
31.5 个请求/秒在文件下载场景下属于不错的表现，考虑到单个文件约 7.5MB，这代表总下载速率约：＇31.5 * 7.5MB ≈ 236MB/s＇
与“接收 KB/sec”数据对得上，说明带宽或文件读取能力没有成为瓶颈。

4. 网络传输稳定，资源利用率较高
高接收吞吐量说明网络带宽、服务器磁盘读取能力尚可，适合支持高并发下载。


## 场景并发测试
