
#include "mbed.h"
#include "mbed_stats.h"

#include "common/Debug.h"
#include "common/Messenger.h"

#ifdef IAM_QBOUGE
#include "PID.h"
#include "QEI.h"
#include "QBouge/Motor.h"
#include "QBouge/MotionController.h"
#endif

#include "mem_stats.h"

void mem_stats(Debug *debug)
{
    debug->printf("----- sizeof\n");
#ifdef IAM_QBOUGE
    debug->printf("MotionController %d\n", sizeof(MotionController));
    debug->printf("\tMotor          %d*2=%d\n", sizeof(Motor), sizeof(Motor)*2);
    debug->printf("\tQEI            %d*2=%d\n", sizeof(QEI), sizeof(QEI)*2);
    debug->printf("\tPID            %d*2=%d\n", sizeof(PID), sizeof(PID)*2);
    debug->printf("\ts_order        %d*%d=%d\n", sizeof(s_order), MAX_ORDERS_COUNT, sizeof(s_order)*MAX_ORDERS_COUNT);
#endif
    debug->printf("Debug            %d\n", sizeof(Debug));
    debug->printf("\tBufferedSerial %d*%d=%d\n", sizeof(BufferedSerial), Debug::DEBUG_LAST, sizeof(BufferedSerial)*Debug::DEBUG_LAST);
    debug->printf("Timer            %d\n", sizeof(Timer));
    debug->printf("CanMessenger     %d\n", sizeof(CanMessenger));

    mbed_stats_heap_t heap_stats;
    osEvent info;
    osThreadId main_id = osThreadGetId();

    debug->printf("----- heap\n");

    mbed_stats_heap_get(&heap_stats);
    debug->printf("Current heap: %d\n", heap_stats.current_size);
    debug->printf("Max heap size: %d\n", heap_stats.max_size);

    debug->printf("----- stack\n");

    info = _osThreadGetInfo(main_id, osThreadInfoStackSize);
    if (info.status != osOK)
        error("Could not get stack size");
    uint32_t stack_size = (uint32_t)info.value.v;

    info = _osThreadGetInfo(main_id, osThreadInfoStackMax);
    if (info.status != osOK)
        error("Could not get max stack");
    uint32_t max_stack = (uint32_t)info.value.v;

    debug->printf("Stack used %d of %d bytes\n", max_stack, stack_size);

    debug->printf("-----\n");
}