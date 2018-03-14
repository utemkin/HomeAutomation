#if 0
#include "enc28j60.h"
#include "enc28j60spi.h"
#include "FreeRTOS.h"
#include "task.h"
#include <stdio.h>

#define container_of(ptr, type, member) ((type *)((char *)(1 ? (ptr) : &((type *)0)->member) - offsetof(type, member)))
#define IS_ELAPSED(initial_tick_count, timeout_ticks) ((TickType_t)(xTaskGetTickCount() - (initial_tick_count)) > (TickType_t)(timeout_ticks))

struct enc28j60_impl
{
  struct enc28j60 iface;
  struct enc28j60spi* spi;
  uint8_t failure_flags;
  uint8_t bank;
  uint8_t lstat;
  uint16_t next_packet;
  union
  {
    uint8_t buf1[1];
    uint8_t buf2[2];
    uint8_t buf3[3];
    uint8_t buf_wait_phy[26];
  };
  int phase;
  TickType_t initial_tick_count;
  TickType_t timeout_ticks;
};

static void reset(struct enc28j60_impl* enc_impl)
{
  reg_clr(enc_impl, ECON2, ECON2_PWRSV);
  vTaskDelay((1 + portTICK_PERIOD_MS - 1) / portTICK_PERIOD_MS + 1);
  op_SRC(enc_impl);
  vTaskDelay((1 + portTICK_PERIOD_MS - 1) / portTICK_PERIOD_MS + 1);
  enc_impl->bank = 0;
  enc_impl->lstat = 0;
  enc_impl->next_packet = 0;
}

static unsigned benchmark(const char* name, void(*test_fn)(struct enc28j60_impl* enc_impl, void* ctx), struct enc28j60_impl* enc, void* ctx, unsigned offset)
{
  printf("Benchmarking %s...\n", name);
  TickType_t start = xTaskGetTickCount();
  while (start == xTaskGetTickCount());
  start = xTaskGetTickCount();
  TickType_t finish = start + 1000 / portTICK_PERIOD_MS;
  unsigned count = 0;
  while (xTaskGetTickCount() < finish)
  {
    test_fn(enc, ctx);
    ++count;
  }
  finish = xTaskGetTickCount();
  unsigned duration_ns = (finish - start) * portTICK_PERIOD_MS * 1000000;
  unsigned cycle_ns = (duration_ns + count / 2) / count - offset;
  unsigned duration_clk = (finish - start) * portTICK_PERIOD_MS * ((SystemCoreClock + 500) / 1000);
  unsigned offset_clk = (offset * ((SystemCoreClock + 500) / 1000) + 500000) / 1000000;
  unsigned cycle_clk = (duration_clk + count / 2) / count - offset_clk;
  printf("               ... cycle = %u ns = %u CLKs\n", cycle_ns, cycle_clk);
  return cycle_ns;
}

static void benchmark_null(struct enc28j60_impl* enc_impl, void* ctx)
{
}

static void benchmark_rxtx(struct enc28j60_impl* enc_impl, void* ctx)
{
  static uint8_t buf[1000];
  enc_impl->spi->txrx(enc_impl->spi, buf, (size_t)ctx);
}

static void benchmark_mem_read(struct enc28j60_impl* enc_impl, void* ctx)
{
  static uint8_t buf[1000];
  mem_read(enc_impl, buf, (size_t)ctx);
}

static void benchmark_mem_write(struct enc28j60_impl* enc_impl, void* ctx)
{
  static uint8_t buf[1000];
  mem_write(enc_impl, buf, (size_t)ctx);
}

static void benchmark_phy_read(struct enc28j60_impl* enc_impl, void* ctx)
{
  phy_read(enc_impl, PHLCON);
}

static void benchmark_phy_write(struct enc28j60_impl* enc_impl, void* ctx)
{
  phy_write(enc_impl, PHLCON, 0x1234);
}

static void benchmark_all(struct enc28j60_impl* enc_impl)
{
  unsigned offset = benchmark("null", &benchmark_null, 0, 0, 0);
  benchmark("rxtx 1", &benchmark_rxtx, enc_impl, (void*)1, offset);
  benchmark("rxtx 2", &benchmark_rxtx, enc_impl, (void*)2, offset);
  benchmark("rxtx 3", &benchmark_rxtx, enc_impl, (void*)3, offset);
  benchmark("mem_read 1000", &benchmark_mem_read, enc_impl, (void*)1000, offset);
  benchmark("mem_read 100", &benchmark_mem_read, enc_impl, (void*)100, offset);
  benchmark("mem_read 10", &benchmark_mem_read, enc_impl, (void*)10, offset);
  benchmark("mem_write 1000", &benchmark_mem_write, enc_impl, (void*)1000, offset);
  benchmark("mem_write 100", &benchmark_mem_write, enc_impl, (void*)100, offset);
  benchmark("mem_write 10", &benchmark_mem_write, enc_impl, (void*)10, offset);
  benchmark("phy_read", &benchmark_phy_read, enc_impl, 0, offset);
  benchmark("phy_write", &benchmark_phy_write, enc_impl, 0, offset);
}
  
