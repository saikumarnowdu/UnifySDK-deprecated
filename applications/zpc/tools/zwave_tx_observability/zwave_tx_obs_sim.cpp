/******************************************************************************
 * Standalone resolver → TX-queue observability simulator.
 *
 * Uses the production send-time comparator and route-cache scoring.
 * Does not talk to an NCP. Run the binary and open http://127.0.0.1:8765/
 *****************************************************************************/
#include "zwave_tx_queue.hpp"
#include "zwave_tx_route_cache.h"
#include "zwave_tx.h"

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <algorithm>
#include <chrono>
#include <cstring>
#include <deque>
#include <fstream>
#include <iostream>
#include <mutex>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

namespace
{
constexpr int kPort              = 8765;
constexpr int kNodeCount         = 232;
constexpr int kQueueCapacity     = ZWAVE_TX_QUEUE_BUFFER_SIZE;
constexpr int kResolverLimit     = ZWAVE_TX_QUEUE_BUFFER_SIZE - 1;
constexpr uint32_t kResolverQos  = 0x0000FFFFu;
constexpr size_t kEventLimit     = 180;

enum class NodeState { Idle, Pending, Deferred, Queued, Sending, Done };

struct NodeProfile {
  zwave_node_id_t id;
  const char *role;
  uint8_t hops;
  last_route_speed_t speed;
  int8_t rssi;
  bool beam;
  NodeState state;
};

struct QueueItem {
  zwave_tx_queue_element_t element;
  uint32_t request_id;
};

struct Event {
  uint32_t ts_ms;
  std::string type;
  std::string detail;
};

struct Sim {
  std::mutex mu;
  bool running     = true;
  bool paused      = false;
  int speed_x      = 8;
  uint32_t sim_ms  = 0;
  uint32_t next_req = 1;
  uint32_t enqueue_seq = 0;

  std::vector<NodeProfile> nodes;
  std::deque<zwave_node_id_t> resolver_pending;
  std::vector<QueueItem> queue;
  bool sending           = false;
  zwave_node_id_t send_node = 0;
  uint32_t send_until_ms = 0;
  uint32_t send_req      = 0;
  zwave_tx_session_id_t send_session = nullptr;

  uint32_t requested  = 0;
  uint32_t completed  = 0;
  uint32_t deferred_total = 0;
  uint32_t enqueued_total = 0;

