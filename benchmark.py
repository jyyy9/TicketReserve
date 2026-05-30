#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
TCP 自定义协议压测工具 (长度头+JSON)
用于门票预约系统性能测试
"""

import asyncio
import json
import struct
import time
import sys
import random
import argparse
from collections import defaultdict
import statistics

# 服务器配置
HOST = '127.0.0.1'
PORT = 6789

# 操作类型（与服务端 OP_TYPE 对应）
OP_LOGIN = 1
OP_REGISTER = 2
OP_VIEW_TICKETS = 3
OP_BOOK = 4
OP_MY_BOOKINGS = 5
OP_CANCEL = 6

# 测试数据池（模拟不同用户）
TEST_USERS = [
    {"tel": f"138{str(i).zfill(8)}", "pwd": "123456", "name": f"user{i}", "realname": f"测试{i}",
     "id_card": f"11010119900307{str(i).zfill(4)}", "gender": 1}
    for i in range(1, 2001)  # 准备2000个测试账号
]

class BenchClient:
    def __init__(self, host, port, user_data=None):
        self.host = host
        self.port = port
        self.user = user_data or random.choice(TEST_USERS)
        self.reader = None
        self.writer = None
        self.latencies = []   # 存储每次请求的延迟(ms)

    async def connect(self):
        """建立 TCP 连接"""
        self.reader, self.writer = await asyncio.open_connection(self.host, self.port)

    async def send_request(self, req_json):
        """发送请求并等待完整响应，返回 (响应JSON, 耗时ms)"""
        start = time.perf_counter()
        # 1. 发送长度头 + JSON
        json_str = json.dumps(req_json, separators=(',', ':'))
        json_bytes = json_str.encode('utf-8')
        length = len(json_bytes)
        header = struct.pack('>I', length)      # 网络字节序（大端）
        self.writer.write(header + json_bytes)
        await self.writer.drain()

        # 2. 接收响应：先读4字节长度头
        len_data = await self.reader.readexactly(4)
        resp_len = struct.unpack('>I', len_data)[0]
        # 3. 读JSON体
        resp_data = await self.reader.readexactly(resp_len)
        elapsed_ms = (time.perf_counter() - start) * 1000
        resp_json = json.loads(resp_data.decode('utf-8'))
        return resp_json, elapsed_ms

    async def close(self):
        if self.writer:
            self.writer.close()
            await self.writer.wait_closed()

    # ---------- 业务操作 ----------
    async def register(self):
        req = {
            "type": OP_REGISTER,
            "user_name": self.user["name"],
            "user_tel": self.user["tel"],
            "user_passwd": self.user["pwd"],
            "real_name": self.user["realname"],
            "gender": self.user["gender"],
            "id_card": self.user["id_card"]
        }
        resp, delay = await self.send_request(req)
        success = (resp.get("status") == "OK")
        return success, delay

    async def login(self):
        req = {
            "type": OP_LOGIN,
            "user_tel": self.user["tel"],
            "user_passwd": self.user["pwd"]
        }
        resp, delay = await self.send_request(req)
        success = (resp.get("status") == "OK")
        return success, delay

    async def view_tickets(self):
        req = {"type": OP_VIEW_TICKETS}
        resp, delay = await self.send_request(req)
        # 响应中会包含多个门票信息，只要没有报错就算成功
        success = (resp.get("status") != "ERR")
        return success, delay

    async def book_ticket(self, ticket_id, num=1):
        req = {
            "type": OP_BOOK,
            "ticket_id": ticket_id,
            "user_tel": self.user["tel"],
            "book_num": num
        }
        resp, delay = await self.send_request(req)
        success = (resp.get("status") == "OK")
        return success, delay

    async def my_bookings(self):
        req = {
            "type": OP_MY_BOOKINGS,
            "user_tel": self.user["tel"]
        }
        resp, delay = await self.send_request(req)
        success = (resp.get("status") != "ERR")
        return success, delay

    async def cancel_ticket(self, ticket_id, num=1):
        req = {
            "type": OP_CANCEL,
            "ticket_id": ticket_id,
            "user_tel": self.user["tel"],
            "book_num": num
        }
        resp, delay = await self.send_request(req)
        success = (resp.get("status") == "OK")
        return success, delay


async def worker(client: BenchClient, ops_sequence, stats):
    """单个连接的工作协程：按顺序执行一系列操作"""
    for op_name, args in ops_sequence:
        try:
            if op_name == 'register':
                ok, delay = await client.register()
            elif op_name == 'login':
                ok, delay = await client.login()
            elif op_name == 'view':
                ok, delay = await client.view_tickets()
            elif op_name == 'book':
                ok, delay = await client.book_ticket(*args)
            elif op_name == 'my':
                ok, delay = await client.my_bookings()
            elif op_name == 'cancel':
                ok, delay = await client.cancel_ticket(*args)
            else:
                continue
            stats['total'] += 1
            if ok:
                stats['success'] += 1
            else:
                stats['fail'] += 1
            stats['latencies'].append(delay)
        except Exception as e:
            stats['fail'] += 1
            stats['total'] += 1

async def run_benchmark(concurrency, duration, ops_per_conn, mix_ratio, ticket_id=1001):
    stats = {
        'total': 0,
        'success': 0,
        'fail': 0,
        'latencies': []
    }

    # 为每个连接分配不同的用户
    clients = []
    for i in range(concurrency):
        user = TEST_USERS[i % len(TEST_USERS)]
        cli = BenchClient(HOST, PORT, user)
        await cli.connect()
        clients.append(cli)

    # 预注册/登录所有连接，确保用户可用
    print("正在为所有连接注册/登录...")
    async def ensure_login(cli):
        ok, _ = await cli.register()
        if not ok:
            ok, _ = await cli.login()
        return ok

    login_results = await asyncio.gather(*[ensure_login(cli) for cli in clients])
    success_cnt = sum(login_results)
    print(f"注册/登录成功: {success_cnt}/{concurrency}")
    if success_cnt == 0:
        print("所有连接登录失败，终止压测")
        return

    # 构造操作权重列表
    op_choices = []
    for op, ratio in mix_ratio.items():
        op_choices.extend([op] * int(ratio * 100))
    if not op_choices:
        op_choices = ['view']

    async def worker_for_client(cli):
        end_time = asyncio.get_running_loop().time() + duration
        while asyncio.get_running_loop().time() < end_time:
            op = random.choice(op_choices)
            try:
                if op == 'view':
                    ok, delay = await cli.view_tickets()
                elif op == 'book':
                    ok, delay = await cli.book_ticket(ticket_id, 1)
                elif op == 'login':
                    ok, delay = await cli.login()
                elif op == 'register':
                    ok, delay = await cli.register()
                else:
                    continue
                stats['total'] += 1
                if ok:
                    stats['success'] += 1
                else:
                    stats['fail'] += 1
                stats['latencies'].append(delay)
            except Exception as e:
                stats['fail'] += 1
                stats['total'] += 1

    tasks = [worker_for_client(cli) for cli in clients]
    await asyncio.gather(*tasks)

    # 关闭所有连接
    for cli in clients:
        await cli.close()

    total_req = stats['total']
    if total_req == 0:
        print("没有发出任何请求")
        return
    success_req = stats['success']
    fail_req = stats['fail']
    latencies = stats['latencies']
    qps = total_req / duration
    avg_lat = statistics.mean(latencies) if latencies else 0
    if latencies:
        latencies.sort()
        p95 = latencies[int(len(latencies)*0.95)]
        p99 = latencies[int(len(latencies)*0.99)]
    else:
        p95 = p99 = 0

    print(f"\n========== 压测结果 ==========")
    print(f"并发连接数: {concurrency}")
    print(f"总请求数: {total_req}")
    print(f"成功: {success_req} ({success_req/total_req*100:.2f}%)")
    print(f"失败: {fail_req}")
    print(f"QPS: {qps:.2f} req/s")
    print(f"平均延迟: {avg_lat:.2f} ms")
    print(f"P95延迟: {p95:.2f} ms")
    print(f"P99延迟: {p99:.2f} ms")
    return stats

async def test_concurrency_limit(max_conn=1500):
    """测试最大连接数：逐渐增加连接直到服务端拒绝或报错"""
    for num in [100, 500, 1000, 1200, 1500]:
        print(f"\n尝试建立 {num} 个连接...")
        clients = []
        ok = 0
        for i in range(num):
            try:
                cli = BenchClient(HOST, PORT, TEST_USERS[i%len(TEST_USERS)])
                await cli.connect()
                clients.append(cli)
                ok += 1
            except Exception as e:
                print(f"连接失败: {e}")
                break
        print(f"成功建立 {ok}/{num} 个长连接")
        # 发送一个简单的 view 请求验证连接可用
        if clients:
            try:
                _, delay = await clients[0].view_tickets()
                print(f"验证请求成功，延迟 {delay:.2f}ms")
            except:
                print("验证请求失败")
        # 关闭
        for c in clients:
            await c.close()
        if ok < num:
            break

async def race_condition_test(ticket_id, stock_before):
    """防超卖测试：并发预约同一门票"""
    concurrency = 500   # 500个并发同时抢
    clients = []
    for i in range(concurrency):
        user = TEST_USERS[i % len(TEST_USERS)]
        cli = BenchClient(HOST, PORT, user)
        await cli.connect()
        clients.append(cli)

    # 所有客户端同时 book
    async def book_one(cli):
        try:
            ok, delay = await cli.book_ticket(ticket_id, 1)
            return ok
        except:
            return False

    tasks = [book_one(cli) for cli in clients]
    results = await asyncio.gather(*tasks)
    success_cnt = sum(results)

    for cli in clients:
        await cli.close()

    print(f"\n=== 防超卖测试（库存 {stock_before}，并发 {concurrency}）===")
    print(f"成功预约数量: {success_cnt}")
    print(f"期望库存减少: {success_cnt}")
    print(f"实际剩余库存: 请手动查询数据库确认是否等于 {stock_before - success_cnt} 且 ≥0")
    print(f"如果剩余 {stock_before - success_cnt} 且无负数，则防超卖机制有效。")

if __name__ == '__main__':
    parser = argparse.ArgumentParser(description='门票预约系统 TCP 压测工具')
    parser.add_argument('--host', default='127.0.0.1', help='服务器IP')
    parser.add_argument('--port', type=int, default=6789, help='服务器端口')
    parser.add_argument('--concurrency', type=int, default=200, help='并发连接数')
    parser.add_argument('--duration', type=int, default=30, help='持续压测时间(秒)')
    parser.add_argument('--ops', type=int, default=0, help='每个连接固定请求数(优先级低于duration)')
    parser.add_argument('--mix', default='view:0.5,login:0.3,book:0.2', help='操作混合比例，如 view:0.5,login:0.3,book:0.2')
    parser.add_argument('--test-conn', action='store_true', help='测试最大连接数')
    parser.add_argument('--race', type=int, help='防超卖测试，指定门票ID')
    args = parser.parse_args()

    HOST = args.host
    PORT = args.port

    if args.test_conn:
        asyncio.run(test_concurrency_limit())
        sys.exit(0)

    if args.race:
        # 先查询数据库当前库存（手动设置一个初始值）
        print("请在数据库中设置门票 {} 的库存为某个值(如10)，然后按回车继续".format(args.race))
        input()
        asyncio.run(race_condition_test(args.race, stock_before=-1))
        sys.exit(0)

    # 解析混合比例
    mix = {}
    for item in args.mix.split(','):
        k,v = item.split(':')
        mix[k.strip()] = float(v)
    # 归一化
    total = sum(mix.values())
    mix = {k:v/total for k,v in mix.items()}

    if args.ops > 0:
        duration = 0
        ops_per_conn = args.ops
    else:
        duration = args.duration
        ops_per_conn = 0

    asyncio.run(run_benchmark(args.concurrency, duration, ops_per_conn, mix))