// assumes there's no tx currently
static void start_packet_tx(struct enc28j60_impl* enc_impl, uint16_t offset, uint16_t len)
{
  dump_state(enc_impl);
  reg_set(enc_impl, ECON1, ECON1_TXRST);
  reg_clr(enc_impl, ECON1, ECON1_TXRST);
  reg_clr(enc_impl, EIR, EIR_TXIF | EIR_TXERIF);
  reg_write16(enc_impl, ETXST16, offset);
  reg_write16(enc_impl, ETXND16, offset + len - 1);
  reg_set(enc_impl, ECON1, ECON1_TXRTS);
  dump_state(enc_impl);
  dump_state(enc_impl);
}

static void init(struct enc28j60_impl* enc_impl)
{
  reg_write(enc_impl, MACON1, MACON1_TXPAUS | MACON1_RXPAUS | MACON1_MARXEN);
  reg_write(enc_impl, MACON3, MACON3_PADCFG0 | MACON3_TXCRCEN | MACON3_FULDPX);
  reg_write(enc_impl, MABBIPG, 0x15);
  reg_write(enc_impl, MAIPG16, 0x12);
  reg_write16(enc_impl, MAMXFL16, 1518);
  phy_write(enc_impl, PHCON1, PHCON1_PDPXMD);
  phy_write(enc_impl, PHLCON, 0x3c16);         //LEDA = link status and receive activity, LEDB = transmit activity
  phy_write(enc_impl, PHIE, PHIE_PLNKIE | PHIE_PGEIE);
  //fixme: setup MAC address
  reg_write16(enc_impl, ERXST16, 0);
  reg_write16(enc_impl, ERXND16, 0x19ff);
  reg_write16(enc_impl, ERXRDPT16, 0x19ff);
  reg_set(enc_impl, ECON1, ECON1_RXEN);
}

static void check_link(struct enc28j60_impl* enc_impl, uint8_t eir)
{
  if (!(eir & EIR_LINKIF))
  {
    return;
  }
  phy_read(enc_impl, PHIR);
  if (phy_read(enc_impl, PHSTAT2) & PHSTAT2_LSTAT)
  {
    if (enc_impl->lstat)
    {
      //fixme
      printf("link pulsed down then up\n");
      enc_impl->lstat = 0;
      enc_impl->lstat = 1;
    }
    else
    {
      //fixme
      printf("link up\n");
      enc_impl->lstat = 1;
    }
  }
  else
  {
    if (enc_impl->lstat)
    {
      //fixme
      printf("link down\n");
      enc_impl->lstat = 0;
    }
    else
    {
      //fixme
      printf("link pulsed up then down\n");
    }
  }
}

static void check_rx(struct enc28j60_impl* enc_impl, uint8_t eir)
{
  if (eir & EIR_RXERIF)
  {
    //fixme
    printf("buffer overflow\n");
  }
  while (reg_read(enc_impl, EPKTCNT))
  {
    reg_set(enc_impl, ECON2, ECON2_PKTDEC);
    reg_write16(enc_impl, ERDPT16, enc_impl->next_packet);
    uint8_t rsv[6];
    mem_read(enc_impl, rsv, sizeof(rsv));
    uint16_t next_packet = rsv[0] | (rsv[1] << 8);
    uint16_t len = rsv[2] | (rsv[3] << 8);
    printf("packet received %u\n", len);
    reg_write16(enc_impl, ERXRDPT16, next_packet ? next_packet - 1 : 0x19ff);
    enc_impl->next_packet = next_packet;
  }
}

static void check_if(struct enc28j60_impl* enc_impl)
{
  uint8_t eir = reg_read(enc_impl, EIR);
  uint8_t eir_clearable = eir & (EIR_TXIF | EIR_TXERIF | EIR_RXERIF);
  if (eir_clearable)
  {
    reg_clr(enc_impl, EIR, eir_clearable);
  }
  check_link(enc_impl, eir);
  check_rx(enc_impl, eir);
}

void enc28j60_test(struct enc28j60spi* spi)
{
  struct enc28j60_impl enc;
  enc.spi = spi;
  enc.failure_flags = 0;
  enc.phase = 0;
  reset(&enc);
  validate(&enc);
  dump_regs(&enc);
  benchmark_all(&enc);
  return;
  init(&enc);
  for (;;) check_if(&enc);
  reg_write16(&enc, EWRPT16, 0);
  const uint8_t zero = 0;
  for (int i = 0; i < 100; ++i)
  {
    mem_write(&enc, &zero, 1);
  }
  start_packet_tx(&enc, 0, 15);
  reg_write16(&enc, ERDPT16, 15);
  for (int i = 15; i < 15 + 7; ++i)
  {
    uint8_t byte;
    mem_read(&enc, &byte, 1);
    printf("%02x ", byte);
  }
  printf("\n");
}

