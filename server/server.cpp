#include "server.h"
#include "db/mysql_client.h"
#include "db/redis_client.h"
#include "db/redis_pool.h"

<<<<<<< HEAD
void SOCK_CON_CALLBACK(int sockfd, short ev, void* arg);
void SOCK_LIS_CALLBACK(int sockfd, short ev, void* arg);
=======
// 创建全局线程池
ThreadPool g_pool(20); // 20个工作线程

void SOCK_CON_CALLBACK(int sockfd, short ev, void *arg); // 函数声明
>>>>>>> 997ecac1d62ed16b323db5dc5da067eade2cba44

bool socket_listen::socket_init()
{
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (-1 == sockfd)
    {
        return false;
    }

    struct sockaddr_in saddr;
    memset(&saddr, 0, sizeof(saddr));
    saddr.sin_family = AF_INET;
    saddr.sin_port = htons(m_port);
    saddr.sin_addr.s_addr = inet_addr(m_ips.c_str());

    int res = bind(sockfd, (struct sockaddr*)&saddr, sizeof(saddr));
    if (res == -1)
    {
        cout << "bind err!" << endl;
        close(sockfd);
        return false;
    }

    res = listen(sockfd, LIS_MAX);
    if (-1 == res)
    {
        return false;
    }
    return true;
}

int socket_listen::accept_client()
{
    int c = accept(sockfd, NULL, NULL);
    return c;
}

bool socket_listen::add_client(int c)
{
    socket_con* q = new socket_con(c);
    struct event* c_ev = event_new(this->base, c, EV_READ | EV_PERSIST, SOCK_CON_CALLBACK, q);
    if (c_ev == NULL)
    {
        close(c);
        delete q;
        return false;
    }
    q->Set_ev(c_ev);
    event_add(c_ev, NULL);
    return true;
}

void socket_con::Send_err()
{
    Json::Value res_val;
    res_val["status"] = "ERR";

    writer.omitEndingLineFeed();
    string send_str = writer.write(res_val);
    int len = send_str.size();

    char send_buff[4096];
    *(int *)send_buff = htonl(len);
    memcpy(send_buff + 4, send_str.c_str(), len);
    send(c, send_buff, len + 4, 0);
}

void socket_con::Send_ok()
{
    Json::Value res_val;
    res_val["status"] = "OK";

    writer.omitEndingLineFeed();
    string send_str = writer.write(res_val);
    int len = send_str.size();

    char send_buff[4096];
    *(int *)send_buff = htonl(len);
    memcpy(send_buff + 4, send_str.c_str(), len);
    send(c, send_buff, len + 4, 0);
}

void socket_con::User_Register()
{
    username = val["user_name"].asString();
    usertel = val["user_tel"].asString();
    passwd = val["user_passwd"].asString();
    realname = val["real_name"].asString();
    gen = static_cast<GENDER>(val["gender"].asInt());
    id_card = val["id_card"].asString();

    if (usertel.empty() || username.empty() || passwd.empty() ||
        realname.empty() || static_cast<int>(gen) < 0 ||
        static_cast<int>(gen) > 2 || id_card.empty())
    {
        Send_err();
        return;
    }

    mysql_client cli;
    if (!cli.mysql_Register(username, passwd, realname, id_card, usertel, gen))
    {
        Send_err();
        return;
    }
    Send_ok();
}

void socket_con::User_Login()
{
    usertel = val["user_tel"].asString();
    passwd = val["user_passwd"].asString();

    mysql_client cli;
    if (!cli.mysql_Login(usertel, passwd, username))
    {
        Send_err();
        return;
    }

    Json::Value res_val;
    res_val["status"] = "OK";
    res_val["user_name"] = username;

    writer.omitEndingLineFeed();
    string send_str = writer.write(res_val);
<<<<<<< HEAD
    send(c, send_str.c_str(), send_str.size(), 0);
=======
    int len = send_str.size();

    char send_buff[4096];
    *(int *)send_buff = htonl(len);
    memcpy(send_buff + 4, send_str.c_str(), len);
    send(c, send_buff, len + 4, 0);
    return;
>>>>>>> 997ecac1d62ed16b323db5dc5da067eade2cba44
}

