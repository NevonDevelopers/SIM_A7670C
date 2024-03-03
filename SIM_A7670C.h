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
#define MAX_SMS_LENGTH 255

class SIM_A7670C
{
public:
  SIM_A7670C(SoftwareSerial &serial);
  SIM_A7670C(HardwareSerial &serial);
  bool connect();
  int registerNetwork();
  int registerGPRS();
  char *readSMS();
  bool sendSMS(const char *phoneNumber, const char *message);
  bool sendCommand(const char *cmd, const char *response, int timeout);
  const char *getResponse() const;
  void trim(char *str);

private:
  Stream &gsmSerial;        // Reference to the serial stream (SoftwareSerial, HardwareSerial)
  char gsmResponse[256];    // Assuming maximum response length is 255 characters
};

#endif