void enc28j60_poll(
    struct enc28j60* enc)
{
  struct enc28j60_impl* enc_impl = container_of(enc, struct enc28j60_impl, iface);
  for (;;)
  {
    switch (enc_impl->phase)
    {
    case 0:
      reg_clr(enc_impl, ECON2, ECON2_PWRSV);
      enc_impl->initial_tick_count = xTaskGetTickCount();
      enc_impl->timeout_ticks = 1;
      ++enc_impl->phase;
      return;
    case 1:
      if (!IS_ELAPSED(enc_impl->initial_tick_count, enc_impl->timeout_ticks))
      {
        return;
      }
      ++enc_impl->phase;
      //no break
    case 2:
      op_SRC(enc_impl);
      enc_impl->initial_tick_count = xTaskGetTickCount();
      enc_impl->timeout_ticks = 1;
      ++enc_impl->phase;
      return;
    case 3:
      if (!IS_ELAPSED(enc_impl->initial_tick_count, enc_impl->timeout_ticks))
      {
        return;
      }
      ++enc_impl->phase;
      //no break
    }
  }
}
#endif

#include <enc28j60/enc28j60.h>
#include <type_traits>

namespace
{
  namespace Reg
  {
    static constexpr uint8_t c_BANK_SHIFT = 6;
    static constexpr uint8_t c_ETH_MASK   = 0b00100000;
    static constexpr uint8_t c_NUM_MASK   = 0b00011111;

    constexpr uint8_t makeAddr(const uint8_t bank, const uint8_t num, const bool eth)
    {
      return (bank << c_BANK_SHIFT) | num | (eth ? c_ETH_MASK : 0);
    }

    enum class Addr : uint8_t
    {
      EIE       = makeAddr(3, 0x1b, true),
      EIR       = makeAddr(3, 0x1c, true),
      ESTAT     = makeAddr(3, 0x1d, true),
      ECON2     = makeAddr(3, 0x1e, true),
      ECON1     = makeAddr(3, 0x1f, true),

      ERDPT16   = makeAddr(0, 0x00, true),
      EWRPT16   = makeAddr(0, 0x02, true),
      ETXST16   = makeAddr(0, 0x04, true),
      ETXND16   = makeAddr(0, 0x06, true),
      ERXST16   = makeAddr(0, 0x08, true),
      ERXND16   = makeAddr(0, 0x0a, true),
      ERXRDPT16 = makeAddr(0, 0x0c, true),
      ERXWRPT16 = makeAddr(0, 0x0e, true),
      EDMAST16  = makeAddr(0, 0x10, true),
      EDMAND16  = makeAddr(0, 0x12, true),
      EDMADST16 = makeAddr(0, 0x14, true),
      EDMACS16  = makeAddr(0, 0x16, true),

      EHT0      = makeAddr(1, 0x00, true),
      EHT1      = makeAddr(1, 0x01, true),
      EHT2      = makeAddr(1, 0x02, true),
      EHT3      = makeAddr(1, 0x03, true),
      EHT4      = makeAddr(1, 0x04, true),
      EHT5      = makeAddr(1, 0x05, true),
      EHT6      = makeAddr(1, 0x06, true),
      EHT7      = makeAddr(1, 0x07, true),
      EPMM0     = makeAddr(1, 0x08, true),
      EPMM1     = makeAddr(1, 0x09, true),
      EPMM2     = makeAddr(1, 0x0a, true),
      EPMM3     = makeAddr(1, 0x0b, true),
      EPMM4     = makeAddr(1, 0x0c, true),
      EPMM5     = makeAddr(1, 0x0d, true),
      EPMM6     = makeAddr(1, 0x0e, true),
      EPMM7     = makeAddr(1, 0x0f, true),
      EPMCS16   = makeAddr(1, 0x10, true),
      EPMO16    = makeAddr(1, 0x14, true),
      ERXFCON   = makeAddr(1, 0x18, true),
      EPKTCNT   = makeAddr(1, 0x19, true),

      MACON1    = makeAddr(2, 0x00, false),
      MACON3    = makeAddr(2, 0x02, false),
      MACON4    = makeAddr(2, 0x03, false),
      MABBIPG   = makeAddr(2, 0x04, false),
      MAIPG16   = makeAddr(2, 0x06, false),
      MACLCON1  = makeAddr(2, 0x08, false),
      MACLCON2  = makeAddr(2, 0x09, false),
      MAMXFL16  = makeAddr(2, 0x0a, false),
      MICMD     = makeAddr(2, 0x12, false),
      MIREGADR  = makeAddr(2, 0x14, false),
      MIWR16    = makeAddr(2, 0x16, false),
      MIRD16    = makeAddr(2, 0x18, false),

