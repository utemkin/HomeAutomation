#include <enc28j60/enc28j60.h>
#include <type_traits>

/*
Driver limitations:
1. Only supports ENC28J60 silicon revision B7
2. Only supports full duplex

Used specifications:
1. Datasheet DS39662E Revision E (November 2012)
2. Errata DS80349C Rev C Document (07/2010)

Errata issues:

Issue 1. Module: MAC Interface
irrelevant - does not affect B7

Issue 2. Module: Reset
relevant - see comment 1

Issue 3. Module: Core (Operating Specifications)
irrelevant - does not affect B7

Issue 4. Module: Oscillator (CLKOUT Pin)
irrelevant - CLKOUT feature not used

Issue 5. Module: Memory (Ethernet Buffer)
relevant - see comment 2

Issue 6. Module: Interrupts
relevant - see comment 3

Issue 7. Module: PHY
relevant - check hardware

Issue 8. Module: PHY
relevant - check hardware

Issue 9. Module: PHY
irrelevant - loopback feature not used

Issue 10. Module: PHY
irrelevant - loopback feature not used

Issue 11. Module: PHY LEDs
relevant - see comment 4

Issue 12. Module: Transmit Logic
relevant - see comment 5

Issue 13. Module: PHY
irrelevant - Half-Duplex feature not used

Issue 14. Module: Memory (Ethernet Buffer)
relevant - see comment 6

Issue 15. Module: Transmit Logic
irrelevant - Half-Duplex feature not used

Issue 16. Module: PHY LEDs
relevant - check hardware; force PHCON1.PDPXMD state

Issue 17. Module: DMA
irrelevant - DMA feature not used

Issue 18. Module: Receive Filter
irrelevant - Pattern Match feature not used

Issue 19. Module: SPI Interface
relevant - see comment 1

Comment 1.
Special reset procedure should be used (consider using separate pin for hardware reset)

Comment 2.
Locate RX buffer at zero offset

Comment 3.
Don't rely on EIR.PKTIF value. Use EPKTCNT to detect if there are packets received

Comment 4.
Don't use that specific LED configuration

Comment 5.
Special packet send procedure should be used

Comment 6.
Special procedure should be used to set/update ERXRDPT
*/

namespace Enc28j60
{
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

    class DeviceImpl : public Device
    {
    public:
      DeviceImpl(std::unique_ptr<Env>&& env, std::unique_ptr<Spi>&& spi, const MacAddress& mac)
        : m_env(std::move(env))
        , m_spi(std::move(spi))
        , m_mac(mac)
      {
      }

    protected:
      const std::unique_ptr<Env> m_env;
      const std::unique_ptr<Spi> m_spi;
      const MacAddress m_mac;
      uint8_t m_failureFlags = 0;
      uint8_t m_bank;
      uint16_t m_nextPacket;
      int m_phase = 0;
      OS::ExpirationTimer m_timer;
      bool m_reportedUplink = false;
      bool m_txEnabled = false;
      bool m_txInProgress;

    protected:
      static constexpr uint8_t c_RCR = 0b00000000;
      static constexpr uint8_t c_RBM = 0b00111010;
      static constexpr uint8_t c_WCR = 0b01000000;
      static constexpr uint8_t c_WBM = 0b01111010;
      static constexpr uint8_t c_BFS = 0b10000000;
      static constexpr uint8_t c_BFC = 0b10100000;
      static constexpr uint8_t c_SRC = 0b11111111;

    protected:
      static constexpr uint16_t c_MaxFrameLength  = 1522;
      static constexpr uint16_t c_RxBufferStart   = 0x0000;
      static constexpr uint16_t c_RxBufferEnd     = 0x19ff;
      static constexpr uint16_t c_TxBufferStart   = 0x1a00;
      static constexpr uint16_t c_TxBufferEnd     = 0x1fff;
      static_assert(c_RxBufferStart == 0);                    //See comment 2

    protected:
      uint8_t opRCRE(const uint8_t num)
      {
        if (m_failureFlags)
          return 0;

        uint8_t buf[2];
        buf[0] = c_RCR | num;
        if (m_spi->txRx(buf, sizeof(buf)) != 0)
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
        if (m_spi->txRx(buf, sizeof(buf)) != 0)
        {
          m_failureFlags |= 1;
          return 0;
        }
        return buf[2];
      }

