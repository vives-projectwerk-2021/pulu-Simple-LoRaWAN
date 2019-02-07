#pragma once

#include "stdint.h"
#include "events/EventQueue.h"
#include "lorawan/LoRaWANInterface.h"
#include "SX1276_LoRaRadio.h"
#include "rtos.h"

/*
 * Sets up an application dependent transmission timer in ms. Used only when Duty Cycling is off for testing
 */
#define TX_TIMER                        10000

/**
 * Maximum number of events for the event queue.
 * 10 is the safe number for the stack events, however, if application
 * also uses the queue for whatever purposes, this number should be increased.
 */
#define MAX_NUMBER_OF_EVENTS            10

/**
 * Maximum number of retries for CONFIRMED messages before giving up
 */
#define CONFIRMED_MSG_RETRY_COUNTER     3

namespace SimpleLoRaWAN
{

class Node
{
public:
    Node(uint8_t _dev_eui[], uint8_t _app_eui[], uint8_t _app_key[], bool wait_until_connected = true);
    virtual ~Node();
    // void send(char* data, int size, bool acknowledge = false);
    // void send(unsigned char port, char* data, int size, bool acknowledge = false);
    // void send(uint8_t* data, int size, bool acknowledge = false);
    void send(uint8_t* data, int size, unsigned char port = 1, bool acknowledge = false);

private:

    events::EventQueue ev_queue;
    SX1276_LoRaRadio radio;
    LoRaWANInterface lorawan;
    lorawan_app_callbacks_t callbacks;

    uint8_t tx_buffer[30];
    uint8_t rx_buffer[30];

    bool connected;

    void initialize();
    void connect(lorawan_connect_t &params);

    void lora_event_handler(lorawan_event_t event);
    void processEvents();

    Thread processThread;

    void send_message();   // TEMP DUMMY !!!
    void receive_message();   // TEMP DUMMY !!!

//     void setReceiveHandler(void (*fnc)(uint8_t port, uint8_t* data, uint8_t length));

//     void setEventHandler(void (*fnc)(ev_t));
//     void setScanTimeoutEventHandler(void (*fnc)());
//     void setBeaconFoundEventHandler(void (*fnc)());
//     void setBeaconMissedEventHandler(void (*fnc)());
//     void setBeaconTrackedEventHandler(void (*fnc)());
//     void setJoiningEventHandler(void (*fnc)());
//     void setJoinedEventHandler(void (*fnc)());
//     void setRfu1EventHandler(void (*fnc)());
//     void setJoinFailedEventHandler(void (*fnc)());
//     void setRejoinFailedEventHandler(void (*fnc)());
//     void setTxCompleteEventHandler(void (*fnc)());
//     void setLostTSyncEventHandler(void (*fnc)());
//     void setResetEventHandler(void (*fnc)());
//     void setRxCompleteEventHandler(void (*fnc)());
//     void setLinkDeadEventHandler(void (*fnc)());
//     void setLinkAliveEventHandler(void (*fnc)());

//     void enableLinkCheck();
//     void disableLinkCheck();
//     void setLinkCheck(int state);

//     void setSpreadFactor(int spreadfactor);

//     int timeUntilNextSend();

// private:
//     void init();
//     void setLinkCheck();

//     void (*eventHandler)(ev_t);
//     void (*scanTimeoutEventHandler)();
//     void (*beaconFoundEventHandler)();
//     void (*beaconMissedEventHandler)();
//     void (*beaconTrackedEventHandler)();
//     void (*joiningEventHandler)();
//     void (*joinedEventHandler)();
//     void (*rfu1EventHandler)();
//     void (*joinFailedEventHandler)();
//     void (*rejoinFailedEventHandler)();
//     void (*txCompleteEventHandler)();
//     void (*lostTsyncEventHandler)();
//     void (*resetEventHandler)();
//     void (*rxCompleteEventHandler)();
//     void (*linkDeadEventHandler)();
//     void (*linkAliveEventHandler)();
//     void (*receiveHandler)(uint8_t, uint8_t*, uint8_t);
};

} /* namespace SimpleLoRaWAN */