  std::deque<Event> events;
};

Sim g;
std::string g_dashboard;
bool g_logged_defer = false;

const char *speed_name(last_route_speed_t s)
{
  switch (s) {
    case ZWAVE_100_KBITS_S:
    case ZWAVE_LONG_RANGE_100_KBITS_S:
      return "100k";
    case ZWAVE_40_KBITS_S:
      return "40k";
    case ZWAVE_9_6_KBITS_S:
      return "9.6k";
    default:
      return "unk";
  }
}

void push_event(const std::string &type, const std::string &detail)
{
  if (g.events.size() >= kEventLimit) {
    g.events.pop_front();
  }
  g.events.push_back({g.sim_ms, type, detail});
}

NodeProfile make_profile(zwave_node_id_t id)
{
  NodeProfile n{};
  n.id    = id;
  n.state = NodeState::Idle;
  if (id <= 40) {
    n.role  = "AL-router";
    n.hops  = 0;
    n.speed = ZWAVE_100_KBITS_S;
    n.rssi  = static_cast<int8_t>(-40 - (id % 15));
    n.beam  = false;
  } else if (id <= 90) {
    n.role  = "AL-1hop";
    n.hops  = 1;
    n.speed = ZWAVE_40_KBITS_S;
    n.rssi  = static_cast<int8_t>(-70);
    n.beam  = false;
  } else if (id <= 160) {
    n.role  = "AL-routed";
    n.hops  = 3;
    n.speed = ZWAVE_9_6_KBITS_S;
    n.rssi  = static_cast<int8_t>(-88);
    n.beam  = false;
  } else {
    n.role  = "FL-beam";
    n.hops  = 4;
    n.speed = ZWAVE_9_6_KBITS_S;
    n.rssi  = static_cast<int8_t>(-95);
    n.beam  = true;
  }
  return n;
}

void reset_locked()
{
  g.sim_ms         = 0;
  g.next_req       = 1;
  g.enqueue_seq    = 0;
  g.requested      = 0;
  g.completed      = 0;
  g.deferred_total = 0;
  g.enqueued_total = 0;
  g.sending        = false;
  g_logged_defer   = false;
  g.send_node      = 0;
  g.send_until_ms  = 0;
  g.resolver_pending.clear();
  g.queue.clear();
  g.events.clear();
  g.nodes.clear();
  g.nodes.reserve(kNodeCount);
  for (zwave_node_id_t id = 1; id <= kNodeCount; ++id) {
    g.nodes.push_back(make_profile(id));
  }
  zwave_tx_route_cache_init();
}

uint32_t airtime_ms(const NodeProfile &n)
{
  uint32_t t = 12;
  t += static_cast<uint32_t>(n.hops) * 20u;
  if (n.speed == ZWAVE_9_6_KBITS_S) {
    t += 40;
  } else if (n.speed == ZWAVE_40_KBITS_S) {
    t += 15;
  }
  if (n.beam) {
    t += 250;
  }
  return t;
}

void seed_metrics_locked()
{
  for (const NodeProfile &n: g.nodes) {
    zwapi_tx_report_t report = {};
    report.number_of_repeaters = n.hops;
    report.ack_rssi            = n.rssi;
    report.last_route_speed    = n.speed;
    report.beam_1000ms         = n.beam;
    zwave_tx_route_cache_update_from_tx_report(n.id, &report);
  }
  push_event("seed", "loaded last-TX hops/speed/RSSI/beam for all 232 nodes");
}

void burst_locked()
{
  for (NodeProfile &n: g.nodes) {
    if (n.state == NodeState::Sending || n.state == NodeState::Queued) {
      continue;
    }
    n.state = NodeState::Pending;
    g.resolver_pending.push_back(n.id);
    g.requested++;
  }
  push_event("burst",
             "resolver scan queued SET for "
               + std::to_string(g.resolver_pending.size()) + " nodes");
}

const char *state_name(NodeState s)
{
  switch (s) {
    case NodeState::Pending:
      return "pending";
    case NodeState::Deferred:
      return "deferred";
    case NodeState::Queued:
      return "queued";
    case NodeState::Sending:
      return "sending";
    case NodeState::Done:
      return "done";
    default:
      return "idle";
  }
}

zwave_tx_queue_element_t make_element(zwave_node_id_t node_id, uint32_t seq)
{
  zwave_tx_queue_element_t e = {};
  e.options.qos_priority     = kResolverQos;
  e.connection_info.remote.node_id      = node_id;
  e.connection_info.remote.is_multicast = false;
  e.queue_timestamp                     = seq;
  e.zwave_tx_session_id = reinterpret_cast<zwave_tx_session_id_t>(
    static_cast<uintptr_t>(seq + 1));
  return e;
}

void fill_from_resolver_locked()
{
  while (!g.resolver_pending.empty()) {
    if (static_cast<int>(g.queue.size()) + (g.sending ? 1 : 0) >= kResolverLimit) {
      for (zwave_node_id_t id: g.resolver_pending) {
        g.nodes[id - 1].state = NodeState::Deferred;
      }
      g.deferred_total += 1;
      if (!g_logged_defer) {
        g_logged_defer = true;
        push_event("defer",
                   "TX queue at capacity "
                     + std::to_string(g.queue.size() + (g.sending ? 1 : 0))
                     + "/" + std::to_string(kQueueCapacity)
                     + " — resolver returns NOT_READY, desired kept");
      }
      break;
    }
    g_logged_defer = false;
    const zwave_node_id_t id = g.resolver_pending.front();
    g.resolver_pending.pop_front();
    QueueItem item;
    item.element    = make_element(id, g.enqueue_seq++);
    item.request_id = g.next_req++;
    g.queue.push_back(item);
    g.nodes[id - 1].state = NodeState::Queued;
    g.enqueued_total++;
    push_event("enqueue",
               "SET Node " + std::to_string(id) + " score="
                 + std::to_string(zwave_tx_route_cache_link_score(id)));
  }
}

void pick_and_send_locked()
{
  if (g.sending || g.queue.empty()) {
    return;
  }
  queue_element_send_compare compare;
  auto best = g.queue.begin();
  for (auto it = g.queue.begin() + 1; it != g.queue.end(); ++it) {
    if (compare(it->element, best->element)) {
      best = it;
    }
  }
  const zwave_node_id_t id = best->element.connection_info.remote.node_id;
  g.sending      = true;
  g.send_node    = id;
  g.send_req     = best->request_id;
  g.send_session = best->element.zwave_tx_session_id;
  g.send_until_ms = g.sim_ms + airtime_ms(g.nodes[id - 1]);
  g.nodes[id - 1].state = NodeState::Sending;
  g.queue.erase(best);
  push_event("tx_send",
             "Node " + std::to_string(id) + " score="
               + std::to_string(zwave_tx_route_cache_link_score(id))
               + " airtime_ms="
               + std::to_string(airtime_ms(g.nodes[id - 1])));
}

void complete_send_locked()
{
  if (!g.sending || g.sim_ms < g.send_until_ms) {
    return;
  }
  NodeProfile &n = g.nodes[g.send_node - 1];
  zwapi_tx_report_t report = {};
  report.number_of_repeaters = n.hops;
  report.ack_rssi            = n.rssi;
  report.last_route_speed    = n.speed;
  report.beam_1000ms         = n.beam;
  zwave_tx_route_cache_update_from_tx_report(n.id, &report);
  n.state = NodeState::Done;
  g.completed++;
  g.sending = false;
  push_event("tx_complete",
             "Node " + std::to_string(n.id) + " hops=" + std::to_string(n.hops)
               + " " + speed_name(n.speed) + " rssi=" + std::to_string(n.rssi)
               + (n.beam ? " BEAM" : ""));
  g.send_node = 0;
}

void tick_locked()
{
  fill_from_resolver_locked();
  complete_send_locked();
  fill_from_resolver_locked();
  pick_and_send_locked();
  g.sim_ms += 1;
}

std::string json_escape(const std::string &in)
{
  std::string out;
  out.reserve(in.size());
  for (char c: in) {
    if (c == '"' || c == '\\') {
      out.push_back('\\');
    }
    out.push_back(c);
  }
  return out;
}

std::string state_json()
{
  std::lock_guard<std::mutex> lock(g.mu);
  std::ostringstream o;
  const int used = static_cast<int>(g.queue.size()) + (g.sending ? 1 : 0);
  int deferred_current = 0;
  for (const NodeProfile &n: g.nodes) {
    if (n.state == NodeState::Deferred || n.state == NodeState::Pending) {
      deferred_current++;
    }
  }
  o << "{\"sim_ms\":" << g.sim_ms << ",\"paused\":" << (g.paused ? "true" : "false")
    << ",\"queue_size\":" << used << ",\"queue_capacity\":" << kQueueCapacity
    << ",\"resolver_limit\":" << kResolverLimit << ",\"counts\":{"
    << "\"requested\":" << g.requested << ",\"enqueued\":" << g.enqueued_total
    << ",\"completed\":" << g.completed << ",\"deferred_current\":"
    << deferred_current << "},";
  if (g.sending) {
    o << "\"current_send\":{\"node_id\":" << g.send_node << ",\"score\":"
      << zwave_tx_route_cache_link_score(g.send_node) << "},";
  } else {
    o << "\"current_send\":null,";
  }

  std::vector<QueueItem> ordered = g.queue;
  queue_element_send_compare compare;
  std::sort(ordered.begin(),
            ordered.end(),
            [&](const QueueItem &a, const QueueItem &b) {
              return compare(a.element, b.element);
            });
  o << "\"queue\":[";
  for (size_t i = 0; i < ordered.size(); ++i) {
    const zwave_node_id_t id = ordered[i].element.connection_info.remote.node_id;
    const NodeProfile &n     = g.nodes[id - 1];
    if (i) {
      o << ",";
    }
    o << "{\"node_id\":" << id << ",\"role\":\"" << n.role << "\",\"qos\":"
      << ordered[i].element.options.qos_priority << ",\"score\":"
      << zwave_tx_route_cache_link_score(id) << ",\"hops\":" << int(n.hops)
      << ",\"speed\":\"" << speed_name(n.speed) << "\",\"rssi\":" << int(n.rssi)
      << ",\"beam\":" << (n.beam ? "true" : "false") << "}";
  }
  o << "],\"nodes\":[";
  for (size_t i = 0; i < g.nodes.size(); ++i) {
    const NodeProfile &n = g.nodes[i];
    if (i) {
      o << ",";
    }
    o << "{\"id\":" << n.id << ",\"role\":\"" << n.role << "\",\"state\":\""
      << state_name(n.state) << "\",\"score\":"
      << zwave_tx_route_cache_link_score(n.id) << ",\"hops\":" << int(n.hops)
      << ",\"speed\":\"" << speed_name(n.speed) << "\",\"rssi\":" << int(n.rssi)
      << ",\"beam\":" << (n.beam ? "true" : "false") << "}";
  }
  o << "],\"events\":[";
  bool first = true;
  for (const Event &e: g.events) {
    if (!first) {
      o << ",";
    }
    first = false;
    o << "{\"ts_ms\":" << e.ts_ms << ",\"type\":\"" << json_escape(e.type)
      << "\",\"detail\":\"" << json_escape(e.detail) << "\"}";
  }
  o << "]}";
  return o.str();
}

std::string load_dashboard(const char *hint)
{
  const char *candidates[] = {
#ifdef DASHBOARD_HTML_PATH
    DASHBOARD_HTML_PATH,
#endif
    hint,
    "dashboard.html",
    "applications/zpc/tools/zwave_tx_observability/dashboard.html",
    "/workspace/applications/zpc/tools/zwave_tx_observability/dashboard.html",
    nullptr};
  for (int i = 0; candidates[i]; ++i) {
    std::ifstream in(candidates[i]);
    if (!in) {
      continue;
    }
    std::ostringstream ss;
    ss << in.rdbuf();
    if (!ss.str().empty()) {
      return ss.str();
    }
  }
  return "<html><body>dashboard.html missing</body></html>";
}

void http_reply(int fd, const std::string &status, const std::string &ctype,
                const std::string &body)
{
  std::ostringstream h;
  h << "HTTP/1.1 " << status
    << "\r\nContent-Type: " << ctype
    << "\r\nContent-Length: " << body.size()
    << "\r\nConnection: close\r\nCache-Control: no-store\r\n\r\n";
  const std::string hdr = h.str();
  send(fd, hdr.data(), hdr.size(), 0);
  send(fd, body.data(), body.size(), 0);
}

void handle_client(int fd)
{
  char buf[2048];
  const ssize_t n = recv(fd, buf, sizeof(buf) - 1, 0);
  if (n <= 0) {
    close(fd);
    return;
  }
  buf[n]             = 0;
  const std::string req(buf);
  const bool is_post = req.rfind("POST ", 0) == 0;
  auto path_end      = req.find(' ', 5);
  std::string path   = req.substr(is_post ? 5 : 4,
                                path_end == std::string::npos
                                    ? std::string::npos
                                    : path_end - (is_post ? 5 : 4));
  if (path.empty() || path[0] != '/') {
    path = "/";
  }

  if (path == "/" || path == "/index.html") {
    http_reply(fd, "200 OK", "text/html; charset=utf-8", g_dashboard);
  } else if (path == "/api/state") {
    http_reply(fd, "200 OK", "application/json", state_json());
  } else if (is_post && path.rfind("/api/", 0) == 0) {
    std::lock_guard<std::mutex> lock(g.mu);
    if (path == "/api/reset") {
      reset_locked();
    } else if (path == "/api/burst") {
      burst_locked();
    } else if (path == "/api/second-burst") {
      for (NodeProfile &n: g.nodes) {
        n.state = NodeState::Idle;
      }
      burst_locked();
    } else if (path == "/api/seed") {
      seed_metrics_locked();
    } else if (path == "/api/pause") {
      g.paused = !g.paused;
    } else if (path.rfind("/api/speed", 0) == 0) {
      const auto x = path.find("x=");
      if (x != std::string::npos) {
        g.speed_x = std::max(1, std::min(50, atoi(path.c_str() + x + 2)));
      }
    }
    http_reply(fd, "200 OK", "application/json", "{\"ok\":true}");
  } else {
    http_reply(fd, "404 Not Found", "text/plain", "not found");
  }
  close(fd);
}

void sim_thread()
{
  while (g.running) {
    {
      std::lock_guard<std::mutex> lock(g.mu);
      if (!g.paused) {
        const int steps = std::max(1, g.speed_x);
        for (int i = 0; i < steps; ++i) {
          tick_locked();
        }
      }
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(8));
  }
}
}  // namespace

