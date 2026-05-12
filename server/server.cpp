#include "server.h"
void SOCK_CON_CALLBACK(int sockfd, short ev, void *arg); // 函数声明

// mysql_client
bool mysql_client::mysql_con_ser()
{
    MYSQL *mysql = mysql_init(&mysql_con);
    if (mysql == NULL)
    {
        return false;
    }
    mysql = mysql_real_connect(mysql, db_host.c_str(), db_user.c_str(), db_passwd.c_str(), db_name.c_str(), port, NULL, 0);
    if (mysql == NULL)
    {
        cout << "connect db err" << endl;
        return false;
    }
    return true;
}
bool mysql_client::mysql_Register(const string &username, const string &passwd, const string &realname, const string &id_card, const string &usertel, GENDER gen)
{
    string sql = string("insert into `user_info` values(0,'") + username + string("','") + passwd + string("','") + realname + string("','") + id_card + string("','") + usertel + string("',") + to_string(static_cast<int>(gen)) + string(",NOW(),NOW(),1);");
    if (mysql_query(&mysql_con, sql.c_str()) != 0)
    {
        cout << "注册失败！数据库错误！" << endl;
        cout << "数据库错误原因：" << mysql_error(&mysql_con) << endl;
        cout << "执行的SQL语句：" << sql << endl;
        return false;
    }
    return true;
}

