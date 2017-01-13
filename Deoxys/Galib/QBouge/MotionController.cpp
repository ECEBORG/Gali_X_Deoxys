
#include "mbed.h"
#include <cstring>  // memcpy

#include "PID.h"
#include "QEI.h"

#include "common/utils.h"
#include "QBouge/Motor.h"
#include "pinout.h"

#include "MotionController.h"


MotionController::MotionController(void) :
    motor_l_(MOTOR_L_PWM, MOTOR_L_DIR, MOTOR_DIR_LEFT_FORWARD, MOTOR_L_CUR, MOTOR_L_TH, MOTOR_L_BRK),
    motor_r_(MOTOR_R_PWM, MOTOR_R_DIR, MOTOR_DIR_RIGHT_FORWARD, MOTOR_R_CUR, MOTOR_R_TH, MOTOR_R_BRK),
    enc_l_(ENC_L_DATA1, ENC_L_DATA2, /*NC,*/ PULSES_PER_REV, QEI::X4_ENCODING),
    enc_r_(ENC_R_DATA1, ENC_R_DATA2, /*NC,*/ PULSES_PER_REV, QEI::X4_ENCODING),
    pid_dist_(0.3*PID_DIST_KU, PID_DIST_TU/2, PID_DIST_TU/8, PID_UPDATE_INTERVAL),
    pid_angle_(0.3*PID_ANGLE_KU, PID_ANGLE_TU/2, PID_ANGLE_TU/8, PID_UPDATE_INTERVAL)
{
    pid_dist_.setInputLimits(-5*1000, 5*1000);  // encoders value
    pid_dist_.setOutputLimits(-1.0, 1.0);  // motor speed (~pwm)
    pid_dist_.setMode(AUTO_MODE);  // AUTO_MODE or MANUAL_MODE
    pid_dist_.setBias(0); // magic *side* effect needed for the pid to work, don't comment this
    pid_dist_.setSetPoint(0);
    this->pidDistSetGoal(0);  // pid's error

    pid_angle_.setInputLimits(-M_PI, M_PI);  // angle. 0 toward, -pi on right, +pi on left
    pid_angle_.setOutputLimits(-1.0, 1.0);  // motor speed (~pwm). -1 right, +1 left, 0 nothing
    pid_angle_.setMode(AUTO_MODE);  // AUTO_MODE or MANUAL_MODE
    pid_angle_.setBias(0); // magic *side* effect needed for the pid to work, don't comment this
    pid_angle_.setSetPoint(0);
    this->pidAngleSetGoal(0);  // pid's error

    this->ordersReset();
    order_count_ = 0;

    pos_.x = 0;
    pos_.y = 0;

    enc_l_val_ = 0;
    enc_r_val_ = 0;

    enc_l_last_ = 0;
    enc_r_last_ = 0;
}

void MotionController::fetchEncodersValue(void) {
    enc_l_last_ = enc_l_val_;
    enc_r_last_ = enc_r_val_;

    enc_l_val_ = enc_l_.getPulses();
    enc_r_val_ = enc_r_.getPulses();
}


/*
    diff_l and diff_r: enc distance in mm, + forward, - backward
    cur_angle: current angle. in radian. facing east is 0. positive is CCW. (std maths)
    cur_x and cur_y: current position. in mm. 0 is bottom left.

    Return 0 on success.
*/
int mc_calcNewPos(
    float diff_l, float diff_r,
    float cur_angle, float cur_x, float cur_y,
    float *new_angle_, float *new_x_, float *new_y_
) {
    float distance = 0, radius = 0;
    float dangle = 0, dx = 0, dy = 0;
    float new_angle = 0;
    float new_x = 0, new_y = 0;

    if (diff_l == diff_r)
    {
        distance = diff_l;

        dangle = 0;
        new_angle = cur_angle;

        dx = distance * cos(cur_angle);
        dy = distance * sin(cur_angle);
    }
    else if (diff_l == - diff_r)
    {
        distance = 0;

        dangle = 0;
        new_angle = cur_angle;

        dx = 0;
        dy = 0;
    }
    else
    {
        distance = (diff_l + diff_r) / 2;
        radius = 1.0 * ENC_POS_RADIUS * (diff_r + diff_l) / (diff_r - diff_l);

        // angle
        dangle = distance / radius;
        new_angle = std_rad_angle(cur_angle + dangle);

        // pos
        dx = radius * (sin(new_angle)-sin(cur_angle));
        dy = - radius * (cos(new_angle)-cos(cur_angle));
    }

    new_x = cur_x + dx;
    new_y = cur_y + dy;

    // update class members
    *new_angle_ = new_angle;
    *new_x_ = new_x;
    *new_y_ = new_y;

    return 0;
}


