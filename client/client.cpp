#include "client.h"

bool socket_client::send_json_msg(int sockfd, const Json::Value& val)
{
    Json::FastWriter writer;
    writer.omitEndingLineFeed();
    string send_str = writer.write(val);

    int len = send_str.size();
    if (len <= 0 || len > 4096 - 4)
    {
        return false;
    }

    char send_buff[4096];
    *(int*)send_buff = htonl(len);
    memcpy(send_buff + 4, send_str.c_str(), len);

    send(sockfd, send_buff, len + 4, 0);
    return true;
}

bool socket_client::connect_server()
{
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1)
    {
        cout << "create socket err" << endl;
        return false;
    }

    struct sockaddr_in saddr;
    memset(&saddr, 0, sizeof(saddr));
    saddr.sin_family = AF_INET;
    saddr.sin_port = htons(6789);
    saddr.sin_addr.s_addr = inet_addr("127.0.0.1");

    int res = connect(sockfd, reinterpret_cast<struct sockaddr*>(&saddr), sizeof(saddr));
    if (res == -1)
    {
        cout << "connect err" << endl;
        close(sockfd);
        return false;
    }
    cout << "connect to server success!" << endl;
    return true;
}

void socket_client::print_info()
{
    if (dl_flg)
    {
        cout << "------已登录-----用户名：" << username << "------" << endl;
        cout << "1.查看预约 2.预订 3.查看我的预约 4.取消预约 5.退出" << endl;
        cout << "------------------" << endl;
        cin >> user_op;
        user_op += OFFSET;
    }
    else
    {
        cout << "|-----未登录----游客-------|" << endl;
        cout << "|--1.登录 2.注册 3.退出----|" << endl;
        cout << "|--------------------------|" << endl;
        cout << "|---请输入您的选项编号-----|" << endl;
        cin >> user_op;
        if (user_op == 3)
        {
            user_op = static_cast<int>(OP_TYPE::LOGOUT);
        }
    }
}

void socket_client::User_Register()
{
    cout << "//--------注册---------//" << endl;
    cout << "请输入手机号码:" << endl;
    cin >> usertel;

    cout << "请输入用户名:" << endl;
    cin >> username;

    string tmp;
    cout << "请输入密码:" << endl;
    cin >> passwd;

    if (passwd.empty())
    {
        cout << "密码不能为空!" << endl;
        return;
    }

    cout << "请再次输入密码:" << endl;
    cin >> tmp;
    if (passwd.compare(tmp))
    {
        cout << "密码不一致!" << endl;
        return;
    }

    cout << "请输入真实姓名:" << endl;
    cin >> realname;

    cout << "请输入身份证号:" << endl;
    cin >> id_card;

    int gen_input;
    cout << "请输入性别(0未知/1男/2女):" << endl;
    cin >> gen_input;
    gen = static_cast<GENDER>(gen_input);

    Json::Value val;
    val["type"] = static_cast<int>(OP_TYPE::REGISTER);
    val["user_tel"] = usertel;
    val["user_name"] = username;
    val["user_passwd"] = passwd;
    val["real_name"] = realname;
    val["gender"] = static_cast<int>(gen);
    val["id_card"] = id_card;

    send_json_msg(sockfd, val);

    char buff[256] = {0};
    if (recv(sockfd, buff, 255, 0) <= 0)
    {
        cout << "ser close!" << endl;
        close(sockfd);
        sockfd = -1;
        return;
    }

    val.clear();
    Json::Reader Read;
    if (!Read.parse(buff, val))
    {
        cout << "Json解析失败！" << endl;
        return;
    }

    string s = val["status"].asString();
    if (s.compare("OK") != 0)
    {
        cout << "注册失败！" << endl;
        return;
    }
    dl_flg = true;
    cout << "注册成功！" << endl;
}

void socket_client::User_Login()
{
    cout << "请输入手机号码：" << endl;
    cin >> usertel;
    cout << "请输入密码：" << endl;
    cin >> passwd;

    Json::Value val;
    val["type"] = static_cast<int>(OP_TYPE::LOGIN);
    val["user_tel"] = usertel;
    val["user_passwd"] = passwd;

    send_json_msg(sockfd, val);

    char buff[1024] = {0};
    if (recv(sockfd, buff, 1023, 0) <= 0)
    {
        cout << "ser close!" << endl;
        close(sockfd);
        sockfd = -1;
        return;
    }

    val.clear();
    Json::Reader Read;
    if (!Read.parse(buff, val))
    {
        cout << "解析Json失败！" << endl;
        return;
    }

    string st = val["status"].asString();
    username = val["user_name"].asString();
    if (st.compare("OK") != 0)
    {
        cout << "登录失败！" << endl;
        return;
    }
    dl_flg = true;
    cout << "登录成功！" << endl;
}

