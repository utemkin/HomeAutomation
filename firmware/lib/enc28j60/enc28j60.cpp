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

#define RCR               (0b00000000)
#define RBM               (0b00111010)
#define WCR               (0b01000000)
#define WBM               (0b01111010)
#define BFS               (0b10000000)
#define BFC               (0b10100000)
#define SRC               (0b11111111)

static uint8_t op_RCR_E(struct enc28j60_impl* enc_impl, uint8_t num)
{
  if (enc_impl->failure_flags)
  {
    return 0;
  }
  enc_impl->buf2[0] = RCR | num;
  if (enc_impl->spi->txrx(enc_impl->spi, enc_impl->buf2, sizeof(enc_impl->buf2)) != 0)
  {
    enc_impl->failure_flags |= 1;
    return 0;
  }
  return enc_impl->buf2[1];
}

static uint8_t op_RCR_M(struct enc28j60_impl* enc_impl, uint8_t num)
{
  if (enc_impl->failure_flags)
  {
    return 0;
  }
  enc_impl->buf3[0] = RCR | num;
  if (enc_impl->spi->txrx(enc_impl->spi, enc_impl->buf3, sizeof(enc_impl->buf3)) != 0)
  {
    enc_impl->failure_flags |= 1;
    return 0;
  }
  return enc_impl->buf3[2];
}

static void op_RBM(struct enc28j60_impl* enc_impl, uint8_t* data, size_t data_len)
{
  if (enc_impl->failure_flags)
  {
    return;
  }
  enc_impl->buf1[0] = RBM;
  if (enc_impl->spi->tx_then_rx(enc_impl->spi, enc_impl->buf1, sizeof(enc_impl->buf1), data, data_len) != 0)
  {
    enc_impl->failure_flags |= 1;
    return;
  }
}

static void op_WCR(struct enc28j60_impl* enc_impl, uint8_t num, uint8_t val)
{
  if (enc_impl->failure_flags)
  {
    return;
  }
  enc_impl->buf2[0] = WCR | num;
  enc_impl->buf2[1] = val;
  if (enc_impl->spi->txrx(enc_impl->spi, enc_impl->buf2, sizeof(enc_impl->buf2)) != 0)
  {
    enc_impl->failure_flags |= 1;
    return;
  }
}

static void op_WBM(struct enc28j60_impl* enc_impl, const uint8_t* data, size_t data_len)
{
  if (enc_impl->failure_flags)
  {
    return;
  }
  enc_impl->buf1[0] = WBM;
  if (enc_impl->spi->tx_then_tx(enc_impl->spi, enc_impl->buf1, sizeof(enc_impl->buf1), data, data_len) != 0)
  {
    enc_impl->failure_flags |= 1;
    return;
  }
}

static void op_BFS(struct enc28j60_impl* enc_impl, uint8_t num, uint8_t val)
{
  if (enc_impl->failure_flags)
  {
    return;
  }
  enc_impl->buf2[0] = BFS | num;
  enc_impl->buf2[1] = val;
  if (enc_impl->spi->txrx(enc_impl->spi, enc_impl->buf2, sizeof(enc_impl->buf2)) != 0)
  {
    enc_impl->failure_flags |= 1;
    return;
  }
}

static void op_BFC(struct enc28j60_impl* enc_impl, uint8_t num, uint8_t val)
{
  if (enc_impl->failure_flags)
  {
    return;
  }
  enc_impl->buf2[0] = BFC | num;
  enc_impl->buf2[1] = val;
  if (enc_impl->spi->txrx(enc_impl->spi, enc_impl->buf2, sizeof(enc_impl->buf2)) != 0)
  {
    enc_impl->failure_flags |= 1;
    return;
  }
}

static void op_SRC(struct enc28j60_impl* enc_impl)
{
  if (enc_impl->failure_flags)
  {
    return;
  }
  enc_impl->buf1[0] = SRC;
  if (enc_impl->spi->txrx(enc_impl->spi, enc_impl->buf1, sizeof(enc_impl->buf1)) != 0)
  {
    enc_impl->failure_flags |= 1;
    return;
  }
}

#define REG_BANK_SHIFT    (6)
#define REG_E_MASK        (0x20)
#define REG_NUM_MASK      (0x1f)
#define REG_E(bank, num)  (((bank) << REG_BANK_SHIFT) | (num) | REG_E_MASK)
#define REG_M(bank, num)  (((bank) << REG_BANK_SHIFT) | (num))

