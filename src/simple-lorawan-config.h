#pragma once

#include "mbed.h"

namespace SimpleLoRaWAN {

struct LoRaWANKeys {
    uint8_t devEui[8]; // = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
    uint8_t appEui[8]; // = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
    uint8_t appKey[16]; // = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
};

struct Pinmapping {
    PinName mosi; // = D11;
    PinName miso; // = D12;
    PinName clk; // = D13;
    PinName nss; // = A0;
    PinName reset; // = A1;
    PinName dio0; // = D2;
    PinName dio1; // = D3;
};

struct LoRaWANSettings {
    // spread factor (union ?)
    // adaptive data rate (union ?)
    // wait until connected (blockin constructor vs event based)
};

}