bool mysql_client::mysql_Login(const string &usertel, const string &passwd, string &username)
{
    // 先看手机号码到底存不存在
    // select username,passwd from user_info where phone=17789023478;
    string sql = string("select username,passwd from user_info where phone='") + usertel + string("'");
    // mysql_query执行sql语句，成功返回0
    if (mysql_query(&mysql_con, sql.c_str()) != 0)
    {
        cout << "登录失败！" << endl;
        return false;
    }
    MYSQL_RES *r = mysql_store_result(&mysql_con); // 获取结果集
    if (r == NULL)
    {
        cout << "提取结果集失败！" << endl;
        return false;
    }
    int num = mysql_num_rows(r); // 看结果集有多少行，0就是结果为空，用户没有注册
    if (num == 0)
    {
        mysql_free_result(r);
        return false;
    }
    MYSQL_ROW row = mysql_fetch_row(r); // 取出结果集中的一行记录

    string password = row[1];
    if (password.compare(passwd) != 0)
    {
        return false;
    }
    username = row[0];
    mysql_free_result(r);

    return true;
}
bool mysql_client::mysql_User_Show_Ticket(Json::Value &resval)
{
    // select ticket_id,ticket_name,total_stock,use_date, booked_count from ticket_info;
    string sql = "select ticket_id,ticket_name,price,total_stock,max_per_user,use_date,booked_count from ticket_info;";
    if (mysql_query(&mysql_con, sql.c_str()) != 0)
    {
        cout << "sql语句执行失败！" << mysql_error(&mysql_con) << endl;
        return false;
    }
    MYSQL_RES *r = mysql_store_result(&mysql_con);
    if (r == NULL)
    {
        cout << "提取结果集失败！" << mysql_error(&mysql_con) << endl;
        return false;
    }

    int n = mysql_num_rows(r);
    if (n == 0)
    {
        resval["status"] = "OK";
        resval["num"] = 0;
        return true;
    }
    resval["status"] = "OK";
    resval["num"] = n;

    // 将信息填入到json数组中
    for (int i = 0; i < n; i++)
    {
        MYSQL_ROW row = mysql_fetch_row(r);
        Json::Value tmp;
        // 数字类型必须用 atoi / atof 转成 int/double
        tmp["ticket_id"] = atoi(row[0]);    // ticket_id
        tmp["ticket_name"] = row[1];        // 门票名称
        tmp["price"] = atof(row[2]);        // 价格（转浮点）
        tmp["total_stock"] = atoi(row[3]);  // 总库存
        tmp["max_per_user"] = atoi(row[4]); // 单人最大购买
        tmp["use_date"] = row[5];           // 使用日期
        tmp["booked_count"] = atoi(row[6]); // 已预约数量
        resval["ticket_arr"].append(tmp);
        /*
        tmp = JSON 对象（{}）
        ticket_arr = JSON 数组（[]）
        append = 把对象放进数组里
        */
    }

    return true;
}
bool mysql_client::mysql_user_begin()
{
    if (mysql_query(&mysql_con, "begin") != 0)
    {
        return false;
    }
    return true;
}
bool mysql_client::mysql_user_commit()
{
    if (mysql_query(&mysql_con, "commit") != 0)
    {
        return false;
    }
    return true;
}
bool mysql_client::mysql_user_rollback()
{
    if (mysql_query(&mysql_con, "rollback") != 0)
    {
        return false;
    }
    return true;
}
bool mysql_client::mysql_User_Book_Ticket(int ticket_id, string usertel, int num)
{
    mysql_user_begin(); // 启动事务
    string s1 = string("select total_stock,booked_count,use_date,max_per_user from ticket_info where ticket_id=") + to_string(ticket_id);
    if (mysql_query(&mysql_con, s1.c_str()) != 0)
    {
        cout << "查询失败！" << endl;
        mysql_user_rollback();
        return false;
    }
    MYSQL_RES *r = mysql_store_result(&mysql_con);
    if (r == NULL)
    {
        cout << "提取结果集失败！" << endl;
        mysql_user_rollback();
        return false;
    }
    int Num = mysql_num_rows(r);
    if (Num != 1)
    {
        cout << "记录行不唯一！" << endl;
        mysql_user_rollback();
        return false;
    }
    MYSQL_ROW row = mysql_fetch_row(r);
    string str_total_stock = row[0];  // 总票数
    string str_booked_count = row[1]; // 当前定出去的票
    string str_use_date = row[2];     // 使用时间
    string str_max_per_user = row[3]; // 最大购买量
    int tk_total_stock = atoi(str_total_stock.c_str());
    int tk_booked_count = atoi(str_booked_count.c_str());
    int tk_max_per_user = atoi(str_max_per_user.c_str());

    mysql_free_result(r);

    if (tk_total_stock <= tk_booked_count)
    {
        cout << "票已经售罄" << endl;
        mysql_user_rollback();
        return false;
    }
    /*SELECT IFNULL(SUM(book_num), 0) AS has_booked FROM user_order WHERE user_tel = 18902389403 AND ticket_id = 1 AND status = 1;*/
    string s2 = string("SELECT IFNULL(SUM(book_num), 0) AS has_booked FROM user_order WHERE user_tel = '") + usertel + string("' AND ticket_id = ") + to_string(ticket_id) + string(" AND status = 1;");
    if (mysql_query(&mysql_con, s2.c_str()) != 0)
    {
        cout << "查询订票记录失败！" << endl;
        mysql_user_rollback();
        return false;
    }
    MYSQL_RES *r2 = mysql_store_result(&mysql_con);
    if (r2 == NULL)
    {
        cout << "提取结果集失败！" << endl;
        mysql_user_rollback();
        return false;
    }
    int Num2 = mysql_num_rows(r2);
    if (Num2 != 1)
    {
        cout << "记录行不唯一！" << endl;
        mysql_user_rollback();
        return false;
    }
    MYSQL_ROW row2 = mysql_fetch_row(r2);
    string booknum = row2[0];
    int booknum2 = atoi(booknum.c_str());

    mysql_free_result(r2);

    if (num + booknum2 > tk_max_per_user)
    {
        cout << "单人购买量已经超过上限" << endl;
        mysql_user_rollback();
        return false;
    }
    tk_booked_count += num;
    string s3 = string("update ticket_info set booked_count=") + to_string(tk_booked_count) + string(" where ticket_id=") + to_string(ticket_id) + string(";");
    if (mysql_query(&mysql_con, s3.c_str()) != 0)
    {
        cout << "修改预定票数失败！" << endl;
        mysql_user_rollback();
        return false;
    }
    // 执行sql,尝试更新原有订单
    // UPDATE user_order SET book_num = book_num + 1 WHERE user_tel = 18902389403 AND ticket_id = 1 AND status = 1;
    string s4 = string("UPDATE user_order SET book_num = book_num + ") + to_string(num) + string(" WHERE user_tel ='") + usertel + string("' AND ticket_id =") + to_string(ticket_id) + string(" AND status = 1;");
    if (mysql_query(&mysql_con, s4.c_str()) != 0)
    {
        cout << "尝试更新原有订单失败！" << endl;
        mysql_user_rollback();
        return false;
    }
    // 判断有没有在原数据上更新，1更新了，0没有更新（要新增）
    int affected_rows = mysql_affected_rows(&mysql_con);
    if (affected_rows == 0)
    {
        // 拼接插入SQL
        string s5 = "INSERT INTO user_order(user_tel, ticket_id, book_num, use_date) "
                    "VALUES('" +
                    usertel + "', " + to_string(ticket_id) + ", " + to_string(num) + ", '" + str_use_date + "');";

        if (mysql_query(&mysql_con, s5.c_str()) != 0)
        {
            cout << "新增订单失败！" << endl;
            mysql_user_rollback();
            return false;
        }
    }
    mysql_user_commit();
    return true;
}
bool mysql_client::mysql_User_Show_My_Ticlet(Json::Value &resval, string usertel)
{
    // select *from user_order;
    string sql = string("select *from user_order where user_tel='") + usertel + string("';");
    if (mysql_query(&mysql_con, sql.c_str()) != 0)
    {
        cout << "sql语句执行失败！" << mysql_error(&mysql_con) << endl;
        return false;
    }
    MYSQL_RES *r = mysql_store_result(&mysql_con);
    if (r == NULL)
    {
        cout << "提取结果集失败！" << mysql_error(&mysql_con) << endl;
        return false;
    }

    int n = mysql_num_rows(r);
    if (n == 0)
    {
        resval["status"] = "OK";
        resval["num"] = 0;
        return true;
    }
    resval["status"] = "OK";
    resval["num"] = n;

    // 将信息填入到json数组中
    for (int i = 0; i < n; i++)
    {
        MYSQL_ROW row = mysql_fetch_row(r);
        Json::Value tmp;

        tmp["order_id"] = atoi(row[0]);  // order_id (预约ID)
        tmp["user_tel"] = row[1];        // 用户手机号
        tmp["ticket_id"] = atoi(row[2]); // 门票 ID
        tmp["book_num"] = atoi(row[3]);  // 预约数量
        tmp["use_date"] = row[4];        // 使用时间
        tmp["create_time"] = row[5];     // 订单创建时间
        tmp["status"] = atoi(row[6]);    // 状态(1未核销 0已核销)

        resval["ticket_arr"].append(tmp);
    }

    return true;
}
bool mysql_client::mysql_User_Cancel_Ticket(int ticket_id, string usertel, int num)
{
    mysql_user_begin(); // 启动事务
    string s1 = string("select book_num from user_order where user_tel='") + usertel + string("' AND ticket_id = ")+to_string(ticket_id)+string(" AND status = 1;");
    if (mysql_query(&mysql_con, s1.c_str()) != 0)
    {
        cout << "查询失败！" << endl;
        mysql_user_rollback();
        return false;
    }
    MYSQL_RES *r1 = mysql_store_result(&mysql_con);
    if (r1 == NULL)
    {
        cout << "提取结果集失败！" << mysql_error(&mysql_con) << endl;
        return false;
    }
    int n1 = mysql_num_rows(r1);
    if (n1 == 0)
    {
        cout << "查不到此人的预定信息" << endl;
        mysql_user_rollback();
        return false;
    }
    MYSQL_ROW row1 = mysql_fetch_row(r1);

    int book_num = atoi(row1[0]);

    mysql_free_result(r1); // 释放
    // 如果取消的数量大于预约的数量，失败
    if (book_num < num)
    {
        cout << "取消票数过多！" << endl;
        mysql_user_rollback();
        return false;
    }
    else if (book_num == num)
    {
        string s2=string("DELETE FROM user_order WHERE user_tel = '")+usertel+string("' AND ticket_id =")+to_string(ticket_id)+string(" AND status = 1;");
        if (mysql_query(&mysql_con, s2.c_str()) != 0)
        {
            cout << "取消预定票数失败！" << endl;
            mysql_user_rollback();
            return false;
        }
    }
    else
    {
        book_num -= num;
        // user_order
        string s3 = string("UPDATE user_order SET book_num = ") + to_string(book_num) + string(" WHERE user_tel = '") + usertel + string("' AND ticket_id = ")+to_string(ticket_id)+string(" AND status = 1;");
        if (mysql_query(&mysql_con, s3.c_str()) != 0)
        {
            cout << "修改预定票数失败！" << endl;
            mysql_user_rollback();
            return false;
        }
    }

    // ticket_info
    string s4 = string("select booked_count from ticket_info where ticket_id=") + to_string(ticket_id) + string(";");
    if (mysql_query(&mysql_con, s4.c_str()) != 0)
    {
        cout << "查询失败！" << endl;
        mysql_user_rollback();
        return false;
    }
    MYSQL_RES *r2 = mysql_store_result(&mysql_con);
    if (r2 == NULL)
    {
        cout << "提取结果集失败！" << mysql_error(&mysql_con) << endl;
        return false;
    }
    int n2 = mysql_num_rows(r2);
    if (n2 == 0)
    {
        cout << "查不到此门票的信息" << endl;
        mysql_user_rollback();
        return false;
    }
    MYSQL_ROW row2 = mysql_fetch_row(r2);

    int bookedcount = atoi(row2[0]);
    mysql_free_result(r2); // 释放
    bookedcount -= num;
    // UPDATE ticket_info SET booked_count = 1 WHERE ticket_id = 2;
    string s5 = string("UPDATE ticket_info SET booked_count =") + to_string(bookedcount) + string(" WHERE ticket_id =") + to_string(ticket_id) + string(";");
    if (mysql_query(&mysql_con, s5.c_str()) != 0)
    {
        cout << "修改全部预定票数失败！" << endl;
        mysql_user_rollback();
        return false;
    }
    mysql_user_commit();
    return true;
}
//---socket_lis
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

    int res = bind(sockfd, (struct sockaddr *)&saddr, sizeof(saddr));
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
    int c = accept(sockfd, NULL, NULL); // c客户端套接字的文件描述符
    return c;
}
bool socket_listen::add_client(int c)
{
    socket_con *q = new socket_con(c);
    struct event *c_ev = event_new(this->base, c, EV_READ | EV_PERSIST, SOCK_CON_CALLBACK, q); // new一个事件
    if (c_ev == NULL)
    {
        close(c);
        delete q;
        return false;
    }
    q->Set_ev(c_ev);
    // 添加到libevent中，做事件驱动（检测到事件发生了，调用回调函数）
    event_add(c_ev, NULL);
    return true;
}