#define EIE               REG_E(3, 0x1b)
#define EIE_INTIE           (0b10000000)
#define EIE_PKTIE           (0b01000000)
#define EIE_DMAIE           (0b00100000)
#define EIE_LINKIE          (0b00010000)
#define EIE_TXIE            (0b00001000)
#define EIE_TXERIE          (0b00000010)
#define EIE_RXERIE          (0b00000001)
#define EIR               REG_E(3, 0x1c)
#define EIR_PKTIF           (0b01000000)
#define EIR_DMAIF           (0b00100000)
#define EIR_LINKIF          (0b00010000)
#define EIR_TXIF            (0b00001000)
#define EIR_TXERIF          (0b00000010)
#define EIR_RXERIF          (0b00000001)
#define ESTAT             REG_E(3, 0x1d)
#define ESTAT_INT           (0b10000000)
#define ESTAT_BUFER         (0b01000000)
#define ESTAT_LATECOL       (0b00010000)
#define ESTAT_RXBUSY        (0b00000100)
#define ESTAT_TXABRT        (0b00000010)
#define ESTAT_CLKRDY        (0b00000001)
#define ECON2             REG_E(3, 0x1e)
#define ECON2_AUTOINC       (0b10000000)
#define ECON2_PKTDEC        (0b01000000)
#define ECON2_PWRSV         (0b00100000)
#define ECON2_VRPS          (0b00001000)
#define ECON1             REG_E(3, 0x1f)
#define ECON1_TXRST         (0b10000000)
#define ECON1_RXRST         (0b01000000)
#define ECON1_DMAST         (0b00100000)
#define ECON1_CSUMEN        (0b00010000)
#define ECON1_TXRTS         (0b00001000)
#define ECON1_RXEN          (0b00000100)
#define ECON1_BSEL1         (0b00000010)
#define ECON1_BSEL0         (0b00000001)

#define ERDPT16           REG_E(0, 0x00)
#define EWRPT16           REG_E(0, 0x02)
#define ETXST16           REG_E(0, 0x04)
#define ETXND16           REG_E(0, 0x06)
#define ERXST16           REG_E(0, 0x08)
#define ERXND16           REG_E(0, 0x0a)
#define ERXRDPT16         REG_E(0, 0x0c)
#define ERXWRPT16         REG_E(0, 0x0e)
#define EDMAST16          REG_E(0, 0x10)
#define EDMAND16          REG_E(0, 0x12)
#define EDMADST16         REG_E(0, 0x14)
#define EDMACS16          REG_E(0, 0x16)

#define EHT0              REG_E(1, 0x00)
#define EHT1              REG_E(1, 0x01)
#define EHT2              REG_E(1, 0x02)
#define EHT3              REG_E(1, 0x03)
#define EHT4              REG_E(1, 0x04)
#define EHT5              REG_E(1, 0x05)
#define EHT6              REG_E(1, 0x06)
#define EHT7              REG_E(1, 0x07)
#define EPMM0             REG_E(1, 0x08)
#define EPMM1             REG_E(1, 0x09)
#define EPMM2             REG_E(1, 0x0a)
#define EPMM3             REG_E(1, 0x0b)
#define EPMM4             REG_E(1, 0x0c)
#define EPMM5             REG_E(1, 0x0d)
#define EPMM6             REG_E(1, 0x0e)
#define EPMM7             REG_E(1, 0x0f)
#define EPMCS16           REG_E(1, 0x10)
#define EPMO16            REG_E(1, 0x14)
#define ERXFCON           REG_E(1, 0x18)
#define EPKTCNT           REG_E(1, 0x19)

#define MACON1            REG_M(2, 0x00)
#define MACON1_TXPAUS       (0b00001000)
#define MACON1_RXPAUS       (0b00000100)
#define MACON1_PASSALL      (0b00000010)
#define MACON1_MARXEN       (0b00000001)
#define MACON3            REG_M(2, 0x02)
#define MACON3_PADCFG2      (0b10000000)
#define MACON3_PADCFG1      (0b01000000)
#define MACON3_PADCFG0      (0b00100000)
#define MACON3_TXCRCEN      (0b00010000)
#define MACON3_PHDREN       (0b00001000)
#define MACON3_HFRMEN       (0b00000100)
#define MACON3_FRMLNEN      (0b00000010)
#define MACON3_FULDPX       (0b00000001)
#define MACON4            REG_M(2, 0x03)
#define MABBIPG           REG_M(2, 0x04)
#define MAIPG16           REG_M(2, 0x06)
#define MACLCON1          REG_M(2, 0x08)
#define MACLCON2          REG_M(2, 0x09)
#define MAMXFL16          REG_M(2, 0x0a)
#define MICMD             REG_M(2, 0x12)
#define MICMD_MIISCAN       (0b00000010)
#define MICMD_MIIRD         (0b00000001)
#define MIREGADR          REG_M(2, 0x14)
#define MIWR16            REG_M(2, 0x16)
#define MIRD16            REG_M(2, 0x18)

