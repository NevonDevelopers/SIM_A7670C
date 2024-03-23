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
#define MAX_SMS_LENGTH 255

class SIM_A7670C
{
public:
  SIM_A7670C(SoftwareSerial &serial);
  SIM_A7670C(HardwareSerial &serial);
  bool connect();
  bool registerNetwork(int &status);
  int registerGPRS(int &stat, int &AcT);
  int httpGET(const char *url, const int port);
  char *readSMS();
  bool sendSMS(const char *phoneNumber, const char *message);
  int sendCommand(const char *cmd, const char *response, int timeout);
  const char *getResponse() const;
  void trim(char *str);

private:
  Stream &gsmSerial;     // Reference to the serial stream (SoftwareSerial, HardwareSerial)
  char gsmResponse[256]; // Assuming maximum response length is 255 characters

  enum ErrorCode
  {
    NO_ERROR,
    TIMEOUT_ERROR,
    RESPONSE_PARSE_ERROR,
    BUFFER_OVERFLOW_ERROR,
    GSM_NETWORK_ERROR,
    GPRS_NETWORK_ERROR,
    OPERATOR_ERROR,
    PACKET_DOMAIN_ERROR,
    PDP_CONTEXT_ERROR,
    HTTP_ERROR,
    HTTPINIT_ERROR,
    HTTPPARA_ERROR,
    HTTPACTION_ERROR
  };
};

#endif