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
        printf("\r\n LoRa initialization failed! \r\n");
        // return -1;
    }

    printf("\r\n Mbed LoRaWANStack initialized \r\n");

    // prepare application callbacks
    callbacks.events = mbed::callback(this, &Node::lora_event_handler);
    lorawan.add_app_callbacks(&callbacks);

    // Set number of retries in case of CONFIRMED messages
    if (lorawan.set_confirmed_msg_retries(CONFIRMED_MSG_RETRY_COUNTER)
            != LORAWAN_STATUS_OK) {
        printf("\r\n set_confirmed_msg_retries failed! \r\n\r\n");
        // return -1;
    }

    printf("\r\n CONFIRMED message retries : %d \r\n",
           CONFIRMED_MSG_RETRY_COUNTER);

    // Enable adaptive data rate
    if (lorawan.enable_adaptive_datarate() != LORAWAN_STATUS_OK) {
        printf("\r\n enable_adaptive_datarate failed! \r\n");
        // return -1;
    }

    printf("\r\n Adaptive data  rate (ADR) - Enabled \r\n");
  }

  void Node::connect(lorawan_connect_t &params)
  {
        // stores the status of a call to LoRaWAN protocol
    lorawan_status_t retcode;

    retcode = lorawan.connect(params);

    if (retcode == LORAWAN_STATUS_OK ||
            retcode == LORAWAN_STATUS_CONNECT_IN_PROGRESS) {
    } else {
        printf("\r\n Connection error, code = %d \r\n", retcode);
        // return -1;
    }

    printf("\r\n Connection - In Progress ...\r\n");
  }

  void Node::send(uint8_t* data, int size, unsigned char port, bool acknowledge)
  {
    uint8_t options = acknowledge ? MSG_CONFIRMED_FLAG : MSG_UNCONFIRMED_FLAG;
    int16_t retcode;

    retcode = lorawan.send(port, data, size, options);

    if (retcode < 0) {
        retcode == LORAWAN_STATUS_WOULD_BLOCK ? printf("send - WOULD BLOCK\r\n")
        : printf("\r\n send() - Error code %d \r\n", retcode);

        if (retcode == LORAWAN_STATUS_WOULD_BLOCK) {
            //retry in 3 seconds
            ev_queue.call_in(3000, mbed::callback(this, &Node::send_message));
        }
        return;
    }

    printf("\r\n %d bytes scheduled for transmission \r\n", retcode);
    memset(tx_buffer, 0, sizeof(tx_buffer));
  }

  void Node::lora_event_handler(lorawan_event_t event)
{
    switch (event) {
        case CONNECTED:
            connected = true;
            printf("\r\n Connection - Successful \r\n");
            break;
        case DISCONNECTED:
            ev_queue.break_dispatch();
            printf("\r\n Disconnected Successfully \r\n");
            break;
        case TX_DONE:
            printf("\r\n Message Sent to Network Server \r\n");
            send_message();
            break;
        case TX_TIMEOUT:
        case TX_ERROR:
        case TX_CRYPTO_ERROR:
        case TX_SCHEDULING_ERROR:
            printf("\r\n Transmission Error - EventCode = %d \r\n", event);
            // try again
            send_message();
            break;
        case RX_DONE:
            printf("\r\n Received message from Network Server \r\n");
            receive_message();
            break;
        case RX_TIMEOUT:
        case RX_ERROR:
            printf("\r\n Error in reception - Code = %d \r\n", event);
            break;
        case JOIN_FAILURE:
            printf("\r\n OTAA Failed - Check Keys \r\n");
            break;
        case UPLINK_REQUIRED:
            printf("\r\n Uplink required by NS \r\n");
            send_message();
            break;
        default:
            MBED_ASSERT("Unknown Event");
    }
}

void Node::processEvents()
{
  // make your event queue dispatching events forever
  ev_queue.dispatch_forever();
}


void Node::send_message(){}
void Node::receive_message(){}

}