      MAADR5    = makeAddr(3, 0x00, false),
      MAADR6    = makeAddr(3, 0x01, false),
      MAADR3    = makeAddr(3, 0x02, false),
      MAADR4    = makeAddr(3, 0x03, false),
      MAADR1    = makeAddr(3, 0x04, false),
      MAADR2    = makeAddr(3, 0x05, false),
      EBSTSD    = makeAddr(3, 0x06, true),
      EBSTCON   = makeAddr(3, 0x07, true),
      EBSTCS16  = makeAddr(3, 0x08, true),
      MISTAT    = makeAddr(3, 0x0a, false),
      EREVID    = makeAddr(3, 0x12, true),
      ECOCON    = makeAddr(3, 0x15, true),
      EFLOCON   = makeAddr(3, 0x17, true),
      EPAUS16   = makeAddr(3, 0x18, true),
    };

    constexpr uint8_t bank(const Addr addr)
    {
      return uint8_t(addr) >> c_BANK_SHIFT;
    }
    constexpr uint8_t num(const Addr addr)
    {
      return uint8_t(addr) & c_NUM_MASK;
    }
    constexpr bool eth(const Addr addr)
    {
      return !!(uint8_t(addr) & c_ETH_MASK);
    }
    constexpr bool anyBank(const Addr addr)
    {
      return addr >= Addr::EIE;
    }

    static constexpr uint8_t c_EIE_INTIE      = 0b10000000;
    static constexpr uint8_t c_EIE_PKTIE      = 0b01000000;
    static constexpr uint8_t c_EIE_DMAIE      = 0b00100000;
    static constexpr uint8_t c_EIE_LINKIE     = 0b00010000;
    static constexpr uint8_t c_EIE_TXIE       = 0b00001000;
    static constexpr uint8_t c_EIE_TXERIE     = 0b00000010;
    static constexpr uint8_t c_EIE_RXERIE     = 0b00000001;

    static constexpr uint8_t c_EIR_PKTIF      = 0b01000000;
    static constexpr uint8_t c_EIR_DMAIF      = 0b00100000;
    static constexpr uint8_t c_EIR_LINKIF     = 0b00010000;
    static constexpr uint8_t c_EIR_TXIF       = 0b00001000;
    static constexpr uint8_t c_EIR_TXERIF     = 0b00000010;
    static constexpr uint8_t c_EIR_RXERIF     = 0b00000001;

    static constexpr uint8_t c_ESTAT_INT      = 0b10000000;
    static constexpr uint8_t c_ESTAT_BUFER    = 0b01000000;
    static constexpr uint8_t c_ESTAT_LATECOL  = 0b00010000;
    static constexpr uint8_t c_ESTAT_RXBUSY   = 0b00000100;
    static constexpr uint8_t c_ESTAT_TXABRT   = 0b00000010;
    static constexpr uint8_t c_ESTAT_CLKRDY   = 0b00000001;

    static constexpr uint8_t c_ECON2_AUTOINC  = 0b10000000;
    static constexpr uint8_t c_ECON2_PKTDEC   = 0b01000000;
    static constexpr uint8_t c_ECON2_PWRSV    = 0b00100000;
    static constexpr uint8_t c_ECON2_VRPS     = 0b00001000;

    static constexpr uint8_t c_ECON1_TXRST    = 0b10000000;
    static constexpr uint8_t c_ECON1_RXRST    = 0b01000000;
    static constexpr uint8_t c_ECON1_DMAST    = 0b00100000;
    static constexpr uint8_t c_ECON1_CSUMEN   = 0b00010000;
    static constexpr uint8_t c_ECON1_TXRTS    = 0b00001000;
    static constexpr uint8_t c_ECON1_RXEN     = 0b00000100;
    static constexpr uint8_t c_ECON1_BSEL1    = 0b00000010;
    static constexpr uint8_t c_ECON1_BSEL0    = 0b00000001;

    static constexpr uint8_t c_MACON1_TXPAUS  = 0b00001000;
    static constexpr uint8_t c_MACON1_RXPAUS  = 0b00000100;
    static constexpr uint8_t c_MACON1_PASSALL = 0b00000010;
    static constexpr uint8_t c_MACON1_MARXEN  = 0b00000001;

    static constexpr uint8_t c_MACON3_PADCFG2 = 0b10000000;
    static constexpr uint8_t c_MACON3_PADCFG1 = 0b01000000;
    static constexpr uint8_t c_MACON3_PADCFG0 = 0b00100000;
    static constexpr uint8_t c_MACON3_TXCRCEN = 0b00010000;
    static constexpr uint8_t c_MACON3_PHDREN  = 0b00001000;
    static constexpr uint8_t c_MACON3_HFRMEN  = 0b00000100;
    static constexpr uint8_t c_MACON3_FRMLNEN = 0b00000010;
    static constexpr uint8_t c_MACON3_FULDPX  = 0b00000001;

