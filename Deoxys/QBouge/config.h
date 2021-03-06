#ifndef CONFIG_H_INCLUDED
#define CONFIG_H_INCLUDED

/*
    Read Doc/code_assumptions.md to not fuck everything up by changing things here.
*/

/*
    Debug prints. Comment to disable, uncomment to enable.
*/

// #define PRINT_TIME
// #define PRINT_COM_CAN_REC
// #define PRINT_COM_CAN_SEND
// #define PRINT_MONITORING_RESET
// #define PRINT_SYS_CAN_STATS

/*
    Common config
*/

#define MAIN_LOOP_FPS       200
#define MAIN_LOOP_DELAY     (1.0/MAIN_LOOP_FPS)

/*
    The asservissement function is call through an interrupt (Ticker) (a priori
    mbed links that to TIM2).
    For reference: gali IX 500 Hz (2 ms) - eseo 200 Hz (5 ms) - two randoms from
    GitHub 100 Hz (10 ms) and 67 Hz (15 ms).
*/
#define ASSERV_FPS          200
#define ASSERV_DELAY        (1.0/ASSERV_FPS)


#define ORDERS_COUNT        100

#endif // CONFIG_H_INCLUDED
