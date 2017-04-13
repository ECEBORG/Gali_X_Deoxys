#ifdef IAM_QBOUGE

#ifndef MOTOR_H_INCLUDED
#define MOTOR_H_INCLUDED

#include "mbed.h"

#define PWM_MIN                 0.08                            // pwm value at which the robot start moving
#define PWM_MAX                 1.00                            // should be 1.00 during matchs
#define PWM_STEP                (1.0/(ASSERV_FPS*1.0))          // pwm goes from 0 to 1 over a X sec timespan
#define PWM_ERROR_TOLERANCE     (PWM_STEP/2)

/*
    Forward and backward direction (value of the digital input `direction` of
    the H bridge) for each motor.
    The code is symetric (MOTOR_DIR_LEFT_FORWARD == MOTOR_DIR_RIGHT_FORWARD),
    but the physical motors are not. This is because the PCB is not symetric.
    Mind the plug-to-motor symetricity also.
*/
#define MOTOR_DIR_LEFT_FORWARD      1
#define MOTOR_DIR_LEFT_BACKWARD     0
#define MOTOR_DIR_RIGHT_FORWARD     1
#define MOTOR_DIR_RIGHT_BACKWARD    0


class Motor {

public:
    Motor(PinName pwm_pin, PinName dir_pin, bool forward_dir);

    /*
        Set direction.
            false (0): backward
            true (1): forward
    */
    void setDir(bool dir);

    /*
        Set unsigned PWM value.
        Value between 0 and 1.
    */
    void setUPwm(float uPwm);

    /*
        Set speed (direction and pwm).
        Value between -1 and +1.
    */
    void setSPwm(float sPwm);

    /*
        Get direction.
            false (0): backward
            true (1): forward
    */
    bool getDir(void);

    /*
        Set unsigned PWM value.
        Value between 0 and 1.
    */
    float getUPwm(void);

    /*
        Get theorical speed (direction and pwm).
        Value between -1 and +1.
    */
    float getSPwm(void);

    /*
        Compute and save the actual speed of the wheel. This value is computed
        from encoder ticks.
    */
    void updateSpeed(float mm_since_last_loop);

    /*
        Return the computed speed.
        Unit: mm/sec
    */
    float getSpeed(void);

protected:
    PwmOut pwm_;
    DigitalOut dir_;
    bool forward_dir_;
    float speed_;  // unit: mm/sec
    float last_sPwm_;
};

#endif // #ifndef MOTOR_H_INCLUDED
#endif // #ifdef IAM_QBOUGE