    static constexpr uint8_t c_MICMD_MIISCAN  = 0b00000010;
    static constexpr uint8_t c_MICMD_MIIRD    = 0b00000001;

    enum class PhyAddr : uint8_t
    {
      PHCON1  = 0x00,
      PHSTAT1 = 0x01,
      PHID1   = 0x02,
      PHID2   = 0x03,
      PHCON2  = 0x10,
      PHSTAT2 = 0x11,
      PHIE    = 0x12,
      PHIR    = 0x13,
      PHLCON  = 0x14,
    };

    constexpr uint8_t num(const PhyAddr addr)
    {
      return uint8_t(addr);
    }

    static constexpr uint16_t c_PHCON1_PRST     = 0b1000000000000000;
    static constexpr uint16_t c_PHCON1_PLOOPBK  = 0b0100000000000000;
    static constexpr uint16_t c_PHCON1_PPWRSV   = 0b0000100000000000;
    static constexpr uint16_t c_PHCON1_PDPXMD   = 0b0000000100000000;

    static constexpr uint16_t c_PHSTAT2_TXSTAT  = 0b0010000000000000;
    static constexpr uint16_t c_PHSTAT2_RXSTAT  = 0b0001000000000000;
    static constexpr uint16_t c_PHSTAT2_COLSTAT = 0b0000100000000000;
    static constexpr uint16_t c_PHSTAT2_LSTAT   = 0b0000010000000000;
    static constexpr uint16_t c_PHSTAT2_DPXSTAT = 0b0000001000000000;
    static constexpr uint16_t c_PHSTAT2_PLRITY  = 0b0000000000100000;

    static constexpr uint16_t c_PHIE_PLNKIE     = 0b0000000000010000;
    static constexpr uint16_t c_PHIE_PGEIE      = 0b0000000000000010;
  }

  class Enc28j60Impl : public Enc28j60
  {
  public:
    Enc28j60Impl(const std::shared_ptr<Enc28j60spi>& spi)
      : m_spi(spi)
    {
    }

  protected:
    const std::shared_ptr<Enc28j60spi> m_spi;
    uint8_t m_failureFlags = 0;
    uint8_t m_bank = 0;

  protected:
    static constexpr uint8_t c_RCR = 0b00000000;
    static constexpr uint8_t c_RBM = 0b00111010;
    static constexpr uint8_t c_WCR = 0b01000000;
    static constexpr uint8_t c_WBM = 0b01111010;
    static constexpr uint8_t c_BFS = 0b10000000;
    static constexpr uint8_t c_BFC = 0b10100000;
    static constexpr uint8_t c_SRC = 0b11111111;

  protected:
    uint8_t opRCRE(const uint8_t num)
    {
      if (m_failureFlags)
        return 0;

      uint8_t buf[2];
      buf[0] = c_RCR | num;
      if (m_spi->txrx(buf, sizeof(buf)) != 0)
      {
        m_failureFlags |= 1;
        return 0;
      }
 
      return buf[1];
    }

    uint8_t opRCRM(const uint8_t num)
    {
      if (m_failureFlags)
        return 0;

      uint8_t buf[3];
      buf[0] = c_RCR | num;
      if (m_spi->txrx(buf, sizeof(buf)) != 0)
      {
        m_failureFlags |= 1;
        return 0;
      }
      return buf[2];
    }

    void opRBM(uint8_t* data, const size_t data_len)
    {
      if (m_failureFlags)
        return;

      uint8_t buf[1];
      buf[0] = c_RBM;
      if (m_spi->txThenRx(buf, sizeof(buf), data, data_len) != 0)
      {
        m_failureFlags |= 1;
        return;
      }
    }

    void opWCR(const uint8_t num, const uint8_t val)
    {
      if (m_failureFlags)
        return;

      uint8_t buf[2];
      buf[0] = c_WCR | num;
      buf[1] = val;
      if (m_spi->txrx(buf, sizeof(buf)) != 0)
      {
        m_failureFlags |= 1;
        return;
      }
    }

    void opWBM(const uint8_t* data, const size_t data_len)
    {
      if (m_failureFlags)
        return;

      uint8_t buf[1];
      buf[0] = c_WBM;
      if (m_spi->txThenTx(buf, sizeof(buf), data, data_len) != 0)
      {
        m_failureFlags |= 1;
        return;
      }
    }

    void opBFS(const uint8_t num, const uint8_t val)
    {
      if (m_failureFlags)
        return;

      uint8_t buf[2];
      buf[0] = c_BFS | num;
      buf[1] = val;
      if (m_spi->txrx(buf, sizeof(buf)) != 0)
      {
        m_failureFlags |= 1;
        return;
      }
    }

    void opBFC(const uint8_t num, const uint8_t val)
    {
      if (m_failureFlags)
        return;

      uint8_t buf[2];
      buf[0] = c_BFC | num;
      buf[1] = val;
      if (m_spi->txrx(buf, sizeof(buf)) != 0)
      {
        m_failureFlags |= 1;
        return;
      }
    }