extern "C" void sl_log(const char *const, sl_log_level_t, const char *, ...) {}
extern "C" void sl_log_set_level(sl_log_level_t) {}
extern "C" sl_log_level_t sl_log_get_level()
{
  return SL_LOG_DEBUG;
}
extern "C" void sl_log_set_tag_level(const char *, sl_log_level_t) {}
extern "C" void sl_log_unset_tag_level(const char *) {}
extern "C" sl_status_t sl_log_level_from_string(const char *, sl_log_level_t *)
{
  return SL_STATUS_OK;
}
extern "C" void sl_log_read_config() {}

int main(int argc, char **argv)
{
  const char *hint = nullptr;
  if (argc > 1) {
    hint = argv[1];
  }
  g_dashboard = load_dashboard(hint);
  {
    std::lock_guard<std::mutex> lock(g.mu);
    reset_locked();
  }

  const int srv = socket(AF_INET, SOCK_STREAM, 0);
  int opt       = 1;
  setsockopt(srv, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
  sockaddr_in addr{};
  addr.sin_family      = AF_INET;
  addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
  addr.sin_port        = htons(kPort);
  if (bind(srv, reinterpret_cast<sockaddr *>(&addr), sizeof(addr)) < 0) {
    std::cerr << "bind failed on 127.0.0.1:" << kPort << "\n";
    return 1;
  }
  listen(srv, 16);
  std::thread worker(sim_thread);
  std::cout << "Resolver/TX observability: http://127.0.0.1:" << kPort << "/\n";
  while (g.running) {
    const int fd = accept(srv, nullptr, nullptr);
    if (fd >= 0) {
      handle_client(fd);
    }
  }
  worker.join();
  close(srv);
  return 0;
}
