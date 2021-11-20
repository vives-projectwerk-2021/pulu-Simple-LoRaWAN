#include "Node.h"
#include "mbedtls_lora_config.h"
#include "lorawan/system/lorawan_data_structures.h"
#include "lorawan/LoRaRadio.h"
#include <stdio.h>

using namespace std::literals::chrono_literals;
using namespace events;

namespace SimpleLoRaWAN
{

  Node::Node(bool wait_until_connected):
  Node (
    {
      MBED_CONF_LORA_DEVICE_EUI,
      MBED_CONF_LORA_APPLICATION_EUI,
      MBED_CONF_LORA_APPLICATION_KEY
    }, wait_until_connected)
  {  }

  Node::Node(LoRaWANKeys keys, bool wait_until_connected): 
  Node (
    keys, { 
      MBED_CONF_SX1276_LORA_DRIVER_SPI_MOSI,
      MBED_CONF_SX1276_LORA_DRIVER_SPI_MISO,
      MBED_CONF_SX1276_LORA_DRIVER_SPI_SCLK,
      MBED_CONF_SX1276_LORA_DRIVER_SPI_CS,
      MBED_CONF_SX1276_LORA_DRIVER_RESET,
      MBED_CONF_SX1276_LORA_DRIVER_DIO0,
      MBED_CONF_SX1276_LORA_DRIVER_DIO1 
    }, wait_until_connected)
  {  }

  Node::Node(LoRaWANKeys keys, Pinmapping pins, bool wait_until_connected):
    radio(
      pins.mosi,
      pins.miso,
      pins.clk,
      pins.nss,
      pins.reset,
      pins.dio0,
      pins.dio1,
      NC, NC, NC, NC, NC, NC, NC, NC, NC, NC, NC),
    ev_queue(MAX_NUMBER_OF_EVENTS *EVENTS_EVENT_SIZE),
    lorawan(radio)
  {
    processThread.start(mbed::callback(this, &Node::processEvents));
    connected = false;
    lorawan_connect_t connect_params = { LORAWAN_CONNECTION_OTAA, {
      keys.devEui,
      keys.appEui,
      keys.appKey,
      5
    } };
    initialize();
    connect(connect_params);
    if(wait_until_connected) {
      while(!connected) {
        ThisThread::sleep_for(100ms);
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
    enableAdaptiveDataRate();
  }

  // Return: false on success; true on failure
  bool Node::enableAdaptiveDataRate()
  {
    if (lorawan.enable_adaptive_datarate() != LORAWAN_STATUS_OK) {
        debug("enable_adaptive_datarate failed!");
        return true;
    }
    else {
        debug("Adaptive data  rate (ADR) - Enabled");
        return false;
    }
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

  void Node::send(uint8_t* data, int size, uint8_t port, bool acknowledge)
  {
    uint8_t options = acknowledge ? MSG_CONFIRMED_FLAG : MSG_UNCONFIRMED_FLAG;
    int16_t retcode;

    retcode = lorawan.send(port, data, size, options);

    if (retcode < 0) {
        retcode == LORAWAN_STATUS_WOULD_BLOCK ? debug("send - WOULD BLOCK")
        : debug("send() - Error code %d", retcode);

        if (retcode == LORAWAN_STATUS_WOULD_BLOCK) {
            //retry in 3 seconds
            ev_queue.call_in(3s,
              mbed::callback(this, &Node::send), data, size, port, acknowledge);
        }
        return;
    }

    debug("%d bytes scheduled for transmission", retcode);
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
            break;
        case RX_DONE:
            debug("Received message from Network Server");
            receive();
            break;
        case RX_TIMEOUT:
        case RX_ERROR:
            debug("Error in reception - Code = %d", event);
            if (onReceptionError) {
              onReceptionError();
            }
            break;
        case JOIN_FAILURE:
            debug("OTAA Failed - Check Keys");
            if (onJoinFailure) {
              onJoinFailure();
            }
            break;
        case UPLINK_REQUIRED:
            debug("Uplink required by NS");
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

  void Node::receive() {
    if (onReceived) {
      char data[MBED_CONF_LORA_TX_MAX_SIZE] {0};
      uint8_t port;
      int flags;

      int16_t ret = lorawan.receive((uint8_t*)data, sizeof(data), port, flags);

      if(ret > 0) {
        debug("Received %d bytes on port %d", ret, port);
        onReceived(data, ret, port);
      }
    }
  }

  void Node::on_connected(mbed::Callback<void()> cb)           { onConnected = cb; }
  void Node::on_disconnected(mbed::Callback<void()> cb)        { onDisconnected = cb; }
  void Node::on_transmitted(mbed::Callback<void()> cb)         { onTransmitted = cb; }
  void Node::on_transmission_error(mbed::Callback<void()> cb)  { onTransmissionError = cb; }
  void Node::on_received(mbed::Callback<void(char* data, uint8_t length, uint8_t port)> cb)  { onReceived = cb; }
  void Node::on_reception_error(mbed::Callback<void()> cb)     { onReceptionError = cb; }
  void Node::on_join_failure(mbed::Callback<void()> cb)        { onJoinFailure = cb; }

}