#define MAADR5            REG_M(3, 0x00)
#define MAADR6            REG_M(3, 0x01)
#define MAADR3            REG_M(3, 0x02)
#define MAADR4            REG_M(3, 0x03)
#define MAADR1            REG_M(3, 0x04)
#define MAADR2            REG_M(3, 0x05)
#define EBSTSD            REG_E(3, 0x06)
#define EBSTCON           REG_E(3, 0x07)
#define EBSTCS16          REG_E(3, 0x08)
#define MISTAT            REG_M(3, 0x0a)
#define EREVID            REG_E(3, 0x12)
#define ECOCON            REG_E(3, 0x15)
#define EFLOCON           REG_E(3, 0x17)
#define EPAUS16           REG_E(3, 0x18)

#define PHCON1            (0x00)
#define PHCON1_PRST         (0b1000000000000000)
#define PHCON1_PLOOPBK      (0b0100000000000000)
#define PHCON1_PPWRSV       (0b0000100000000000)
#define PHCON1_PDPXMD       (0b0000000100000000)
#define PHSTAT1           (0x01)
#define PHID1             (0x02)
#define PHID2             (0x03)
#define PHCON2            (0x10)
#define PHSTAT2           (0x11)
#define PHSTAT2_TXSTAT      (0b0010000000000000)
#define PHSTAT2_RXSTAT      (0b0001000000000000)
#define PHSTAT2_COLSTAT     (0b0000100000000000)
#define PHSTAT2_LSTAT       (0b0000010000000000)
#define PHSTAT2_DPXSTAT     (0b0000001000000000)
#define PHSTAT2_PLRITY      (0b0000000000100000)
#define PHIE              (0x12)
#define PHIE_PLNKIE         (0b0000000000010000)
#define PHIE_PGEIE          (0b0000000000000010)
#define PHIR              (0x13)
#define PHLCON            (0x14)

static void set_bank(struct enc28j60_impl* enc_impl, uint8_t reg)
{
  if (reg >= EIE)
  {
    return;
  }
  uint8_t bank = reg >> REG_BANK_SHIFT;
  if (enc_impl->bank == bank)
  {
    return;
  }
  uint8_t clr = enc_impl->bank & ~bank;
  uint8_t set = ~enc_impl->bank & bank;
  enc_impl->bank = bank;
  if (clr)
  {
    op_BFC(enc_impl, ECON1 & REG_NUM_MASK, clr);
  }
  if (set)
  {
    op_BFS(enc_impl, ECON1 & REG_NUM_MASK, set);
  }
}

static uint8_t reg_read(struct enc28j60_impl* enc_impl, uint8_t reg)
{
  set_bank(enc_impl, reg);
  return (reg & REG_E_MASK ? op_RCR_E : op_RCR_M)(enc_impl, reg & REG_NUM_MASK);
}

static uint16_t reg_read16(struct enc28j60_impl* enc_impl, uint8_t reg)
{
  set_bank(enc_impl, reg);
  if (reg & REG_E_MASK)
  {
    reg &= REG_NUM_MASK;
    return op_RCR_E(enc_impl, reg) | (op_RCR_E(enc_impl, reg + 1) << 8);
  }
  else
  {
    reg &= REG_NUM_MASK;
    return op_RCR_M(enc_impl, reg) | (op_RCR_M(enc_impl, reg + 1) << 8);
  }
}

static void reg_write(struct enc28j60_impl* enc_impl, uint8_t reg, uint8_t val)
{
  set_bank(enc_impl, reg);
  op_WCR(enc_impl, reg & REG_NUM_MASK, val);
}

static void reg_write16(struct enc28j60_impl* enc_impl, uint8_t reg, uint16_t val)
{
  set_bank(enc_impl, reg);
  reg &= REG_NUM_MASK;
  op_WCR(enc_impl, reg, val & 0xff);
  op_WCR(enc_impl, reg + 1, val >> 8);
}

static void reg_set(struct enc28j60_impl* enc_impl, uint8_t reg, uint8_t val)
{
  set_bank(enc_impl, reg);
  op_BFS(enc_impl, reg & REG_NUM_MASK, val);
}

