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
  HTTP_INIT_ERROR,
  URL_CONFIG_ERROR,
  CONTENT_TYPE_ERROR,
  HTTP_ACTION_ERROR,
  HTTP_READ_ERROR,
  HTTP_TERM_ERROR,
  HTTP_RESPONSE_CODE_ERROR
};

class SIM_A7670C
{
public:
  SIM_A7670C(SoftwareSerial &serial);
  SIM_A7670C(HardwareSerial &serial);
  bool connect();
  bool registerNetwork(int &status);
  char *readSMS();
  bool sendSMS(const char *phoneNumber, const char *message);
  bool registerGPRS(int &stat, int &AcT, ErrorCode &error_code);
  bool httpGET(const char *url, const int port, int &response_code, ErrorCode &error_code);
  bool sendCommand(const char *cmd, const char *response, int timeout, ErrorCode &error_code);
  const char *getResponse() const;
  void trim(char *str);

  bool showATcommands = false;

private:
  Stream &gsmSerial;     // Reference to the serial stream (SoftwareSerial, HardwareSerial)
  char gsmResponse[256]; // Assuming maximum response length is 255 characters
};

#endif