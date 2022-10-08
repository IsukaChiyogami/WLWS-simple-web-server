# WLWS-simple-web-server
my web server in C , coding with learning
# W_http_server_C3系列
> * C3U:通用版 universal version
> * C3D:适用于智能桌面壁纸的特化版，配套网页见<kbd>vwallpaper_web_pages</kbd> just for vwallpaper, with a set of web pages in <kbd>vwallpaper_web_pages</kbd>
>> * 控制台支持4条指令(c console support 4 command):
>>> * quit/exit 退出
>>> * stop 暂停
>>> * continue 继续
>>> * reload 重载预缓存首页
>> * 使用请注意　read before using
>>> * 预存index.html作为主页(在预处理器宏可修改) prestore index.html as home page
>>> * 需要自定义的预定义宏有 the following list the predefined macros which need user-define：
>>>> * RECORD_IN_FILE  可定义为任意值,定义后消息会被记录在文件中,否则仅在控制台打印 could be defined as any value , if be defined , the message would be record in record file and print on the console, instead, it would just print on console
>>>> * RECORD_FILE_NAME  默认记录文件 record file path
>>>> * HTTP_VERSION   HTTP版本(只支持1.0与1.1) only support 1.0 and 1.1
>>>> * ROOT_PATH   根目录(相对绝对都可以) root path of web pages
>>>> * RECV_BUFFER_SIZE 首次接收大小,必须大于HTTP请求头大小 the buffer size of string recv first time, should be more than HTTP header size (recommanded 8192)
>>>> * RECV_MAX_CONTENT_LENGTH 超过这个大小返回 413	Request Entity Too Large return 413 if HTTP request over this size
>>>> * 预定义宏中关于HTTP标头数量(headers store size is define in predefined macros)：
>>>>> * HTTP标头使用链表储存(每个单元含有x单元字典)，储存总量/total size = MAX_HTTP_HEADER_PAGE_NUM * MAX_UNIT_PER_PAGE_IN_CHAIN 
>> * 二次开发注意 read before secondary development
>>> * 没有任何特殊请求处理(在reply_client函数中开发即可特别对待指定请求) make sepcial response in function "reply_client"
>>> * 关于cookie和url中的query的解析程序还没写，如果需要，请自行添加 there is nothing about parsing string of cookie and query in url, write it if needed
> ---------------
> 程序结构 structure
>> main
>
>> main多线程(create thread)=><kbd>HTTP_server_loop</kbd>(循环等待连接 loop waiting for connection)
>
>> <kbd>HTTP_server_loop</kbd>被链接(be connected)=>创建线程(create thread)=><kbd>HTTP_connection_handle_thread</kbd>，然后继续循环
>
>><kbd>HTTP_connection_handle_thread</kbd>(<kbd>HTTP_connection_handle</kbd>返回1时保持连接，0则断(return 1 to keep connection, 0 to break it))
>>> <kbd>HTTP_connection_handle</kbd>处理连接
>>>> <kbd>reply_client</kbd>回应客户端 <p style="color:cyan">( 重点自定义函数，其他基本不用修改 to do special reply in this function )</p>
>>> <br>
>> <br>
> <p style="color:orangered">3518767065@qq.com</p>