      void opRBM(uint8_t* const data, const size_t data_len)
      {
        if (m_failureFlags)
          return;

        if (m_spi->txThenRx(c_RBM, data, data_len) != 0)
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
        if (m_spi->txRx(buf, sizeof(buf)) != 0)
        {
          m_failureFlags |= 1;
          return;
        }
      }

      void opWBM(const uint8_t* const data, const size_t data_len)
      {
        if (m_failureFlags)
          return;

        if (m_spi->txThenTx(c_WBM, data, data_len) != 0)
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
        if (m_spi->txRx(buf, sizeof(buf)) != 0)
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
        if (m_spi->txRx(buf, sizeof(buf)) != 0)
        {
          m_failureFlags |= 1;
          return;
        }
      }

      void opSRC()
      {
        if (m_failureFlags)
          return;

        uint8_t buf[1];
        buf[0] = c_SRC;
        if (m_spi->txRx(buf, sizeof(buf)) != 0)
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

      void regWrite16(const Reg::Addr addr, uint16_t val)
      {
        if (addr == Reg::Addr::ERXRDPT16)
          val = int(val) - 1 >= c_RxBufferStart ? val - 1 : c_RxBufferEnd;  //See comment 6

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

      void memRead(uint8_t* const data, const size_t data_len)
      {
        opRBM(data, data_len);
      }

      void memWrite(const uint8_t* const data, const size_t data_len)
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

        uint8_t savedECON1 = regRead(Reg::Addr::ECON1);
        uint8_t savedBank = m_bank;

        uint16_t savedERDPT16 = regRead16(Reg::Addr::ERDPT16);
        uint8_t savedMIREGADR = regRead(Reg::Addr::MIREGADR);

        regWrite(Reg::Addr::ECON1, savedECON1);
        m_bank = savedBank;

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
        //DUMP_PHY(PHIR); this read would affect LINKIF, PGIF and PLNKIF flags
        DUMP_PHY(PHLCON);
        printf("\n");

        regWrite16(Reg::Addr::ERDPT16, savedERDPT16);
        regWrite(Reg::Addr::MIREGADR, savedMIREGADR);

        regWrite(Reg::Addr::ECON1, savedECON1);
        m_bank = savedBank;

      #undef DUMP
      #undef DUMP16
      #undef DUMP_PHY
      }

      void dumpState()
      {
      #define DUMP_BIT(reg) printf(#reg " = %01x\n", (val & Reg::c_##reg) ? 1 : 0)
      #define DUMP(reg) printf(#reg " = %02x\n", regRead(Reg::Addr::reg))
      #define DUMP16(reg) printf(#reg " = %04x\n", regRead16(Reg::Addr::reg))

        uint8_t savedECON1 = regRead(Reg::Addr::ECON1);
        uint8_t savedBank = m_bank;

        uint8_t val;

        val = regRead(Reg::Addr::EIR);
        DUMP_BIT(EIR_PKTIF);
        DUMP_BIT(EIR_DMAIF);
        DUMP_BIT(EIR_LINKIF);
        DUMP_BIT(EIR_TXIF);
        DUMP_BIT(EIR_TXERIF);
        DUMP_BIT(EIR_RXERIF);

        val = regRead(Reg::Addr::ESTAT);
        DUMP_BIT(ESTAT_INT);
        DUMP_BIT(ESTAT_BUFER);
        DUMP_BIT(ESTAT_LATECOL);
        DUMP_BIT(ESTAT_RXBUSY);
        DUMP_BIT(ESTAT_TXABRT);
        DUMP_BIT(ESTAT_CLKRDY);

        val = regRead(Reg::Addr::ECON2);
        DUMP_BIT(ECON2_PWRSV);
        DUMP_BIT(ECON2_VRPS);

        val = regRead(Reg::Addr::ECON1);
        DUMP_BIT(ECON1_TXRTS);
        DUMP_BIT(ECON1_RXEN);

        DUMP16(ERXRDPT16);
        DUMP16(ERXWRPT16);
        DUMP(EPKTCNT);

        printf("\n");

        regWrite(Reg::Addr::ECON1, savedECON1);
        m_bank = savedBank;

      #undef DUMP_BIT
      #undef DUMP
      #undef DUMP16
      }

      int test1()
      {
        Tools::CRC32 crc;
        for (uint8_t i = 0; i <= 0x17; ++i)
        {
          crc.update(opRCRE(i));
        }
        if (crc.get() != 0xeebd948d)
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
        Tools::CRC32 crc;
        for (size_t i = 0; i < std::extent<decltype(addrs)>::value; ++i)
        {
          crc.update(i);
          regWrite16(addrs[i], crc.get() & 0x1fff);
        }
        crc = Tools::CRC32();
        for (size_t i = 0; i < std::extent<decltype(addrs)>::value; ++i)
        {
          crc.update(i);
          if (regRead16(addrs[i]) != (crc.get() & 0x1fff))
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

      unsigned benchmark(const char* name, void(DeviceImpl::* const test_fn)(void* ctx), void* ctx, unsigned offset)
      {
        printf("Benchmarking %s...\n", name);
        TickType_t start = xTaskGetTickCount();
        while (start == xTaskGetTickCount());
        start = xTaskGetTickCount();
        TickType_t finish = start + 1000 / portTICK_PERIOD_MS;
        unsigned count = 0;
        Tools::IdleMeasure im;
        while (xTaskGetTickCount() < finish)
        {
          (this->*test_fn)(ctx);
          ++count;
        }
        int percent;
        int hundreds;
        im.get(percent, hundreds);
        finish = xTaskGetTickCount();
        unsigned duration_ns = (finish - start) * portTICK_PERIOD_MS * 1000000;
        unsigned cycle_ns = (duration_ns + count / 2) / count - offset;
        unsigned duration_clk = (finish - start) * portTICK_PERIOD_MS * ((SystemCoreClock + 500) / 1000);
        unsigned offset_clk = (offset * ((SystemCoreClock + 500) / 1000) + 500000) / 1000000;
        unsigned cycle_clk = (duration_clk + count / 2) / count - offset_clk;
        printf("               ... cycle = %u ns = %u CLKs CPU IDLE=%02u.%02u%%\n", cycle_ns, cycle_clk, percent, hundreds);
        return cycle_ns;
      }

      void benchmarkNull(void* /*ctx*/)
      {
      }

      void benchmarkTxRx(void* ctx)
      {
        static uint8_t buf[1000];
        m_spi->txRx(buf, (size_t)ctx);
      }

      void benchmarkTxThenTx(void* ctx)
      {
        static uint8_t buf[1000];
        m_spi->txThenTx(0, buf, (size_t)ctx);
      }

      void benchmarkTxThenRx(void* ctx)
      {
        static uint8_t buf[1000];
        m_spi->txThenRx(0, buf, (size_t)ctx);
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

      void benchmarkAll()
      {
        unsigned offset = benchmark("null", &DeviceImpl::benchmarkNull, 0, 0);
        benchmark("txRx 1", &DeviceImpl::benchmarkTxRx, (void*)1, offset);
        benchmark("txRx 2", &DeviceImpl::benchmarkTxRx, (void*)2, offset);
        benchmark("txRx 3", &DeviceImpl::benchmarkTxRx, (void*)3, offset);
        benchmark("txThenTx 0", &DeviceImpl::benchmarkTxThenTx, (void*)0, offset);
        benchmark("txThenTx 1", &DeviceImpl::benchmarkTxThenTx, (void*)1, offset);
        benchmark("txThenRx 1", &DeviceImpl::benchmarkTxThenRx, (void*)1, offset);
        benchmark("txThenRx 2", &DeviceImpl::benchmarkTxThenRx, (void*)2, offset);
//        benchmark("memRead 1", &DeviceImpl::benchmarkMemRead, (void*)1, offset);
//        benchmark("memRead 2", &DeviceImpl::benchmarkMemRead, (void*)2, offset);
//        benchmark("memWrite 0", &DeviceImpl::benchmarkMemWrite, (void*)0, offset);
//        benchmark("memWrite 1", &DeviceImpl::benchmarkMemWrite, (void*)1, offset);
//        benchmark("memRead 1000", &DeviceImpl::benchmarkMemRead, (void*)1000, offset);
        benchmark("memRead 100", &DeviceImpl::benchmarkMemRead, (void*)100, offset);
//        benchmark("memRead 10", &DeviceImpl::benchmarkMemRead, (void*)10, offset);
//        benchmark("memRead 51", &DeviceImpl::benchmarkMemRead, (void*)51, offset);
//        benchmark("memRead 50", &DeviceImpl::benchmarkMemRead, (void*)50, offset);
//        benchmark("memWrite 1000", &DeviceImpl::benchmarkMemWrite, (void*)1000, offset);
        benchmark("memWrite 100", &DeviceImpl::benchmarkMemWrite, (void*)100, offset);
//        benchmark("memWrite 10", &DeviceImpl::benchmarkMemWrite, (void*)10, offset);
//        benchmark("memWrite 51", &DeviceImpl::benchmarkMemWrite, (void*)51, offset);
//        benchmark("memWrite 50", &DeviceImpl::benchmarkMemWrite, (void*)50, offset);
//        benchmark("phyRead", &DeviceImpl::benchmarkPhyRead, 0, offset);
//        benchmark("phyWrite", &DeviceImpl::benchmarkPhyWrite, 0, offset);
      }

      void checkTx(uint8_t eir)
      {
        if (eir & (Reg::c_EIR_TXIF | Reg::c_EIR_TXERIF))
          //fixme: check tsv, log error if failed
          m_txInProgress = false;
      }

      void checkRx(uint8_t eir)
      {
        while (regRead(Reg::Addr::EPKTCNT))               //See comment 3
        {
          regSet(Reg::Addr::ECON2, Reg::c_ECON2_PKTDEC);
          regWrite16(Reg::Addr::ERDPT16, m_nextPacket);
          uint8_t rsv[6];
          memRead(rsv, sizeof(rsv));
          uint16_t nextPacket = rsv[0] | (rsv[1] << 8);
          uint16_t len = rsv[2] | (rsv[3] << 8);
          //fixme
          printf("packet received %u\n", len);
          if (true)   //fixme: check rsv; log on error
          {
            auto packet = m_env->allocatePbuf(len);
            if (packet->size() != len)
            {
              //log error
            } else {
              uint8_t* data;
              size_t size;
              while(packet->next(data, size))
                memRead(data, size);
              m_env->input(std::move(packet));
            }
          }
          regWrite16(Reg::Addr::ERXRDPT16, nextPacket);
          m_nextPacket = nextPacket;
        }
        if (eir & Reg::c_EIR_RXERIF)
        {
          //fixme
          printf("buffer overflow\n");
        }
      }

      void checkLink(uint8_t eir)
      {
        if (!(eir & Reg::c_EIR_LINKIF))
          return;

        phyRead(Reg::PhyAddr::PHIR);
        if (phyRead(Reg::PhyAddr::PHSTAT2) & Reg::c_PHSTAT2_LSTAT)
        {
          if (m_reportedUplink)
          {
            //fixme
            printf("link pulsed down then up\n");
            m_reportedUplink = false;
            m_env->setLinkState(false);
            m_reportedUplink = true;
            m_env->setLinkState(true);
          }
          else
          {
            //fixme
            printf("link up\n");
            m_reportedUplink = true;
            m_env->setLinkState(true);
          }
        }
        else
        {
          if (m_reportedUplink)
          {
            //fixme
            printf("link down\n");
            m_reportedUplink = false;
            m_env->setLinkState(false);
          }
          else
          {
            //fixme
            printf("link pulsed up then down\n");
          }
        }
      }

      void check(bool txOnly = false)
      {
        uint8_t eir = regRead(Reg::Addr::EIR);
        uint8_t eirClear = eir & (Reg::c_EIR_TXIF | Reg::c_EIR_TXERIF | (txOnly ? 0 : (Reg::c_EIR_PKTIF | Reg::c_EIR_RXERIF)));
        if (eirClear)
        {
          regClr(Reg::Addr::EIR, eirClear);
        }
        checkTx(eir);
        if (!txOnly)
        {
          checkRx(eir);
          checkLink(eir);
        }
      }

    public:
      virtual bool output(std::unique_ptr<Pbuf>&& packet) override
      {
        if (!m_txEnabled || m_failureFlags)
          return true;      //report error

        if (m_txInProgress)
          check(true);

        if (m_txInProgress)
          return false;

        m_txInProgress = true;
        const uint16_t offset = c_TxBufferStart;
        regWrite16(Reg::Addr::EWRPT16, offset);
        const uint8_t control = 0;
        memWrite(&control, sizeof(control));
        const uint8_t* data;
        size_t size;
        while(packet->next(data, size))
          memWrite(data, size);

        regSet(Reg::Addr::ECON1, Reg::c_ECON1_TXRST);                 //See comment 5
        regClr(Reg::Addr::ECON1, Reg::c_ECON1_TXRST);
        regWrite16(Reg::Addr::ETXST16, offset);
        regWrite16(Reg::Addr::ETXND16, offset + packet->size() - 1);
        regSet(Reg::Addr::ECON1, Reg::c_ECON1_TXRTS);
        return true;
      }

      virtual void periodic() override
      {
        for (;;)
        {
          switch (m_phase)
          {
          case 0:                             //See comment 1
            if (m_reportedUplink)
            {
              m_reportedUplink = false;
              m_env->setLinkState(false);
            }
            regClr(Reg::Addr::ECON2, Reg::c_ECON2_PWRSV);
            m_timer = OS::ExpirationTimer();
            ++m_phase;
            return;
          case 1:
            if (!m_timer.elapsedus(300))
            {
              return;
            }
            ++m_phase;
            //no break
          case 2:
            opSRC();
            m_timer = OS::ExpirationTimer();
            ++m_phase;
            return;
          case 3:
            if (!m_timer.elapsedus(1000))
            {
              return;
            }
            m_bank = 0;
            m_nextPacket = 0;
            ++m_phase;
            //no break
          case 4:
            regWrite(Reg::Addr::MACON1, Reg::c_MACON1_TXPAUS | Reg::c_MACON1_RXPAUS | Reg::c_MACON1_MARXEN);
            regWrite(Reg::Addr::MACON3, Reg::c_MACON3_PADCFG0 | Reg::c_MACON3_TXCRCEN | Reg::c_MACON3_FULDPX);
            regWrite(Reg::Addr::MABBIPG, 0x15);
            regWrite(Reg::Addr::MAIPG16, 0x12);
            regWrite16(Reg::Addr::MAMXFL16, c_MaxFrameLength);
            phyWrite(Reg::PhyAddr::PHCON1, Reg::c_PHCON1_PDPXMD);
            phyWrite(Reg::PhyAddr::PHLCON, 0x3c16);         //See comment 4: LEDA = link status and receive activity, LEDB = transmit activity
            phyWrite(Reg::PhyAddr::PHIE, Reg::c_PHIE_PLNKIE | Reg::c_PHIE_PGEIE);
            regWrite(Reg::Addr::MAADR1, m_mac.addr[0]);
            regWrite(Reg::Addr::MAADR2, m_mac.addr[1]);
            regWrite(Reg::Addr::MAADR3, m_mac.addr[2]);
            regWrite(Reg::Addr::MAADR4, m_mac.addr[3]);
            regWrite(Reg::Addr::MAADR5, m_mac.addr[4]);
            regWrite(Reg::Addr::MAADR6, m_mac.addr[5]);
            regWrite16(Reg::Addr::ERXST16, c_RxBufferStart);
            regWrite16(Reg::Addr::ERXND16, c_RxBufferEnd);
            regWrite16(Reg::Addr::ERXRDPT16, c_RxBufferStart);
            regSet(Reg::Addr::ECON1, Reg::c_ECON1_RXEN);
            m_txEnabled = true;
            m_txInProgress = false;
            ++m_phase;
            //no break
          case 5:
            check();
            if (m_failureFlags)
            {
              //fixme: log and/or update failure counter
              m_spi->reinit();  //fixme: check result
              m_failureFlags = 0;
              m_txEnabled = false;
              m_phase = 0;
              continue;
            }
            return;
          }
        }
      }

      virtual void test() override
      {
        regClr(Reg::Addr::ECON2, Reg::c_ECON2_PWRSV);
        OS::ExpirationTimer::delay(1);
        opSRC();
        OS::ExpirationTimer::delay(1);
        dumpRegs();
        dumpState();
//        validate();
        benchmarkAll();
      }
    };
  }
}

auto Enc28j60::CreateDevice(std::unique_ptr<Env>&& env, std::unique_ptr<Spi>&& spi, const MacAddress& mac) -> std::unique_ptr<Device>
{
  return std::make_unique<DeviceImpl>(std::move(env), std::move(spi), mac);
}
