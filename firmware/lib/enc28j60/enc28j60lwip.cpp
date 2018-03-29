#include <enc28j60/enc28j60lwip.h>
#include "netif/ethernet.h"
#include "netif/etharp.h"
#include "lwip/netifapi.h"
#include <common/utils.h>

#if NO_SYS
#error NO_SYS!=0 not supported
#endif

namespace Enc28j60
{
  namespace
  {
    class LwipNetifImpl : public LwipNetif
    {
    public:
      //arbitrary thread
      LwipNetifImpl(std::unique_ptr<Spi>&& spi)
        : m_env(std::make_unique<EnvImpl>(m_netif))
        , m_spi(std::move(spi))
      {
        if (netifapi_netif_add(&m_netif, IP4_ADDR_ANY4, IP4_ADDR_ANY4, IP4_ADDR_ANY4, (void*)this, &init, &tcpip_input) != ERR_OK)
        {
          //fixme
          return;
        }
        netifapi_netif_set_up(&m_netif);
      }

      //arbitrary thread
      virtual void setDefault() override
      {
        netifapi_netif_set_default(&m_netif);
      }

      //arbitrary thread
      virtual void startDhcp() override
      {
        stop();
        netifapi_dhcp_start(&m_netif);
        m_dhcpMode = true;
      }

      //arbitrary thread
      virtual void start(uint32_t ipaddr, uint32_t netmask, uint32_t gw) override
      {
        stop();
        const ip4_addr_t _ipaddr = {ipaddr};
        const ip4_addr_t _netmask = {netmask};
        const ip4_addr_t _gw = {gw};
        netifapi_netif_set_addr(&m_netif, &_ipaddr, &_netmask, &_gw);
      }

      //arbitrary thread
      virtual void stop() override
      {
        if (m_dhcpMode)
        {
          netifapi_dhcp_release_and_stop(&m_netif);
          m_dhcpMode = false;
        } else
          netifapi_netif_set_addr(&m_netif, IP4_ADDR_ANY4, IP4_ADDR_ANY4, IP4_ADDR_ANY4);
      }

    protected:
      netif m_netif = {};
      class PbufImpl : public Pbuf
      {
      public:
        PbufImpl(pbuf* pbuf)
          : m_pbuf(pbuf)
          , m_pbufNext(pbuf)
        {
        }

        virtual size_t size() const override
        {
          return m_pbuf == NULL ? 0 : m_pbuf->tot_len;
        }
        
        virtual bool next(uint8_t** data, size_t* size) override
        {
          return next((const uint8_t**)data, size);
        }

        virtual bool next(const uint8_t** data, size_t* size) const override
        {
          if (m_pbufNext == NULL)
            return false;

          *data = (uint8_t*)m_pbufNext->payload;
          *size = m_pbufNext->len;
          m_pbufNext = m_pbufNext->next;
          return true;
        }

        pbuf* get() const
        {
          return m_pbuf;
        }

      protected:
        pbuf* const m_pbuf;
        mutable pbuf* m_pbufNext;
      };
      class PbufAllocImpl : public PbufImpl
      {
      public:
        PbufAllocImpl(size_t size)
          : PbufImpl(pbuf_alloc(PBUF_RAW, size, PBUF_POOL))
        {
        }

        virtual ~PbufAllocImpl() override
        {
          if (m_pbuf != NULL)
            pbuf_free(m_pbuf);
        }
      };
      class EnvImpl : public Env
      {
      public:
        EnvImpl(netif& netif)
          : m_netif(netif)
        {
        }

        //tcp thread or core lock
        virtual std::unique_ptr<Pbuf> allocatePbuf(size_t size) override
        {
          return std::make_unique<PbufAllocImpl>(size);
        }

        //tcp thread or core lock
        virtual void input(std::unique_ptr<Pbuf>&& packet) override
        {
          if (m_netif.input(static_cast<PbufAllocImpl*>(packet.get())->get(), &m_netif) != ERR_OK)
            ; //fixme: pbuf_free(p);
        }

        //tcp thread or core lock
        virtual void setLinkState(bool linked) override
        {
          if (linked)
            netif_set_link_up(&m_netif);
          else
            netif_set_link_down(&m_netif);
        }

      protected:
        netif& m_netif;
      };
      std::unique_ptr<EnvImpl> m_env;
      std::unique_ptr<Spi> m_spi;
      std::unique_ptr<Device> m_device;
      bool m_dhcpMode = false;

    protected:
      //tcp thread or core lock
      void periodic()
      {
        m_device->periodic();
        sys_timeout(0, periodic, this);
      }

      //tcp thread or core lock
      static void periodic(void* arg)
      {
        return ((LwipNetifImpl*)arg)->periodic();
      }

      //tcp thread or core lock
      err_t init()
      {
        m_device = CreateDevice(std::move(m_env), std::move(m_spi));
        //fixme: return !ERR_OK on error
    
        m_netif.hwaddr_len = ETH_HWADDR_LEN;
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
        periodic();
        return ERR_OK;
      }

      //tcp thread or core lock
      static err_t init(netif *netif)
      {
        return ((LwipNetifImpl*)netif->state)->init();
      }

      //tcp thread or core lock
      err_t linkoutput(pbuf *p)
      {
        m_device->output(std::make_unique<PbufImpl>(p));
        //fixme: return !ERR_OK on error
        return ERR_OK;
      }

      //tcp thread or core lock
      static err_t linkoutput(netif *netif, pbuf *p)
      {
        return ((LwipNetifImpl*)netif->state)->linkoutput(p);
      }
    };

    //tcp thread or core lock
    static void initDone(void *arg)
    {
      ((OS::BinarySemaphore*)arg)->signal();
    }
  }

  //arbitrary thread
  void LwipNetif::initLwip()
  {
    OS::BinarySemaphore sem;
    tcpip_init(&initDone, (void*)&sem);
    sem.wait();
  }
}

auto Enc28j60::CreateLwipNetif(std::unique_ptr<Spi>&& spi) -> std::unique_ptr<LwipNetif>
{
  return std::make_unique<LwipNetifImpl>(std::move(spi));
}