//---socket_con

void socket_con::Send_err()
{
    Json::Value res_val;
    res_val["status"] = "ERR";

    writer.omitEndingLineFeed();
    string send_str = writer.write(res_val);
    send(c, send_str.c_str(), send_str.size(), 0);
}
void socket_con::Send_ok()
{
    Json::Value res_val;
    res_val["status"] = "OK";

    writer.omitEndingLineFeed();
    string send_str = writer.write(res_val);
    send(c, send_str.c_str(), send_str.size(), 0);
}
void socket_con::User_Register()
{
    username = val["user_name"].asString();
    usertel = val["user_tel"].asString();
    passwd = val["user_passwd"].asString();
    realname = val["real_name"].asString();
    gen = static_cast<GENDER>(val["gender"].asInt());
    id_card = val["id_card"].asString();

    if (usertel.empty() || username.empty() || passwd.empty() || realname.empty() || static_cast<int>(gen) < 0 || static_cast<int>(gen) > 2 || id_card.empty())
    {
        Send_err();
        return;
    }

    mysql_client cli;
    if (!cli.mysql_con_ser())
    {
        Send_err();
        return;
    }
    if (!cli.mysql_Register(username, passwd, realname, id_card, usertel, gen))
    {
        Send_err();
        return;
    }
    Send_ok();
    return;
}
void socket_con::User_Login()
{
    usertel = val["user_tel"].asString();
    passwd = val["user_passwd"].asString();

    mysql_client cli;
    if (!cli.mysql_con_ser())
    {
        Send_err();
        return;
    }
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
    send(c, send_str.c_str(), send_str.size(), 0);
    return;
}
void socket_con::User_Show_Ticket()
{
    Json::Value resval;
    // 每次调用数据库函数之前先去连接数据库

    mysql_client cli;
    if (!cli.mysql_con_ser())
    {
        Send_err();
        return;
    }
    if (!cli.mysql_User_Show_Ticket(resval))
    {
        Send_err();
        return;
    }

    Json::FastWriter writer;
    writer.omitEndingLineFeed();
    send(c, writer.write(resval).c_str(), writer.write(resval).size(), 0);
    return;
}