    void opSRC()
    {
      if (m_failureFlags)
        return;

      uint8_t buf[2];
      buf[0] = c_SRC;
      if (m_spi->txrx(buf, sizeof(buf)) != 0)
      {
        m_failureFlags |= 1;
        return;
      }
    }

  protected:
    void setBank(const Reg::Addr addr)
    {
      if (Reg::anyBank(addr))
      {
        return;
      }

      uint8_t bank = Reg::bank(addr);
      if (m_bank == bank)
      {
        return;
      }

      uint8_t clr = m_bank & ~bank;
      uint8_t set = ~m_bank & bank;
      m_bank = bank;
      if (clr)
      {
        opBFC(Reg::num(Reg::Addr::ECON1), clr);
      }

      if (set)
      {
        opBFS(Reg::num(Reg::Addr::ECON1), set);
      }
    }

    uint8_t regRead(const Reg::Addr addr)
    {
      setBank(addr);
      if (Reg::eth(addr))
        return opRCRE(Reg::num(addr));
      else
        return opRCRM(Reg::num(addr));
    }

    uint16_t regRead16(const Reg::Addr addr)
    {
      setBank(addr);
      if (Reg::eth(addr))
        return opRCRE(Reg::num(addr)) | (opRCRE(Reg::num(addr) + 1) << 8);
      else
        return opRCRM(Reg::num(addr)) | (opRCRM(Reg::num(addr) + 1) << 8);
    }

    void regWrite(const Reg::Addr addr, const uint8_t val)
    {
      setBank(addr);
      opWCR(Reg::num(addr), val);
    }

    void regWrite16(const Reg::Addr addr, const uint16_t val)
    {
      setBank(addr);
      opWCR(Reg::num(addr), val & 0xff);
      opWCR(Reg::num(addr) + 1, val >> 8);
    }

    void regSet(const Reg::Addr addr, const uint8_t val)
    {
      setBank(addr);
      opBFS(Reg::num(addr), val);
    }

    void regClr(const Reg::Addr addr, const uint8_t val)
    {
      setBank(addr);
      opBFC(Reg::num(addr), val);
    }

    void memRead(uint8_t* data, const size_t data_len)
    {
      opRBM(data, data_len);
    }

    void memWrite(const uint8_t* data, const size_t data_len)
    {
      opWBM(data, data_len);
    }

    uint16_t phyRead(const Reg::PhyAddr addr)
    {
      regWrite(Reg::Addr::MIREGADR, Reg::num(addr));
      regWrite(Reg::Addr::MICMD, Reg::c_MICMD_MIIRD);
      uint8_t buf[26];            // Need to delay at least 10240ns here.
                                  // Minimum bit period for enc28j60 is 50ns.
                                  // 10240/50 = 204.8 bits = 25.6 bytes
      memRead(buf, sizeof(buf));
      regWrite(Reg::Addr::MICMD, 0);
      return regRead16(Reg::Addr::MIRD16);
    }

    void phyWrite(const Reg::PhyAddr addr, const uint16_t val)
    {
      regWrite(Reg::Addr::MIREGADR, Reg::num(addr));
      regWrite16(Reg::Addr::MIWR16, val);
      uint8_t buf[26];            // Need to delay at least 10240ns here.
                                  // Minimum bit period for enc28j60 is 50ns.
                                  // 10240/50 = 204.8 bits = 25.6 bytes
      memRead(buf, sizeof(buf));
    }

    void reset()
    {
      opSRC();
    }