void socket_con::User_Show_Ticket()
{
    cout << "User_Show_Ticket start" << endl;
    Json::Value resval;

    mysql_client cli;
    if (!cli.mysql_User_Show_Ticket(resval))
    {
        cout << "mysql_User_Show_Ticket failed" << endl;
        Send_err();
        return;
    }
    cout << "mysql_User_Show_Ticket success, sending response" << endl;

    Json::FastWriter writer;
    writer.omitEndingLineFeed();
<<<<<<< HEAD
    send(c, writer.write(resval).c_str(), writer.write(resval).size(), 0);
=======
    string send_str = writer.write(resval);
    int len = send_str.size();

    char send_buff[4096];
    *(int *)send_buff = htonl(len);
    memcpy(send_buff + 4, send_str.c_str(), len);
    send(c, send_buff, len + 4, 0);
    cout << "response sent" << endl;
    return;
>>>>>>> 997ecac1d62ed16b323db5dc5da067eade2cba44
}

void socket_con::User_Book_Ticket()
{
    int ticket_id = val["ticket_id"].asInt();
    usertel = val["user_tel"].asString();
    int num = val["book_num"].asInt();

    mysql_client cli;
    if (!cli.mysql_User_Book_Ticket(ticket_id, usertel, num))
    {
        Send_err();
        return;
    }
    Send_ok();
}

void socket_con::User_Show_My_Ticlet()
{
    usertel = val["user_tel"].asString();

    mysql_client cli;
    if (!cli.mysql_User_Show_My_Ticlet(val, usertel))
    {
        Send_err();
        return;
    }

    val["status"] = "OK";
    Json::FastWriter writer;
    writer.omitEndingLineFeed();
<<<<<<< HEAD
    send(c, writer.write(val).c_str(), writer.write(val).size(), 0);
=======
    string send_str = writer.write(val);
    int len = send_str.size();

    char send_buff[4096];
    *(int *)send_buff = htonl(len);
    memcpy(send_buff + 4, send_str.c_str(), len);
    send(c, send_buff, len + 4, 0);
    return;
>>>>>>> 997ecac1d62ed16b323db5dc5da067eade2cba44
}

void socket_con::User_Cancel_Ticket()
{
    usertel = val["user_tel"].asString();
    int ticketid = val["ticket_id"].asInt();
    int num = val["book_num"].asInt();

    mysql_client cli;
    if (!cli.mysql_User_Cancel_Ticket(ticketid, usertel, num))
    {
        Send_err();
        return;
    }
    Send_ok();
}

