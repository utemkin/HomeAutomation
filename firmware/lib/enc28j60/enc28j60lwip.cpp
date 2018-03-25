#include <enc28j60/enc28j60lwip.h>
#include "lwip.h"
#include "lwip/netifapi.h"
#include <common/utils.h>

namespace
{
  class Enc28j60lwipImpl : public Enc28j60lwip
  {
  public:
    //arbitrary thread
    Enc28j60lwipImpl()
    {
      ip4_addr_t ipaddr = {};
      ip4_addr_t netmask = {};
      ip4_addr_t gw = {};
      if (netifapi_netif_add(&m_netif, &ipaddr, &netmask, &gw, (void*)this, &init, &tcpip_input) != ERR_OK)
      {
        //fixme
      }
    }

    //arbitrary thread
    static void initLwip()
    {
      OS::BinarySemaphore sem;
      tcpip_init(&initDone, (void*)&sem);
      sem.wait();
    }

    //arbitrary thread
    virtual std::unique_ptr<Enc28j60Pbuf> allocatePbuf(size_t size) override
    {
      //fixme
    }

    //arbitrary thread
    virtual void input(std::unique_ptr<Enc28j60Pbuf>&& packet) override
    {
      //fixme
    }

    //arbitrary thread
    virtual void setLinkState(bool linked) override
    {
      //fixme
    }

  protected:
    netif m_netif = {};

  protected:
    //tcp thread
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
      m_netif.linkoutput = linkoutput;
      return ERR_OK;
    }

    //tcp thread
    static err_t init(netif *netif)
    {
      return ((Enc28j60lwipImpl*)netif->state)->init();
    }

    //tcp thread
    static void initDone(void *arg)
    {
      ((OS::BinarySemaphore*)arg)->signal();
    }

    //tcp thread
    err_t linkoutput(pbuf *p)
    {
      //fixme
    }

    //tcp thread
    static err_t linkoutput(netif *netif, pbuf *p)
    {
      return ((Enc28j60lwipImpl*)netif->state)->linkoutput(p);
    }
  };
}

std::unique_ptr<Enc28j60lwip> CreateEnc28j60lwip()
{
  return std::make_unique<Enc28j60lwipImpl>();
}
