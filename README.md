# Crispy

## 性能测试结果

### 测试命令

```bash
docker-redis-benchmark -h 127.0.0.1 -p 12345 -t get,set -n 1000000 -r 10000000 -c 50 -P 50
docker-redis-benchmark -h 127.0.0.1 -p 6379  -t get,set -n 1000000 -r 10000000 -c 50 -P 50
```

说明:

- `12345`: Crispy
- `6379`: 官方 Redis
- Crispy 压测时出现 `WARNING: Could not fetch server CONFIG`，不影响核心吞吐和延迟统计
- 当前版本尚未实现 AOF（Append Only File）持久化

### 结果摘要

| 指标 | Crispy SET | Redis SET | Crispy GET | Redis GET |
|---|---:|---:|---:|---:|
| Throughput (req/s) | 1,692,047.38 | 660,938.50 | 2,666,666.75 | 1,408,450.75 |
| Avg Latency (ms) | 1.378 | 3.622 | 0.872 | 1.638 |
| P50 (ms) | 1.343 | 3.591 | 0.871 | 1.599 |
| P95 (ms) | 1.455 | 4.863 | 0.935 | 2.135 |
| P99 (ms) | 1.887 | 7.903 | 0.991 | 2.431 |
| Max (ms) | 12.143 | 19.375 | 1.175 | 17.231 |

### 对比结论

- SET 吞吐: Crispy 约为 Redis 的 `2.56x`
- GET 吞吐: Crispy 约为 Redis 的 `1.89x`
- 在平均延迟与 P95/P99 上，Crispy 当前测试数据优于官方 Redis

### 原始输出

<details>
<summary>Crispy (127.0.0.1:12345) 原始结果</summary>

```text
WARNING: Could not fetch server CONFIG
====== SET ======
  1000000 requests completed in 0.59 seconds
  50 parallel clients
  3 bytes payload
  keep alive: 1
  multi-thread: no

Summary:
  throughput summary: 1692047.38 requests per second
  latency summary (msec):
          avg       min       p50       p95       p99       max
        1.378     0.816     1.343     1.455     1.887    12.143

====== GET ======
  1000000 requests completed in 0.38 seconds
  50 parallel clients
  3 bytes payload
  keep alive: 1
  multi-thread: no

Summary:
  throughput summary: 2666666.75 requests per second
  latency summary (msec):
          avg       min       p50       p95       p99       max
        0.872     0.488     0.871     0.935     0.991     1.175
```

</details>

<details>
<summary>Redis (127.0.0.1:6379) 原始结果</summary>

```text
====== SET ======
  1000000 requests completed in 1.51 seconds
  50 parallel clients
  3 bytes payload
  keep alive: 1
  host configuration "save": 3600 1 300 100 60 10000
  host configuration "appendonly": yes
  multi-thread: no

Summary:
  throughput summary: 660938.50 requests per second
  latency summary (msec):
          avg       min       p50       p95       p99       max
        3.622     0.888     3.591     4.863     7.903    19.375

====== GET ======
  1000000 requests completed in 0.71 seconds
  50 parallel clients
  3 bytes payload
  keep alive: 1
  host configuration "save": 3600 1 300 100 60 10000
  host configuration "appendonly": yes
  multi-thread: no

Summary:
  throughput summary: 1408450.75 requests per second
  latency summary (msec):
          avg       min       p50       p95       p99       max
        1.638     0.560     1.599     2.135     2.431    17.231
```

</details>
