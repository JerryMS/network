
#include "ft-socket/ft_broadcast.hpp"
#include "esp_log.h"

namespace FtTCP {

Broadcast::Broadcast(const std::string& app_version) 
  : m_interfaceIP(""), m_connected(false), m_appVersion(app_version) {}

bool Broadcast::Start()
{
  m_workerThread = std::thread([this]() { this->Run(); });
  return true;
}
void Broadcast::Stop()
{
  if (m_workerThread.joinable()) {
    m_shutdown = true;
    m_workerThread.join();
  }
}

bool Broadcast::Renew(bool connected, std::string interface_ip)
{
  std::lock_guard<std::mutex> lock(m_updateMutex);
  if (connected)
  {
    FtTCP::AddressPtr addr = FtTCP::Address::CreateBroadcastAddress(interface_ip.c_str(), m_port);
    if (!addr->IsValid())
    {
      return false;
    }
    m_socket = nullptr; 
    m_socket = Socket::CreateSocket(addr);
  }
  m_connected = connected;
  m_interfaceIP = interface_ip;
  return true;
}

void Broadcast::Run()
{
  auto start = std::chrono::steady_clock::now();
  ESP_LOGI("Broadcast", "Started");
  while (!m_shutdown) {
    if (m_connected && m_attemp < m_maxAttempCount) {
      auto end = std::chrono::steady_clock::now();
      std::chrono::seconds elapsed =
        std::chrono::duration_cast<std::chrono::seconds>(end - start);
      if (elapsed >= m_attempTimeout) {
        if (SendBroadcastPacket()) {
          m_attemp++;
          start = end;
        }
      }
    }
    else if (!m_connected && m_attemp > 0) {
      m_attemp = 0;
    }
    std::this_thread::sleep_for(m_throtleTime);
  }
}

bool Broadcast::SendBroadcastPacket()
{
  return m_socket->SendDatagram(
    static_cast<const void*>(m_appVersion.c_str()), m_appVersion.length());
}

BroadcastPtr Broadcast::CreateBroadcast(const std::string& appVersion)
{
  return std::make_shared<Broadcast>(appVersion);
}

} // namespace FtTCP
