#include <SoftwareSerial.h>
#include <SIM_A7670C.h>

SoftwareSerial gsmSerial(2, 3);
SIM_A7670C gsm(gsmSerial);

bool number_registered_to_receive_sms = false;
char phone_number[25];

void setup()
{
  Serial.begin(9600);
  initializeGSM();

  // while (!number_registered_to_receive_sms)
  // {
  //   char *smsMessage = gsm.readSMS();

  //   if (smsMessage != nullptr)
  //   {
  //     // Check if SMS contains "REG"
  //     char *checkREG = strstr(smsMessage, "REG");
  //     if (checkREG != nullptr)
  //     {
  //         smsMessage = checkREG + 3;
  //         strcpy(phone_number, smsMessage);
  //         gsm.trim(phone_number);

  //         if (gsm.sendSMS(phone_number, "Your number is Registered."))
  //         {
  //           Serial.print(F("Registered "));
  //           Serial.println(phone_number);
  //         }
  //     }
  //   }
  //   else
  //   {
  //     // If no message is available or an error occurs, print an error message
  //     Serial.println(F("Failed to read SMS or no SMS available."));
  //   }
  // }
}

void loop()
{
  // Your main code goes here
  delay(5000); // Adjust delay as needed
}

void initializeGSM()
{
  gsmSerial.begin(115200);

  // Set GSM module's baud rate to 9600
  gsm.sendCommand("AT+IPR=9600", "OK", 10000);

  // End and restart serial communication at the new baud rate
  gsmSerial.end();
  gsmSerial.begin(9600);

  // Check if the module is responsive at the new baud rate
  if (!gsm.sendCommand("AT", "OK", 5000))
  {
    Serial.println(F("GSM module not responding"));
    return;
  }

  if (!gsm.connect())
  {
    Serial.println(F("Failed to connect to GSM"));
    return;
  }
  Serial.println(F("GSM connected successfully"));

  int network_state;
  gsm.registerNetwork(network_state);
  if (network_state >= 0)
  {
    printNetworkStatus(network_state);
  }
  else
  {
    Serial.println(F("Failed to retrieve network status"));
  }

  int gprs_state, gprs_act;
  Serial.println(gsm.registerGPRS(gprs_state, gprs_act));
  if (gprs_state >= 0)
  {
    printGPRSStatus(gprs_state);
    printGPRSAccessTechnology(gprs_act);
  }
  else
  {
    Serial.println(F("Failed to retrieve GPRS status"));
  }
}

void printNetworkStatus(int status)
{
  Serial.print(F("Network status ("));
  Serial.print(status);
  Serial.print(F("): "));
  switch (status)
  {
  case 0:
    Serial.println(F("Not registered, the module is not searching a new operator to register to"));
    break;
  case 1:
    Serial.println(F("Registered, home network"));
    break;
  case 2:
    Serial.println(F("Not registered, but the module is searching for a new operator to register to"));
    break;
  case 3:
    Serial.println(F("Registration denied"));
    break;
  case 4:
    Serial.println(F("Unknown status"));
    break;
  case 5:
  case 6:
    Serial.println(F("Registered, in roaming"));
    break;
  default:
    Serial.println(F("No valid Network status"));
  }
}

void printGPRSStatus(int status)
{
  Serial.print(F("GPRS Network status ("));
  Serial.print(status);
  Serial.print(F("): "));
  switch (status)
  {
  case 0:
    Serial.println(F("Not registered, the module is not searching a new operator to register to"));
    break;
  case 1:
    Serial.println(F("Registered, home network"));
    break;
  case 2:
    Serial.println(F("Not registered, but the module is searching for a new operator to register to"));
    break;
  case 3:
    Serial.println(F("Registration denied"));
    break;
  case 4:
    Serial.println(F("Unknown status"));
    break;
  case 5:
  case 6:
    Serial.println(F("Registered, in roaming"));
    break;
  default:
    Serial.println(F("No valid GPRS Network status"));
  }
}

void printGPRSAccessTechnology(int technology)
{
  Serial.print(F("Access technology ("));
  Serial.print(technology);
  Serial.print(F("): "));
  switch (technology)
  {
  case 0:
    Serial.println(F("GSM"));
    break;
  case 1:
    Serial.println(F("GPRS"));
    break;
  case 2:
    Serial.println(F("EDGE"));
    break;
  case 3:
    Serial.println(F("WCDMA (UMTS)"));
    break;
  case 4:
    Serial.println(F("HSDPA"));
    break;
  case 5:
    Serial.println(F("HSUPA"));
    break;
  case 6:
    Serial.println(F("HSPA"));
    break;
  case 7:
    Serial.println(F("LTE"));
    break;
  default:
    Serial.println(F("No valid Access technology"));
  }
}