    void dumpRegs()
    {
    #define DUMP(reg) printf(#reg " = %02x\n", regRead(Reg::Addr::reg))
    #define DUMP16(reg) printf(#reg " = %04x\n", regRead16(Reg::Addr::reg))
    #define DUMP_PHY(reg) printf(#reg " = %04x\n", phyRead(Reg::PhyAddr::reg))

      printf("All banks:\n");
      DUMP(EIE);
      DUMP(EIR);
      DUMP(ESTAT);
      DUMP(ECON2);
      DUMP(ECON1);
      printf("\n");

      printf("Bank 0:\n");
      DUMP16(ERDPT16);
      DUMP16(EWRPT16);
      DUMP16(ETXST16);
      DUMP16(ETXND16);
      DUMP16(ERXST16);
      DUMP16(ERXND16);
      DUMP16(ERXRDPT16);
      DUMP16(ERXWRPT16);
      DUMP16(EDMAST16);
      DUMP16(EDMAND16);
      DUMP16(EDMADST16);
      DUMP16(EDMACS16);
      printf("\n");

      printf("Bank 1:\n");
      DUMP(EHT0);
      DUMP(EHT1);
      DUMP(EHT2);
      DUMP(EHT3);
      DUMP(EHT4);
      DUMP(EHT5);
      DUMP(EHT6);
      DUMP(EHT7);
      DUMP(EPMM0);
      DUMP(EPMM1);
      DUMP(EPMM2);
      DUMP(EPMM3);
      DUMP(EPMM4);
      DUMP(EPMM5);
      DUMP(EPMM6);
      DUMP(EPMM7);
      DUMP16(EPMCS16);
      DUMP16(EPMO16);
      DUMP(ERXFCON);
      DUMP(EPKTCNT);
      printf("\n");

      printf("Bank 2:\n");
      DUMP(MACON1);
      DUMP(MACON3);
      DUMP(MACON4);
      DUMP(MABBIPG);
      DUMP16(MAIPG16);
      DUMP(MACLCON1);
      DUMP(MACLCON2);
      DUMP16(MAMXFL16);
      DUMP(MICMD);
      DUMP(MIREGADR);
      DUMP16(MIWR16);
      DUMP16(MIRD16);
      printf("\n");

      printf("Bank 3:\n");
      DUMP(MAADR5);
      DUMP(MAADR6);
      DUMP(MAADR3);
      DUMP(MAADR4);
      DUMP(MAADR1);
      DUMP(MAADR2);
      DUMP(EBSTSD);
      DUMP(EBSTCON);
      DUMP16(EBSTCS16);
      DUMP(MISTAT);
      DUMP(EREVID);
      DUMP(ECOCON);
      DUMP(EFLOCON);
      DUMP16(EPAUS16);
      printf("\n");

      printf("PHY:\n");
      DUMP_PHY(PHCON1);
      DUMP_PHY(PHSTAT1);
      DUMP_PHY(PHID1);
      DUMP_PHY(PHID2);
      DUMP_PHY(PHCON2);
      DUMP_PHY(PHSTAT2);
      DUMP_PHY(PHIE);
      DUMP_PHY(PHIR);
      DUMP_PHY(PHLCON);
      printf("\n");

    #undef DUMP
    #undef DUMP16
    #undef DUMP_PHY
    }

    void dumpState()
    {
    #define DUMP(reg) printf(#reg " = %01x\n", (val & Reg::c_##reg) ? 1 : 0)

      uint8_t val;

      val = regRead(Reg::Addr::EIR);
      DUMP(EIR_PKTIF);
      DUMP(EIR_DMAIF);
      DUMP(EIR_LINKIF);
      DUMP(EIR_TXIF);
      DUMP(EIR_TXERIF);
      DUMP(EIR_RXERIF);

      val = regRead(Reg::Addr::ESTAT);
      DUMP(ESTAT_INT);
      DUMP(ESTAT_BUFER);
      DUMP(ESTAT_LATECOL);
      DUMP(ESTAT_RXBUSY);
      DUMP(ESTAT_TXABRT);
      DUMP(ESTAT_CLKRDY);

      val = regRead(Reg::Addr::ECON1);
      DUMP(ECON1_TXRTS);

      val = regRead(Reg::Addr::ECON2);
      DUMP(ECON2_PWRSV);
      DUMP(ECON2_VRPS);

      printf("\n");

    #undef DUMP
    }

    static uint32_t updateCrc(const uint32_t crc, const uint8_t byte)
    {
      uint32_t result = crc;
      result = result ^ byte;
      result = ((result & 1) ? 0xedB88320 : 0) ^ (result >> 1);
      result = ((result & 1) ? 0xedB88320 : 0) ^ (result >> 1);
      result = ((result & 1) ? 0xedB88320 : 0) ^ (result >> 1);
      result = ((result & 1) ? 0xedB88320 : 0) ^ (result >> 1);
      result = ((result & 1) ? 0xedB88320 : 0) ^ (result >> 1);
      result = ((result & 1) ? 0xedB88320 : 0) ^ (result >> 1);
      result = ((result & 1) ? 0xedB88320 : 0) ^ (result >> 1);
      result = ((result & 1) ? 0xedB88320 : 0) ^ (result >> 1);
      return result;
    }

    int test1()
    {
      uint32_t crc = 0xffffffff;
      for (uint8_t i = 0; i <= 0x17; ++i)
      {
        crc = updateCrc(crc, opRCRE(i));
      }
      if (~crc != 0xeebd948d)
      {
        return 1;
      }
      return 0;
    }

    int test2()
    {
      static const Reg::Addr addrs[] = {
        Reg::Addr::ERDPT16,
        Reg::Addr::EWRPT16,
        Reg::Addr::ETXST16,
        Reg::Addr::ETXND16,
        Reg::Addr::ERXST16,
        Reg::Addr::ERXND16,
        Reg::Addr::ERXRDPT16,
        Reg::Addr::EDMAST16,
        Reg::Addr::EDMAND16,
        Reg::Addr::EDMADST16,
      };
      uint32_t crc = 0xffffffff;
      for (size_t i = 0; i < std::extent<decltype(addrs)>::value; ++i)
      {
        crc = updateCrc(crc, i);
        regWrite16(addrs[i], crc & 0x1fff);
      }
      crc = 0xffffffff;
      for (size_t i = 0; i < std::extent<decltype(addrs)>::value; ++i)
      {
        crc = updateCrc(crc, i);
        if (regRead16(addrs[i]) != (crc & 0x1fff))
        {
          return 1;
        }
      }
      return 0;
    }