void socket_con::Recv_data()
{
<<<<<<< HEAD
    char len_buff[4] = {0};
    int n = recv(c, len_buff, 4, MSG_WAITALL);
=======
    // 1.接收长度
    char len_buff[4]{0};
    int n = recv(c, len_buff, 4, MSG_WAITALL); // recv收到数据(报文)
>>>>>>> 997ecac1d62ed16b323db5dc5da067eade2cba44
    if (4 != n)
    {
        cout << "client close" << endl;
        delete this;
        return;
    }

<<<<<<< HEAD
    int data_len = ntohl(*(int*)len_buff);
    if (data_len <= 0 || data_len > 4096)
    {
=======
    // 转成主机字节序
    int data_len = ntohl(*(int *)len_buff);
    if (data_len <= 0 || data_len > 4096)
    { // 限制最大长度，防止攻击
>>>>>>> 997ecac1d62ed16b323db5dc5da067eade2cba44
        cout << "非法数据长度" << endl;
        Send_err();
        return;
    }

<<<<<<< HEAD
    char data_buff[4096] = {0};
    n = recv(c, data_buff, data_len, MSG_WAITALL);
=======
    // 2.接收JSON数据
    char data_buff[4096] = {0};
    n = recv(c, data_buff, data_len, MSG_WAITALL); // 不收满指定字节数(4)，绝不返回
>>>>>>> 997ecac1d62ed16b323db5dc5da067eade2cba44
    if (n != data_len)
    {
        cout << "接收数据失败" << endl;
        Send_err();
        return;
    }

    cout << "recv=" << data_buff << endl;

<<<<<<< HEAD
=======
    // 解析(反序列化)
    Json::Value val;
>>>>>>> 997ecac1d62ed16b323db5dc5da067eade2cba44
    Json::Reader Read;
    if (!Read.parse(data_buff, data_buff + data_len, val))
    {
        cout << "Recv_data:解析json失败！" << endl;
        Send_err();
        return;
    }

<<<<<<< HEAD
    int ops = val["type"].asInt();

    switch (ops)
    {
    case (int)OP_TYPE::LOGIN:
        User_Login();
        break;
    case (int)OP_TYPE::REGISTER:
        User_Register();
        break;
    case (int)OP_TYPE::VIEW_AVAILABLE_BOOKING:
        User_Show_Ticket();
        break;
    case (int)OP_TYPE::BOOK_APPOINTMENT:
        User_Book_Ticket();
        break;
    case (int)OP_TYPE::VIEW_MY_BOOKING:
        User_Show_My_Ticlet();
        break;
    case (int)OP_TYPE::CANCEL_MY_BOOKING:
        User_Cancel_Ticket();
        break;
    case (int)OP_TYPE::LOGOUT:
        break;
    default:
        cout << "输入无效！" << endl;
        break;
    }
=======
    socket_con *self = this;
    self->AddRef(); // 防止线程执行前对象被销毁

    // 抛给线程池执行***
    g_pool.add_task([self, val]()
                    {
                        self->val = val;
                        // 拿出操作类型，调用相关的函数
                        int ops = self->val["type"].asInt();

                        switch (ops)
                        {
                        case (int)OP_TYPE::LOGIN:
                            self->User_Login();
                            break;
                        case (int)OP_TYPE::REGISTER:
                            self->User_Register();
                            break;
                        case (int)OP_TYPE::VIEW_AVAILABLE_BOOKING:
                            self->User_Show_Ticket();
                            break;
                        case (int)OP_TYPE::BOOK_APPOINTMENT:
                            self->User_Book_Ticket();
                            break;
                        case (int)OP_TYPE::VIEW_MY_BOOKING:
                            self->User_Show_My_Ticlet();
                            break;
                        case (int)OP_TYPE::CANCEL_MY_BOOKING:
                            self->User_Cancel_Ticket();
                            break;
                        case (int)OP_TYPE::LOGOUT:
                            ops = false;
                            break;

                        default:
                            cout << "输入无效！" << endl;
                            break;
                        }
                        self->ReleaseRef(); // 减引用
                    });
>>>>>>> 997ecac1d62ed16b323db5dc5da067eade2cba44
}

void SOCK_CON_CALLBACK(int sockfd, short ev, void* arg)
{
    socket_con* q = static_cast<socket_con*>(arg);
    if (NULL == q)
    {
        return;
    }

    if (ev & EV_READ)
    {
        q->Recv_data();
    }
}

void SOCK_LIS_CALLBACK(int sockfd, short ev, void* arg)
{
    socket_listen* p = static_cast<socket_listen*>(arg);
    if (NULL == p)
    {
        return;
    }

    if (ev & EV_READ)
    {
        int c = p->accept_client();
        if (c == -1)
        {
            return;
        }
        cout << "accept:c=" << c << endl;
        if (!p->add_client(c))
        {
            cout << "客户端添加失败！" << c << endl;
        }
    }
}

int main()
{
<<<<<<< HEAD
    if (!MySQLPool::instance().init("127.0.0.1", "root", "123456",
                                      "Online_Res_DB", 3306, 10))
=======
    if (!MySQLPool::instance().init("127.0.0.1", "root", "123456", "Online_Res_DB", 3306, 10))
>>>>>>> 997ecac1d62ed16b323db5dc5da067eade2cba44
    {
        cout << "Mysql 连接池初始化失败！" << endl;
        exit(1);
    }
<<<<<<< HEAD

=======
    // 初始化 Redis 连接池
>>>>>>> 997ecac1d62ed16b323db5dc5da067eade2cba44
    if (!RedisPool::instance().init("127.0.0.1", 6379, 10))
    {
        cout << "Redis 连接池初始化失败！" << endl;
        exit(1);
    }
<<<<<<< HEAD

=======
    // 监听套接字
>>>>>>> 997ecac1d62ed16b323db5dc5da067eade2cba44
    socket_listen sock_ser;
    if (!sock_ser.socket_init())
    {
        cout << "socket_err!" << endl;
    }

    sock_ser.Get_sockfd();

    struct event_base* base = event_init();
    if (base == NULL)
    {
        cout << "base NULL" << endl;
        exit(1);
    }

    sock_ser.Set_base(base);

    struct event* sock_ev = event_new(base, sock_ser.Get_sockfd(),
                                      EV_READ | EV_PERSIST,
                                      SOCK_LIS_CALLBACK, &sock_ser);
    event_add(sock_ev, NULL);

    event_base_dispatch(base);

    event_free(sock_ev);
    event_base_free(base);
}