void socket_client::User_Show_Ticket()
{
    Json::Value val;
    val["type"] = static_cast<int>(OP_TYPE::VIEW_AVAILABLE_BOOKING);
    send_json_msg(sockfd, val);

    char buff[4096] = {0};
    int n = recv(sockfd, buff, 4095, 0);
    if (n <= 0)
    {
        cout << "ser close!" << endl;
        close(sockfd);
        sockfd = -1;
        return;
    }

    m_val.clear();
    Json::Reader Read;
    if (!Read.parse(buff, m_val))
    {
        cout << "解析Json失败！" << endl;
        return;
    }

    string st = m_val["status"].asString();
    if (st.compare("OK") != 0)
    {
        cout << "查询预约信息失败！" << endl;
        return;
    }

    int num = m_val["num"].asInt();
    if (num == 0)
    {
        cout << "没有可预约的信息！" << endl;
        return;
    }

    cout << "============================================================================================" << endl;
    cout << left
         << setw(18) << "门票编号"
         << setw(20) << "门票名称"
         << setw(14) << "价格"
         << setw(15) << "总库存"
         << setw(18) << "最大预约数"
         << setw(25) << "可使用日期"
         << setw(10) << "已预约" << endl;
    cout << "============================================================================================" << endl;

    for (int i = 0; i < num; i++)
    {
        cout << left
             << setw(10) << m_val["ticket_arr"][i]["ticket_id"].asString()
             << setw(30) << m_val["ticket_arr"][i]["ticket_name"].asString()
             << setw(10) << m_val["ticket_arr"][i]["price"].asString()
             << setw(13) << m_val["ticket_arr"][i]["total_stock"].asString()
             << setw(10) << m_val["ticket_arr"][i]["max_per_user"].asString()
             << setw(25) << m_val["ticket_arr"][i]["use_date"].asString()
             << setw(10) << m_val["ticket_arr"][i]["booked_count"].asString() << endl;
    }
}

void socket_client::User_Book_Ticket()
{
    User_Show_Ticket();
    cout << "请输入要预定的编号：" << endl;
    int ticketid = 0;
    cin >> ticketid;

    cout << "请输入要预定的数量：" << endl;
    int num = 0;
    cin >> num;

    Json::Value val;
    val["type"] = static_cast<int>(OP_TYPE::BOOK_APPOINTMENT);
    val["user_tel"] = usertel;
    val["ticket_id"] = ticketid;
    val["book_num"] = num;

    send_json_msg(sockfd, val);

    char buff[256] = {0};
    int n = recv(sockfd, buff, 255, 0);
    if (n <= 0)
    {
        cout << "ser close!" << endl;
        return;
    }

    val.clear();
    Json::Reader Read;
    if (!Read.parse(buff, val))
    {
        cout << "json 解析失败！" << endl;
        return;
    }

    string st = val["status"].asString();
    if (st.compare("OK") != 0)
    {
        cout << "预定失败！" << endl;
        return;
    }
    cout << "预订成功！" << endl;
}

void socket_client::User_Show_My_Ticlet()
{
    Json::Value val;
    val["type"] = static_cast<int>(OP_TYPE::VIEW_MY_BOOKING);
    val["user_tel"] = usertel;

    send_json_msg(sockfd, val);

    char buff[1024] = {0};
    int n = recv(sockfd, buff, 1023, 0);
    if (n <= 0)
    {
        cout << "ser close" << endl;
        close(sockfd);
        sockfd = -1;
        return;
    }

    val.clear();
    Json::Reader Read;
    if (!Read.parse(buff, val))
    {
        cout << "json解析失败" << endl;
        return;
    }

    string st = val["status"].asString();
    if (st.compare("OK") != 0)
    {
        cout << "查询我的预约失败！" << endl;
        return;
    }

    int num = val["num"].asInt();
    if (num == 0)
    {
        cout << "没有预约记录！" << endl;
        return;
    }

    cout << "============================================================================================" << endl;
    cout << left
         << setw(10) << "预约ID"
         << setw(20) << "用户手机号"
         << setw(14) << "门票 ID"
         << setw(18) << "预约数量"
         << setw(18) << "使用时间"
         << setw(25) << "订单创建时间"
         << setw(10) << "状态(1未核销0已核销)" << endl;
    cout << "============================================================================================" << endl;

    for (int i = 0; i < num; i++)
    {
        cout << left
             << setw(10) << val["ticket_arr"][i]["order_id"].asString()
             << setw(20) << val["ticket_arr"][i]["user_tel"].asString()
             << setw(10) << val["ticket_arr"][i]["ticket_id"].asString()
             << setw(10) << val["ticket_arr"][i]["book_num"].asString()
             << setw(15) << val["ticket_arr"][i]["use_date"].asString()
             << setw(24) << val["ticket_arr"][i]["create_time"].asString()
             << setw(10) << val["ticket_arr"][i]["status"].asString() << endl;
    }
}

void socket_client::User_Cancel_Ticket()
{
    User_Show_Ticket();
    cout << "请输入要取消的编号：" << endl;
    int ticketid = 0;
    cin >> ticketid;

    cout << "请输入要取消的数量：" << endl;
    int num = 0;
    cin >> num;

    Json::Value val;
    val["type"] = static_cast<int>(OP_TYPE::CANCEL_MY_BOOKING);
    val["user_tel"] = usertel;
    val["ticket_id"] = ticketid;
    val["book_num"] = num;

    send_json_msg(sockfd, val);

    char buff[256] = {0};
    int n = recv(sockfd, buff, 255, 0);
    if (n <= 0)
    {
        cout << "ser close!" << endl;
        return;
    }

    val.clear();
    Json::Reader Read;
    if (!Read.parse(buff, val))
    {
        cout << "json 解析失败！" << endl;
        return;
    }

    string st = val["status"].asString();
    if (st.compare("OK") != 0)
    {
        cout << "取消预定失败！" << endl;
        return;
    }
    cout << "取消预订成功！" << endl;
}

void socket_client::Run()
{
    while (runing)
    {
        print_info();

        switch (user_op)
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
            runing = false;
            break;
        default:
            cout << "输入无效！" << endl;
            break;
        }
    }
}

int main()
{
    socket_client cli;
    if (!cli.connect_server())
    {
        exit(1);
    }
    cli.Run();

    exit(0);
}
