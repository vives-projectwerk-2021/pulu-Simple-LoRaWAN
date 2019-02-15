#include "Node.h"
#include "mbedtls_lora_config.h"
#include "lorawan/system/lorawan_data_structures.h"
#include "lorawan/LoRaRadio.h"
#include <stdio.h>

using namespace events;

namespace SimpleLoRaWAN
{

  Node::Node(uint8_t _dev_eui[], uint8_t _app_eui[], uint8_t _app_key[], bool wait_until_connected):
    radio(D11, D12, D13, A0, A1, D2, D3, NC, NC, NC, NC, NC, NC, NC, NC, NC, NC, NC),
    ev_queue(MAX_NUMBER_OF_EVENTS *EVENTS_EVENT_SIZE),
    lorawan(radio),
    processThread(mbed::callback(this, &Node::processEvents))
  {
    connected = false;
    lorawan_connect_t connect_params = { LORAWAN_CONNECTION_OTAA, {_dev_eui, _app_eui, _app_key, 5} };
    initialize();
    connect(connect_params);
    if(wait_until_connected) {
      while(!connected) {
        Thread::wait(100);
      }
    }
  }

  Node::~Node(){}

  void Node::initialize()
  {
    // Initialize LoRaWAN stack
    if (lorawan.initialize(&ev_queue) != LORAWAN_STATUS_OK) {
        debug("LoRa initialization failed!");
        // return -1;
    }

    debug("Mbed LoRaWANStack initialized");

    // prepare application callbacks
    callbacks.events = mbed::callback(this, &Node::lora_event_handler);
    lorawan.add_app_callbacks(&callbacks);

    // Set number of retries in case of CONFIRMED messages
    if (lorawan.set_confirmed_msg_retries(CONFIRMED_MSG_RETRY_COUNTER)
            != LORAWAN_STATUS_OK) {
        debug("set_confirmed_msg_retries failed!");
        // return -1;
    }

    debug("CONFIRMED message retries : %d",
           CONFIRMED_MSG_RETRY_COUNTER);

    // Enable adaptive data rate
    if (lorawan.enable_adaptive_datarate() != LORAWAN_STATUS_OK) {
        debug("enable_adaptive_datarate failed!");
        // return -1;
    }

    debug("Adaptive data  rate (ADR) - Enabled");
  }

  void Node::connect(lorawan_connect_t &params)
  {
        // stores the status of a call to LoRaWAN protocol
    lorawan_status_t retcode;

    retcode = lorawan.connect(params);

    if (retcode == LORAWAN_STATUS_OK ||
            retcode == LORAWAN_STATUS_CONNECT_IN_PROGRESS) {
    } else {
        debug("Connection error, code = %d", retcode);
        // return -1;
    }

    debug("Connection - In Progress ...");
  }

  void Node::send(uint8_t* data, int size, unsigned char port, bool acknowledge)
  {
    uint8_t options = acknowledge ? MSG_CONFIRMED_FLAG : MSG_UNCONFIRMED_FLAG;
    int16_t retcode;

    retcode = lorawan.send(port, data, size, options);

    if (retcode < 0) {
        retcode == LORAWAN_STATUS_WOULD_BLOCK ? debug("send - WOULD BLOCK")
        : debug("send() - Error code %d", retcode);

        if (retcode == LORAWAN_STATUS_WOULD_BLOCK) {
            //retry in 3 seconds
            ev_queue.call_in(3000, mbed::callback(this, &Node::send_message));
        }
        return;
    }

    debug("%d bytes scheduled for transmission", retcode);
    memset(tx_buffer, 0, sizeof(tx_buffer));
  }

  void Node::lora_event_handler(lorawan_event_t event)
{
    switch (event) {
        case CONNECTED:
            connected = true;
            debug("Connection - Successful");
            if (onConnected) {
              onConnected();
            }
            break;
        case DISCONNECTED:
            connected = false;
            ev_queue.break_dispatch();
            debug("Disconnected Successfully");
            if (onDisconnected) {
              onDisconnected();
            }
            break;
        case TX_DONE:
            debug("Message Sent to Network Server");
            send_message();
            if (onTransmitted) {
              onTransmitted();
            }
            break;
        case TX_TIMEOUT:
        case TX_ERROR:
        case TX_CRYPTO_ERROR:
        case TX_SCHEDULING_ERROR:
            debug("Transmission Error - EventCode = %d", event);
            if (onTransmissionError) {
              onTransmissionError();
            }
            // try again
            send_message();
            break;
        case RX_DONE:
            debug("Received message from Network Server");
            receive_message();
            break;
        case RX_TIMEOUT:
        case RX_ERROR:
            debug("Error in reception - Code = %d", event);
            break;
        case JOIN_FAILURE:
            debug("OTAA Failed - Check Keys");
            break;
        case UPLINK_REQUIRED:
            debug("Uplink required by NS");
            send_message();
            break;
        default:
            debug("Unknown event happened");
    }
}

void Node::processEvents()
{
  // make your event queue dispatching events forever
  ev_queue.dispatch_forever();
}

void Node::send_message(){}
void Node::receive_message(){}

void Node::on_connected(mbed::Callback<void()> cb)           { onConnected = cb; }
void Node::on_disconnected(mbed::Callback<void()> cb)        { onDisconnected = cb; }
void Node::on_transmitted(mbed::Callback<void()> cb)         { onTransmitted = cb; }
void Node::on_transmission_error(mbed::Callback<void()> cb)  { onTransmissionError = cb; }

}