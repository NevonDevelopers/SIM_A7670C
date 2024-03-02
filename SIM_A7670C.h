/*
  SIM_A7670C.h - Library for SIM_A7670C 4G GSM Module.
  Created by Himanshu Sharma, March 2, 2024.
  Released for Nevon Solutions.
*/
#ifndef SIM_A7670C_h
#define SIM_A7670C_h

#include <Arduino.h>
#include <SoftwareSerial.h>

#define OK "OK"
#define AT "AT"

class SIM_A7670C
{
    public:
        SIM_A7670C(uint8_t rxpin, uint8_t txpin);
        void initialize();
        bool connect();
        uint8_t getResponse(const char *cmd, const char *response, int timeout);
        void trim(char *str);

    private:
        SoftwareSerial gsmSerial; // Instance of SoftwareSerial
        unsigned long current_time = 0;
        char gsm_response[256]; // Assuming maximum response length is 255 characters

        uint8_t rx_pin = 2;
        uint8_t tx_pin = 3;
};

#endif