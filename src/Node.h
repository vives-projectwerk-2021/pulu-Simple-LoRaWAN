#pragma once

#include "stdint.h"
#include "events/EventQueue.h"
#include "lorawan/LoRaWANInterface.h"
#include "lorawan/LoRaRadio.h"
#include "SX1276_LoRaRadio.h"
#include "rtos.h"
#include "simple-lorawan-config.h"

// #define LORAWAN_DEBUGGING
// #ifdef LORAWAN_DEBUGGING
//   #define debug(MSG, ...)  printf("[Simple-LoRaWAN] " MSG "\r\n", ## __VA_ARGS__)
// #else
#define debug(msg, ...) ((void)0)
// #endif

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
    Node(bool wait_until_connected = true);
    Node(LoRaWANKeys keys, bool wait_until_connected = true);
    Node(LoRaWANKeys keys, Pinmapping pins, bool wait_until_connected = true);
    virtual ~Node();

    bool enableAdaptiveDataRate();
    void send(uint8_t* data, int size, uint8_t port = 1, bool acknowledge = false);

  // Register callbacks
  public:
    void on_connected(mbed::Callback<void()> cb);
    void on_disconnected(mbed::Callback<void()> cb);
    void on_transmitted(mbed::Callback<void()> cb);
    void on_transmission_error(mbed::Callback<void()> cb);
    void on_received(mbed::Callback<void(char* data, uint8_t length, uint8_t port)> cb);
    void on_reception_error(mbed::Callback<void()> cb);
    void on_join_failure(mbed::Callback<void()> cb);

private:

    SX1276_LoRaRadio radio;
    events::EventQueue ev_queue;
    LoRaWANInterface lorawan;
    lorawan_app_callbacks_t callbacks;

    bool connected;

    void initialize();
    void connect(lorawan_connect_t &params);

    void lora_event_handler(lorawan_event_t event);
    void processEvents();

    void receive();

    // Allows for easy event handler registration
    mbed::Callback<void()> onConnected;
    mbed::Callback<void()> onDisconnected;
    mbed::Callback<void()> onTransmitted;
    mbed::Callback<void()> onTransmissionError;
    mbed::Callback<void(char* data, uint8_t length, uint8_t port)> onReceived;
    mbed::Callback<void()> onReceptionError;
    mbed::Callback<void()> onJoinFailure;

    Thread processThread;
};

};