void MotionController::updatePosition(void) {
    float diff_l = 0, diff_r = 0;  // unit: mm

    diff_l = TICKS_TO_MM(enc_l_val_-enc_l_last_);
    diff_r = TICKS_TO_MM(enc_r_val_-enc_r_last_);

    mc_calcNewPos(
        diff_l, diff_r,
        angle_, pos_.x, pos_.y,
        &angle_, &pos_.x, &pos_.y
    );

    motor_l_.updateSpeed(diff_l);
    motor_r_.updateSpeed(diff_r);
    speed_ = TICKS_TO_MM(diff_l + diff_r) / 2 / PID_UPDATE_INTERVAL;
}


void mc_calcDistThetaOrderPos(float *dist_, float *theta_) {
    float dist = 0, theta = 0;

    dist = *dist_;
    theta = *theta_;

    if ((theta < -M_PI/2) || (M_PI/2 < theta))
    {
        dist = -dist;
        theta = std_rad_angle(theta+M_PI);
    }

    *dist_ = dist;
    *theta_ = theta;
}


int mc_updateCurOrder(
    s_vector_float cur_pos,  float cur_angle, s_order *cur_order, float time_since_last_order_finished,
    float *dist_, float *theta_
) {
    float dx = 0, dy = 0;
    float dist = 0, theta = 0;  // units: mm, rad
    int ret = 0;

    dist = *dist_;
    theta = *theta_;

    // update the goals in function of the given order

    switch (cur_order->type)
    {
        case ORDER_TYPE_POS:
            dx = cur_order->pos.x - cur_pos.x;
            dy = cur_order->pos.y - cur_pos.y;

            dist = DIST(dx, dy);
            theta = std_rad_angle(atan2(dy, dx) - cur_angle);
            mc_calcDistThetaOrderPos(&dist, &theta);

            if (ABS(dist) < 30)
                ret = 1;

            break;

        case ORDER_TYPE_ANGLE:
            dist = 0;
            theta = std_rad_angle(cur_order->angle - cur_angle);

            if (ABS(theta) < DEG2RAD(10))
                ret = 1;

            break;

        case ORDER_TYPE_DELAY:
            dist = 0;
            theta = 0;

            if (time_since_last_order_finished > cur_order->delay)
                ret = 1;

            break;
    }

    *dist_ = dist;
    *theta_ = theta;

    return ret;
}


void MotionController::updateCurOrder(float match_timestamp) {
    float dist = 0, theta = 0;  // units: mm, rad

    if (order_count_ != 0)
    {
        if (mc_updateCurOrder(pos_, angle_, &orders_[0], match_timestamp-last_order_timestamp_, &dist, &theta))
            this->updateGoalToNextOrder(match_timestamp);
    }

    this->pidDistSetGoal(dist);
    this->pidAngleSetGoal(theta);
}


void MotionController::computePid(void) {
    pid_dist_.setProcessValue(-pid_dist_goal_);
    pid_dist_out_ = pid_dist_.compute();

    pid_angle_.setProcessValue(-pid_angle_goal_);
    pid_angle_out_ = pid_angle_.compute();
}


void MotionController::updateMotors(void) {
    float mot_l_val = 0, mot_r_val = 0, m = 0;

    if (order_count_ == 0)
    {
        mot_l_val = 0;
        mot_r_val = 0;
    }
    else
    {
        mot_l_val = pid_dist_out_ - pid_angle_out_;
        mot_r_val = pid_dist_out_ + pid_angle_out_;

        // if the magnitude of one of the two is > 1, divide by the bigest of these
        // two two magnitudes (in order to keep the scale)
        if ((ABS(mot_l_val) > 1) || (ABS(mot_r_val) > 1))
        {
            m = ABS(MAX(mot_l_val, mot_r_val));
            mot_l_val /= m;
            mot_r_val /= m;
        }
    }

    motor_l_.setSPwm(mot_l_val);
    motor_r_.setSPwm(mot_r_val);
}