static void reg_clr(struct enc28j60_impl* enc_impl, uint8_t reg, uint8_t val)
{
  set_bank(enc_impl, reg);
  op_BFC(enc_impl, reg & REG_NUM_MASK, val);
}

static void mem_read(struct enc28j60_impl* enc_impl, uint8_t* data, size_t data_len)
{
  op_RBM(enc_impl, data, data_len);
}

static void mem_write(struct enc28j60_impl* enc_impl, const uint8_t* data, size_t data_len)
{
  op_WBM(enc_impl, data, data_len);
}

static uint16_t phy_read(struct enc28j60_impl* enc_impl, uint8_t reg)
{
  reg_write(enc_impl, MIREGADR, reg);
  reg_write(enc_impl, MICMD, MICMD_MIIRD);
  mem_read(enc_impl, enc_impl->buf_wait_phy, sizeof(enc_impl->buf_wait_phy));
  reg_write(enc_impl, MICMD, 0);
  return reg_read16(enc_impl, MIRD16);
}

static void phy_write(struct enc28j60_impl* enc_impl, uint8_t reg, uint16_t val)
{
  reg_write(enc_impl, MIREGADR, reg);
  reg_write16(enc_impl, MIWR16, val);
  mem_read(enc_impl, enc_impl->buf_wait_phy, sizeof(enc_impl->buf_wait_phy));
}

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

static void dump_regs(struct enc28j60_impl* enc_impl)
{
#define DUMP(reg) ENC28J60_printf(#reg " = %02x\n", reg_read(enc_impl, reg))
#define DUMP16(reg) ENC28J60_printf(#reg " = %04x\n", reg_read16(enc_impl, reg))
#define DUMP_PHY(reg) ENC28J60_printf(#reg " = %04x\n", phy_read(enc_impl, reg))

  ENC28J60_printf("All banks:\n");
  DUMP(EIE);
  DUMP(EIR);
  DUMP(ESTAT);
  DUMP(ECON2);
  DUMP(ECON1);
  ENC28J60_printf("\n");

  ENC28J60_printf("Bank 0:\n");
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
  ENC28J60_printf("\n");

  ENC28J60_printf("Bank 1:\n");
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
  ENC28J60_printf("\n");

  ENC28J60_printf("Bank 2:\n");
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
  ENC28J60_printf("\n");

  ENC28J60_printf("Bank 3:\n");
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
  ENC28J60_printf("\n");

  ENC28J60_printf("PHY:\n");
  DUMP_PHY(PHCON1);
  DUMP_PHY(PHSTAT1);
  DUMP_PHY(PHID1);
  DUMP_PHY(PHID2);
  DUMP_PHY(PHCON2);
  DUMP_PHY(PHSTAT2);
  DUMP_PHY(PHIE);
  DUMP_PHY(PHIR);
  DUMP_PHY(PHLCON);
  ENC28J60_printf("\n");

#undef DUMP
#undef DUMP16
#undef DUMP_PHY
}

static void dump_state(struct enc28j60_impl* enc_impl)
{
#define DUMP(reg) ENC28J60_printf(#reg " = %01x\n", (val & reg) ? 1 : 0)

  uint8_t val;

  val = reg_read(enc_impl, EIR);
  DUMP(EIR_PKTIF);
  DUMP(EIR_DMAIF);
  DUMP(EIR_LINKIF);
  DUMP(EIR_TXIF);
  DUMP(EIR_TXERIF);
  DUMP(EIR_RXERIF);

  val = reg_read(enc_impl, ESTAT);
  DUMP(ESTAT_INT);
  DUMP(ESTAT_BUFER);
  DUMP(ESTAT_LATECOL);
  DUMP(ESTAT_RXBUSY);
  DUMP(ESTAT_TXABRT);
  DUMP(ESTAT_CLKRDY);

  val = reg_read(enc_impl, ECON1);
  DUMP(ECON1_TXRTS);

  val = reg_read(enc_impl, ECON2);
  DUMP(ECON2_PWRSV);
  DUMP(ECON2_VRPS);

  ENC28J60_printf("\n");

#undef DUMP
}

