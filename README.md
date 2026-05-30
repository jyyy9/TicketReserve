# 票务预订系统

基于 C++ 的票务预订系统，采用 libevent 实现高性能网络通信，后端使用 MySQL 存储数据，Redis 作为缓存。

## 项目结构

```
TicketReserve/
├── client/            # 客户端
│   ├── client.cpp     # 客户端实现
│   └── client.h       # 客户端头文件
├── common/            # 公共模块
│   └── OpType.h       # 操作类型枚举定义
├── db/                # 数据库模块
│   ├── mysql_client.h/cpp    # MySQL 客户端
│   ├── mysql_pool.h/cpp      # MySQL 连接池
│   ├── redis_client.h/cpp    # Redis 客户端
│   └── redis_pool.h/cpp      # Redis 连接池
├── server/            # 服务端
│   ├── server.cpp     # 服务端实现
│   └── server.h       # 服务端头文件
└── .vscode/           # VSCode 配置
```

## 功能列表

- 用户注册
- 用户登录
- 查看可预约票务
- 预订票务
- 查看我的预约
- 取消预约
- 退出登录

## 依赖环境

- C++ 编译器 (gcc/g++)
- libevent
- MySQL
- Redis
- jsoncpp
- hiredis

## 编译

### 服务端

```bash
cd server
g++ -o server server.cpp ../db/*.cpp -I../common -levent -lmysqlclient -lhiredis -ljsoncpp -lpthread
```

### 客户端

```bash
cd client
g++ -o client client.cpp -I../common -ljsoncpp -lpthread
```

## 运行

### 启动服务端

```bash
cd server
./server
```

### 启动客户端

```bash
cd client
./client
```

## 配置说明

- 服务端默认监听地址: `127.0.0.1:6789`
- MySQL 和 Redis 配置可在 `db/` 目录下的源文件中修改