void MotionController::debug(Debug *debug) {
    int i = 0;

    debug->printf("[MC/i] %d %d\n", enc_l_val_, enc_r_val_);
    debug->printf("[MC/t_pid] (dist angle) %.3f %.3f\n", pid_dist_out_, pid_angle_out_);
    debug->printf("[MC/o_mot] (pwm current) %.3f (%.3f A) | %.3f (%.3f A)\n",
        motor_l_.getSPwm(), motor_l_.current_sense_.read()*4,
        motor_r_.getSPwm(), motor_r_.current_sense_.read()*4
    );
    debug->printf("[MC/o_robot] (pos angle speed) %.0f %.0f %.0f %.0f\n", pos_.x, pos_.y, RAD2DEG(angle_), speed_);

    if (order_count_ == 0)
        debug->printf("[MC/orders] -\n");
    else
    {
        for (i = 0; (i < order_count_) && (i < 3); ++i)
        {
            debug->printf("[MC/orders] %d/%d -> %d %.0f %.0f %.0f %.3f\n",
                i,
                order_count_,
                orders_[i].type,
                orders_[i].pos.x,
                orders_[i].pos.y,
                RAD2DEG(orders_[i].angle),
                orders_[i].delay
            );
        }
    }
}

void MotionController::debug(CanMessenger *cm) {
    cm->send_msg_CQB_MC_pos(pos_.x, pos_.y);
    cm->send_msg_CQB_MC_angle_speed(RAD2DEG(angle_), speed_);

    cm->send_msg_CQB_MC_encs(enc_l_val_, enc_r_val_);
    cm->send_msg_CQB_MC_pids(pid_dist_out_, pid_angle_out_);
    cm->send_msg_CQB_MC_motors(motor_l_.getSPwm(), motor_r_.getSPwm());

    // cm->send_msg_CQB_MC_order_pos(int16_t x, int16_t y);
    // cm->send_msg_CQB_MC_order_angle(float angle);
    // cm->send_msg_CQB_MC_order_delay(float delay);
}

void MotionController::pidDistSetGoal(float goal) {
    pid_dist_goal_ = goal;
}


void MotionController::pidAngleSetGoal(float goal) {
    pid_angle_goal_ = goal;
}


void MotionController::ordersReset(void) {
    memset(orders_, 0, sizeof(s_order)*MAX_ORDERS_COUNT);
}


int MotionController::ordersAppend(e_order_type type, int16_t x, int16_t y, float angle, uint16_t delay) {
    if (order_count_ == MAX_ORDERS_COUNT)
        return 1;

    memcpy(&orders_[order_count_], &orders_[order_count_-1], sizeof(s_order));

    orders_[order_count_].type = type;

    switch (type) {
        case ORDER_TYPE_POS:
            orders_[order_count_].pos.x = x;
            orders_[order_count_].pos.y = y;
            break;

        case ORDER_TYPE_ANGLE:
            orders_[order_count_].angle = std_rad_angle(angle);
            break;

        case ORDER_TYPE_DELAY:
            orders_[order_count_].delay = delay;
            break;
    }

    order_count_ ++;

    return 0;
}


int MotionController::ordersAppendAbsPos(int16_t x, int16_t y) {
    return this->ordersAppend(ORDER_TYPE_POS, x, y, 0, 0);
}


int MotionController::ordersAppendAbsAngle(float angle) {
    return this->ordersAppend(ORDER_TYPE_ANGLE, 0, 0, angle, 0);
}


int MotionController::ordersAppendAbsDelay(uint16_t delay) {
    return this->ordersAppend(ORDER_TYPE_DELAY, 0, 0, 0, delay);
}

int MotionController::ordersAppendRelDist(int16_t dist) {
    s_order *last_order = &orders_[order_count_-1];
    return this->ordersAppend(
        ORDER_TYPE_POS,
        last_order->pos.x + dist*cos(last_order->angle), last_order->pos.y + dist*sin(last_order->angle),
        last_order->angle, last_order->delay
    );
}

int MotionController::ordersAppendRelAngle(float angle) {
    s_order *last_order = &orders_[order_count_-1];
    return this->ordersAppend(
        ORDER_TYPE_ANGLE, last_order->pos.x, last_order->pos.y, last_order->angle + angle, last_order->delay
    );
}

void MotionController::updateGoalToNextOrder(float match_timestamp) {
    memmove(&orders_[0], &orders_[1], sizeof(s_order)*(order_count_-1));

    order_count_ --;
    last_order_timestamp_ = match_timestamp;
}

void MotionController::setMotor(float l, float r, Debug *debug, char *reason) {
    this->ordersReset();  // override orders and pid to make sure they don't interfere

    motor_l_.setSPwm(l);
    motor_r_.setSPwm(r);
}