void socket_con::User_Book_Ticket()
{
    // 去客户端获取 ticket_id , user_tel
    int ticket_id = val["ticket_id"].asInt();
    usertel = val["user_tel"].asString();
    int num = val["book_num"].asInt();

    mysql_client cli;
    if (!cli.mysql_con_ser())
    {
        cout << "connect con err!" << endl;
        Send_err();
        return;
    }
    if (!cli.mysql_User_Book_Ticket(ticket_id, usertel, num))
    {
        Send_err();
        return;
    }
    Send_ok();
    return;
}
void socket_con::User_Show_My_Ticlet()
{
    usertel = val["user_tel"].asString();

    mysql_client cli;
    if (!cli.mysql_con_ser())
    {
        Send_err();
        return;
    }
    if (!cli.mysql_User_Show_My_Ticlet(val, usertel))
    {
        Send_err();
        return;
    }

    val["status"] = "OK";
    Json::FastWriter writer;
    writer.omitEndingLineFeed();
    send(c, writer.write(val).c_str(), writer.write(val).size(), 0);
    return;
}
void socket_con::User_Cancel_Ticket()
{
    usertel = val["user_tel"].asString();
    int ticketid = val["ticket_id"].asInt();
    int num = val["book_num"].asInt();

    mysql_client cli;
    if (!cli.mysql_con_ser())
    {
        Send_err();
        return;
    }
    if (!cli.mysql_User_Cancel_Ticket(ticketid, usertel, num))
    {
        Send_err();
        return;
    }
    Send_ok();
    return;
}
void socket_con::Recv_data() // 第一步：收到客户端发来的数据
{
    //1.接收长度
    char len_buff[4]{0};
    int n = recv(c, len_buff, 4, MSG_WAITALL); // recv收到数据(报文)
    if (4!=n)
    {
        cout << "client close" << endl;
        delete this;
        return;
    }

    //转成主机字节序
    int data_len=ntohl(*(int*)len_buff);
    if (data_len <= 0 || data_len > 4096) { // 限制最大长度，防止攻击
        cout << "非法数据长度" << endl;
        Send_err();
        return;
    }

    //2.接收JSON数据
    char data_buff[4096] = {0};
    n = recv(c, data_buff, data_len, MSG_WAITALL);//不收满指定字节数(4)，绝不返回
    if (n != data_len) {
        cout << "接收数据失败" << endl;
        Send_err();
        return;
    }

    // 测试
    cout << "recv=" << data_buff << endl;

    // 解析(反序列化)
    Json::Reader Read;
    if (!Read.parse(data_buff, data_buff + data_len,val))
    {
        cout << "Recv_data:解析json失败！" << endl;
        Send_err();
        return;
    }

    // 拿出操作类型，调用相关的函数
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
        ops = false;
        break;

    default:
        cout << "输入无效！" << endl;
        break;
    }
}

