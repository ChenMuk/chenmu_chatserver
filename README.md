# chenmu_chatserver
使用nginx负载均衡，redis消息队列，mysql，基于muduo网络库的聊天服务器和客户端集群项目

编译方式：
cd build
cmake ..
make

运行环境：
1. ubuntu20.04
1. nginx tcp负载均衡
     参考include/test.conf 来配置nginx.conf
3. redis server
     需要安装redis 以及hiredis
5. mysql
6. muduo
7. cmake
