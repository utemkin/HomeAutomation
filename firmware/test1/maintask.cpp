#include <common/utils.h>
#include <common/stm32.h>
#include <enc28j60/enc28j60.h>
#include <enc28j60/enc28j60spistm32.h>
#include "cmsis_os.h"
#include "main.h"

static unsigned benchmark(const char* name, void(*test_fn)(Enc28j60spi* enc, void* ctx), Enc28j60spi* enc, void* ctx, unsigned offset)
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

static void benchmark_null(Enc28j60spi* enc, void* /*ctx*/)
{
}

static void benchmark_rxtx(Enc28j60spi* enc, void* ctx)
{
  static uint8_t buf[1000];
  enc->txrx(buf, (size_t)ctx);
}

static void benchmark_all(Enc28j60spi* enc)
{
  unsigned offset = benchmark("null", &benchmark_null, 0, 0, 0);
  benchmark("rxtx 1", &benchmark_rxtx, enc, (void*)1, offset);
  benchmark("rxtx 2", &benchmark_rxtx, enc, (void*)2, offset);
  benchmark("rxtx 3", &benchmark_rxtx, enc, (void*)3, offset);
//  benchmark("mem_read 1000", &benchmark_mem_read, enc, (void*)1000, offset);
//  benchmark("mem_read 100", &benchmark_mem_read, enc, (void*)100, offset);
//  benchmark("mem_read 10", &benchmark_mem_read, enc, (void*)10, offset);
//  benchmark("mem_write 1000", &benchmark_mem_write, enc, (void*)1000, offset);
//  benchmark("mem_write 100", &benchmark_mem_write, enc, (void*)100, offset);
//  benchmark("mem_write 10", &benchmark_mem_write, enc, (void*)10, offset);
//  benchmark("phy_read", &benchmark_phy_read, enc, 0, offset);
//  benchmark("phy_write", &benchmark_phy_write, enc, 0, offset);
}

extern "C" void maintask()
{
  uint32_t uid[3];
  HAL_GetUID(uid);
  printf("Device %08lx%08lx%08lx is running at %lu\n", uid[0], uid[1], uid[2], HAL_RCC_GetHCLKFreq());

  auto spi = mstd::to_shared(CreateEnc28j60spiStm32(SPI1, SPI1_CS_GPIO_Port, SPI1_CS_Pin, true));
  auto enc = mstd::to_shared(CreateEnc28j60(spi));

  benchmark_all(spi.get());




  for(;;);
}
