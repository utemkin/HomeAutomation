#include <enc28j60/enc28j60lwip.h>
#include "lwip.h"
#include "lwip/netifapi.h"
#include <common/utils.h>

class enc28j60lwip : mstd::noncopyable
{
public:
  enc28j60lwip()
  {
    ip4_addr_t ipaddr = {};
    ip4_addr_t netmask = {};
    ip4_addr_t gw = {};
    if (netifapi_netif_add(&m_netif, &ipaddr, &netmask, &gw, (void*)this, &init, &tcpip_input) != ERR_OK)
    {
      //fixme
    }
  }

  static void initLwip()
  {
    OS::BinarySemaphore sem;
    tcpip_init(&initDone, (void*)&sem);
    sem.wait();
  }

protected:
  netif m_netif = {};

protected:
  err_t init()
  {
    
    //fixme: init hardware, return !ERR_OK on error
    
    m_netif.hwaddr_len = ETHARP_HWADDR_LEN;
//    m_netif.hwaddr[0] = ?;
//    m_netif.hwaddr[1] = ?;
//    m_netif.hwaddr[2] = ?;
//    m_netif.hwaddr[3] = ?;
//    m_netif.hwaddr[4] = ?;
//    m_netif.hwaddr[5] = ?;
    m_netif.mtu = 1500;
    m_netif.flags = NETIF_FLAG_BROADCAST | NETIF_FLAG_ETHARP;
    m_netif.name[0] = 'e';
    m_netif.name[1] = 'n';
    m_netif.output = etharp_output;
//    m_netif.linkoutput = ?;
    return ERR_OK;
  }
  static err_t init(netif *netif)
  {
    return ((enc28j60lwip*)netif->state)->init();
  }
  static void initDone(void *arg)
  {
    ((OS::BinarySemaphore*)arg)->signal();
  }
};