    int test3()
    {
      if (regRead(Reg::Addr::EREVID) != 6 || phyRead(Reg::PhyAddr::PHID1) != 0x0083 || phyRead(Reg::PhyAddr::PHID2) != 0x1400)
      {
        return 1;
      }
      return 0;
    }

    void validate()
    {
      printf("test1 returned %i\n", test1());
      printf("test2 returned %i\n", test2());
      printf("test3 returned %i\n", test3());
    }

    unsigned benchmark(const char* name, void(Enc28j60Impl::*test_fn)(void* ctx), void* ctx, unsigned offset)
    {
      printf("Benchmarking %s...\n", name);
      TickType_t start = xTaskGetTickCount();
      while (start == xTaskGetTickCount());
      start = xTaskGetTickCount();
      TickType_t finish = start + 1000 / portTICK_PERIOD_MS;
      unsigned count = 0;
      while (xTaskGetTickCount() < finish)
      {
        (this->*test_fn)(ctx);
        ++count;
      }
      finish = xTaskGetTickCount();
      unsigned duration_ns = (finish - start) * portTICK_PERIOD_MS * 1000000;
      unsigned cycle_ns = (duration_ns + count / 2) / count - offset;
      unsigned duration_clk = (finish - start) * portTICK_PERIOD_MS * ((SystemCoreClock + 500) / 1000);
      unsigned offset_clk = (offset * ((SystemCoreClock + 500) / 1000) + 500000) / 1000000;
      unsigned cycle_clk = (duration_clk + count / 2) / count - offset_clk;
      printf("               ... cycle = %u ns = %u CLKs\n", cycle_ns, cycle_clk);
      return cycle_ns;
    }

    void benchmarkNull(void* /*ctx*/)
    {
    }

    void benchmarkRxtx(void* ctx)
    {
      static uint8_t buf[1000];
      m_spi->txrx(buf, (size_t)ctx);
    }

    void benchmarkMemRead(void* ctx)
    {
      static uint8_t buf[1000];
      memRead(buf, (size_t)ctx);
    }

    void benchmarkMemWrite(void* ctx)
    {
      static uint8_t buf[1000];
      memWrite(buf, (size_t)ctx);
    }

    void benchmarkPhyRead(void* ctx)
    {
      phyRead(Reg::PhyAddr::PHLCON);
    }

    void benchmarkPhyWrite(void* ctx)
    {
      phyWrite(Reg::PhyAddr::PHLCON, 0x1234);
    }

    void benchmark_all()
    {
      unsigned offset = benchmark("null", &Enc28j60Impl::benchmarkNull, 0, 0);
      benchmark("rxtx 1", &Enc28j60Impl::benchmarkRxtx, (void*)1, offset);
      benchmark("rxtx 2", &Enc28j60Impl::benchmarkRxtx, (void*)2, offset);
      benchmark("rxtx 3", &Enc28j60Impl::benchmarkRxtx, (void*)3, offset);
      benchmark("memRead 1000", &Enc28j60Impl::benchmarkMemRead, (void*)1000, offset);
      benchmark("memRead 100", &Enc28j60Impl::benchmarkMemRead, (void*)100, offset);
      benchmark("memRead 10", &Enc28j60Impl::benchmarkMemRead, (void*)10, offset);
      benchmark("memWrite 1000", &Enc28j60Impl::benchmarkMemWrite, (void*)1000, offset);
      benchmark("memWrite 100", &Enc28j60Impl::benchmarkMemWrite, (void*)100, offset);
      benchmark("memWrite 10", &Enc28j60Impl::benchmarkMemWrite, (void*)10, offset);
      benchmark("phyRead", &Enc28j60Impl::benchmarkPhyRead, 0, offset);
      benchmark("phyWrite", &Enc28j60Impl::benchmarkPhyWrite, 0, offset);
    }

  public:
    virtual void test() override
    {
      regClr(Reg::Addr::ECON2, Reg::c_ECON2_PWRSV);
      OS::ExpirationTimer::delay(1);
      opSRC();
      OS::ExpirationTimer::delay(1);
      validate();
      dumpRegs();
      //dumpRegs();
      //dumpState();
      regRead(Reg::Addr::ERDPT16);
      benchmark_all();
    }
  };
}

std::unique_ptr<Enc28j60> CreateEnc28j60(const std::shared_ptr<Enc28j60spi>& spi)
{
  return std::make_unique<Enc28j60Impl>(spi);
}
