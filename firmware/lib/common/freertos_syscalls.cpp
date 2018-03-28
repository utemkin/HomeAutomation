#include "FreeRTOS.h"
#include "task.h"
#include <malloc.h>
#include <errno.h>

#if !configUSE_NEWLIB_REENTRANT
#error configUSE_NEWLIB_REENTRANT==0 not supported
#endif

extern "C"
{
  void __malloc_lock(struct _reent *)
  {
    vTaskSuspendAll();
  }

  void __malloc_unlock(struct _reent *)
  {
    xTaskResumeAll();
  };

  void __env_lock(struct _reent *reent)
  {
    vTaskSuspendAll();
  }

  void __env_unlock(struct _reent *reent)
  {
    xTaskResumeAll();
  }

  extern uint8_t _estack;
  extern uint8_t _end;
  extern uint8_t _Min_Stack_Size;
  static uint8_t* s_curHeapEnd = &_end;
  static uint8_t* const s_maxHeapEnd = &_estack - size_t(&_Min_Stack_Size);

  void* _sbrk(ptrdiff_t incr)
  {
    vTaskSuspendAll();
    uint8_t* ret = s_curHeapEnd;
    if (ret + incr > s_maxHeapEnd)
    {
      xTaskResumeAll();
      errno = ENOMEM;
      return (void*)-1;
    }
    s_curHeapEnd = ret + incr;
    xTaskResumeAll();
    return ret;
  }

  void* pvPortMalloc(size_t xSize) PRIVILEGED_FUNCTION
  {
    void* ret = malloc(xSize);
    #if( configUSE_MALLOC_FAILED_HOOK == 1 )
    {
      if(ret == NULL)
      {
        extern void vApplicationMallocFailedHook();
        vApplicationMallocFailedHook();
      }
    }
    #endif
    return ret;
  }

  void vPortFree(void* pv) PRIVILEGED_FUNCTION
  {
    free(pv);
  };

  size_t xPortGetFreeHeapSize() PRIVILEGED_FUNCTION
  {
    struct mallinfo mi = mallinfo();
    return mi.fordblks + (s_maxHeapEnd - s_curHeapEnd);
  }
}
