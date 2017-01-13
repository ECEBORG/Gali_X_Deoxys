
#include <cstring>
#include "mbed.h"

#include "common/utils.h"
#include "QBouge/MotionController.h"
#include "pinout.h"

#include "Messenger.h"


/*
    Message (payload send via CAN)
*/

Message::Message(void) {
    id = MT_empty;
    len = 0;
    memset(payload.raw_data, 0, 8);
}

Message::Message(e_message_type id_, unsigned int len_, u_payload payload_) {
    id = id_;
    len = len_;
    payload = payload_;
}


/*
    CanMessenger (medium used to transmit Message). This could be UartMessenger or I2cMessenger.
*/

CanMessenger::CanMessenger(void) : can_(CAN_RX, CAN_TX) {
    can_.frequency(100*1000);
}

int CanMessenger::send_msg(Message msg) {
    return can_.write(
        CANMessage(msg.id, msg.payload.raw_data, msg.len)
    );
}

int CanMessenger::read_msg(Message *dest) {
    CANMessage msg;
    int ret = 0;

    ret = can_.read(msg);
    if (ret)
    {
        dest->id = (Message::e_message_type)msg.id;
        dest->len = msg.len;
        memcpy(dest->payload.raw_data, msg.data, 8);
    }
    return ret;
}

/*
    CanMessenger::send_msg_*
*/

int CanMessenger::send_msg_ping(char data[8]) {
    Message::CP_ping payload;
    memcpy(payload.data, data, 8);

    return this->send_msg(Message(Message::MT_ping, sizeof(payload), (Message::u_payload){.ping = payload}));
}

int CanMessenger::send_msg_pong(char data[8]) {
    Message::e_message_type message_type;
    Message::CP_pong payload;

#ifdef IAM_QBOUGE
    message_type = Message::MT_CQB_pong;
#endif
#ifdef IAM_QREFLECHI
    message_type = Message::MT_CQR_pong;
#endif
    memcpy(payload.data, data, 8);

    return this->send_msg(Message(message_type, sizeof(payload), (Message::u_payload){.pong = payload}));
}

int CanMessenger::send_msg_CQB_MC_pos(float x, float y) {
    Message::CP_CQB_MC_pos payload;
    payload.pos.x = x;
    payload.pos.y = y;

    return this->send_msg(Message(Message::MT_CQB_MC_order, sizeof(payload), (Message::u_payload){.CQB_MC_pos = payload}));
}

int CanMessenger::send_msg_CQB_MC_angle_speed(float angle, float speed) {
    Message::CP_CQB_MC_angle_speed payload;
    payload.angle = angle;
    payload.speed = speed;
    return this->send_msg(Message(Message::MT_CQB_MC_angle_speed, sizeof(payload), (Message::u_payload){.CQB_MC_angle_speed = payload}));
}

int CanMessenger::send_msg_CQB_MC_encs(int32_t enc_l, int32_t enc_r) {
    Message::CP_CQB_MC_encs payload;
    payload.enc_l = enc_l;
    payload.enc_r = enc_r;
    return this->send_msg(Message(Message::MT_CQB_MC_encs, sizeof(payload), (Message::u_payload){.CQB_MC_encs = payload}));
}

int CanMessenger::send_msg_CQB_MC_pids(float dist, float angle) {
    Message::CP_CQB_MC_pids payload;
    payload.dist = dist;
    payload.angle = angle;
    return this->send_msg(Message(Message::MT_CQB_MC_pids, sizeof(payload), (Message::u_payload){.CQB_MC_pids = payload}));
}

int CanMessenger::send_msg_CQB_MC_motors(float pwm_l, float pwm_r) {
    Message::CP_CQB_MC_motors payload;
    payload.pwm_l = pwm_l;
    payload.pwm_r = pwm_r;
    return this->send_msg(Message(Message::MT_CQB_MC_motors, sizeof(payload), (Message::u_payload){.CQB_MC_motors = payload}));
}

int CanMessenger::send_msg_CQB_MC_order_pos(int16_t x, int16_t y) {
    Message::CP_CQB_MC_order payload;
    payload.type = ORDER_TYPE_POS;
    payload.order_data.pos.x = x;
    payload.order_data.pos.y = y;
    return this->send_msg(Message(Message::MT_CQB_MC_order, sizeof(payload), (Message::u_payload){.CQB_MC_order = payload}));
}

int CanMessenger::send_msg_CQB_MC_order_angle(float angle) {
    Message::CP_CQB_MC_order payload;
    payload.type = ORDER_TYPE_ANGLE;
    payload.order_data.angle = angle;
    return this->send_msg(Message(Message::MT_CQB_MC_order, sizeof(payload), (Message::u_payload){.CQB_MC_order = payload}));
}

int CanMessenger::send_msg_CQB_MC_order_delay(float delay) {
    Message::CP_CQB_MC_order payload;
    payload.type = ORDER_TYPE_DELAY;
    payload.order_data.delay = delay;
    return this->send_msg(Message(Message::MT_CQB_MC_order, sizeof(payload), (Message::u_payload){.CQB_MC_order = payload}));
}