static unsigned benchmark(const char* name, void(*test_fn)(struct enc28j60_impl* enc_impl, void* ctx), struct enc28j60_impl* enc, void* ctx, unsigned offset)
{
  ENC28J60_printf("Benchmarking %s...\n", name);
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
  ENC28J60_printf("               ... cycle = %u ns = %u CLKs\n", cycle_ns, cycle_clk);
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

static uint32_t update_crc(uint32_t crc, uint8_t byte)
{
  crc = crc ^ byte;
  crc = ((crc & 1) ? 0xedB88320 : 0) ^ (crc >> 1);
  crc = ((crc & 1) ? 0xedB88320 : 0) ^ (crc >> 1);
  crc = ((crc & 1) ? 0xedB88320 : 0) ^ (crc >> 1);
  crc = ((crc & 1) ? 0xedB88320 : 0) ^ (crc >> 1);
  crc = ((crc & 1) ? 0xedB88320 : 0) ^ (crc >> 1);
  crc = ((crc & 1) ? 0xedB88320 : 0) ^ (crc >> 1);
  crc = ((crc & 1) ? 0xedB88320 : 0) ^ (crc >> 1);
  crc = ((crc & 1) ? 0xedB88320 : 0) ^ (crc >> 1);
  return crc;
}

static int test1(struct enc28j60_impl* enc_impl)
{
  uint32_t crc = 0xffffffff;
  for (int i = 0; i <= 0x17; ++i)
  {
    crc = update_crc(crc, op_RCR_E(enc_impl, i));
  }
  if (~crc != 0xeebd948d)
  {
    return 1;
  }
  return 0;
}

static int test2(struct enc28j60_impl* enc_impl)
{
  static const uint8_t reg[] = {
      ERDPT16,
      EWRPT16,
      ETXST16,
      ETXND16,
      ERXST16,
      ERXND16,
      ERXRDPT16,
      EDMAST16,
      EDMAND16,
      EDMADST16};
  uint32_t crc = 0xffffffff;
  for (int i = 0; i < sizeof(reg)/sizeof(reg[0]); ++i)
  {
    crc = update_crc(crc, i);
    reg_write16(enc_impl, reg[i], crc & 0x1fff);
  }
  crc = 0xffffffff;
  for (int i = 0; i < sizeof(reg)/sizeof(reg[0]); ++i)
  {
    crc = update_crc(crc, i);
    if (reg_read16(enc_impl, reg[i]) != (crc & 0x1fff))
    {
      return 1;
    }
  }
  return 0;
}

static int test3(struct enc28j60_impl* enc_impl)
{
  if (reg_read(enc_impl, EREVID) != 6 || phy_read(enc_impl, PHID1) != 0x0083 || phy_read(enc_impl, PHID2) != 0x1400)
  {
    return 1;
  }
  return 0;
}

static void validate(struct enc28j60_impl* enc_impl)
{
  ENC28J60_printf("test1 returned %i\n", test1(enc_impl));
  ENC28J60_printf("test2 returned %i\n", test2(enc_impl));
  ENC28J60_printf("test3 returned %i\n", test3(enc_impl));
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
      ENC28J60_printf("link pulsed down then up\n");
      enc_impl->lstat = 0;
      enc_impl->lstat = 1;
    }
    else
    {
      //fixme
      ENC28J60_printf("link up\n");
      enc_impl->lstat = 1;
    }
  }
  else
  {
    if (enc_impl->lstat)
    {
      //fixme
      ENC28J60_printf("link down\n");
      enc_impl->lstat = 0;
    }
    else
    {
      //fixme
      ENC28J60_printf("link pulsed up then down\n");
    }
  }
}

static void check_rx(struct enc28j60_impl* enc_impl, uint8_t eir)
{
  if (eir & EIR_RXERIF)
  {
    //fixme
    ENC28J60_printf("buffer overflow\n");
  }
  while (reg_read(enc_impl, EPKTCNT))
  {
    reg_set(enc_impl, ECON2, ECON2_PKTDEC);
    reg_write16(enc_impl, ERDPT16, enc_impl->next_packet);
    uint8_t rsv[6];
    mem_read(enc_impl, rsv, sizeof(rsv));
    uint16_t next_packet = rsv[0] | (rsv[1] << 8);
    uint16_t len = rsv[2] | (rsv[3] << 8);
    ENC28J60_printf("packet received %u\n", len);
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
    ENC28J60_printf("%02x ", byte);
  }
  ENC28J60_printf("\n");
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

class Enc28j60Impl : public Enc28j60
{
public:
  Enc28j60Impl(const std::shared_ptr<Enc28j60spi>& spi)
    : m_spi(spi)
  {
  }

protected:
  const std::shared_ptr<Enc28j60spi> m_spi;
};

std::unique_ptr<Enc28j60> CreateEnc28j60(const std::shared_ptr<Enc28j60spi>& spi)
{
  return std::make_unique<Enc28j60Impl>(spi);
}
