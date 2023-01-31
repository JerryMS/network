#pragma once

#include <atomic>
#include <chrono>
#include <thread>
#include "ft_socket.hpp"

namespace FtTCP {

class Broadcast;

using BroadcastPtr = std::shared_ptr<Broadcast>;

class Broadcast {
private:
  SocketPtr m_socket;
  std::atomic_bool m_shutdown{false};
  std::string m_interfaceIP;
  bool m_connected;
  std::string m_appVersion;
  uint32_t m_attemp;
  std::mutex m_updateMutex;
  std::thread m_workerThread;
  static constexpr std::chrono::seconds m_attempTimeout{5};
  static constexpr std::chrono::milliseconds m_throtleTime{300};
  static constexpr uint32_t m_maxAttempCount{11};
  static constexpr uint16_t m_port{3055};
public:
  Broadcast(const std::string& app_version);
  bool Start();
  void Stop();
  bool Renew(bool connected, std::string interface_ip = std::string(""));

  static BroadcastPtr CreateBroadcast(const std::string& appVersion);
private:
  void Run();
  bool SendBroadcastPacket();
};

} // namespace FtTCP