//---callback
void SOCK_CON_CALLBACK(int sockfd, short ev, void *arg)
{
    socket_con *q = static_cast<socket_con *>(arg);
    if (NULL == q)
    {
        return;
    }
    // 处理读事件
    if (ev & EV_READ)
    {
        // receive
        q->Recv_data();
    }
}
void SOCK_LIS_CALLBACK(int sockfd, short ev, void *arg)
{
    socket_listen *p = static_cast<socket_listen *>(arg);
    if (NULL == p)
    {
        return;
    }
    // 处理读事件
    if (ev & EV_READ)
    {
        int c = p->accept_client(); // 通过类完成对套接字的所有处理
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
    // 监听套接字
    socket_listen sock_ser;
    if (!sock_ser.socket_init())
    {
        cout << "socket_err!" << endl;
    }

    sock_ser.Get_sockfd();

    // 启动libevent,创建base
    struct event_base *base = event_init();
    if (base == NULL)
    {
        cout << "base NULL" << endl;
        exit(1);
    }

    // 设置socket_listen中的libevent的base
    sock_ser.Set_base(base);

    // 添加事件sockfd到libevent
    struct event *sock_ev = event_new(base, sock_ser.Get_sockfd(), EV_READ | EV_PERSIST, SOCK_LIS_CALLBACK, &sock_ser); // 先定义事件
    event_add(sock_ev, NULL);

    // 启动事件循环
    event_base_dispatch(base);
    // 释放资源
    event_free(sock_ev);
    event_base_free(base);

    // 初始化 Redis 连接池
    RedisPool::instance().init("127.0.0.1", 6379, 10);
}
