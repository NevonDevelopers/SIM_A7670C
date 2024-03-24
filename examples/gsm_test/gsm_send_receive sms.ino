#include <SoftwareSerial.h>
#include <SIM_A7670C.h>

// Define the RX and TX pins for the GSM module connection
#define GSM_RX_PIN 2
#define GSM_TX_PIN 3

SoftwareSerial gsmSerial(GSM_RX_PIN, GSM_TX_PIN);
SIM_A7670C gsm(gsmSerial);

bool number_registered_to_receive_sms = false;
char phone_number[25]; // Ensure the buffer is large enough to store a phone number plus null terminator

void setup()
{
  Serial.begin(9600);
  // Wait for Serial to be ready - necessary for some boards
  while (!Serial)
  {
  }

  Serial.println(F("Initializing GSM module..."));
  
  gsm.showATcommands = true;
  if (!initializeGSM())
  {
    Serial.println(F("Failed to initialize GSM module."));
    while (1)
      ; // Halt execution if GSM module cannot be initialized
  }

  Serial.println(F("GSM module initialized successfully."));

  int network_status;
  if (gsm.connect() && gsm.registerNetwork(network_status))
  {
    Serial.println(F("GSM network registration successful."));
  }
  else
  {
    Serial.println(F("Network registration failed."));
    Serial.print(F("Network Status: "));
    Serial.println(network_status);
    // Consider whether to halt execution or attempt reconnection
  }

  Serial.println(F("Waiting for registration SMS..."));
}

void loop()
{
  if (!number_registered_to_receive_sms)
  {
    // Send SMS to GSM for registeration
    // This SMS should follow the format "REG+91XXXXXXXXXX", where "+91XXXXXXXXXX" 
    // should be replaced with the actual phone number to register.
    checkForRegistrationSMS();
  }

  // Place the rest of your main code here, if any
  delay(5000); // Example delay, adjust as needed
}

void checkForRegistrationSMS()
{
  char *smsMessage = gsm.readSMS();
  if (smsMessage != nullptr)
  {
    // Parse the SMS message for registration command
    if (parseRegistrationSMS(smsMessage))
    {
      if (gsm.sendSMS(phone_number, "Hello from SIM_A7670C! You're now registered to receive SMS."))
      {
        Serial.print(F("Registered: "));
        Serial.println(phone_number);
        number_registered_to_receive_sms = true;
      }
      else
      {
        Serial.println(F("Failed to send registration confirmation SMS."));
      }
    }
    free(smsMessage); // Important: Prevent memory leak
  }
}

bool parseRegistrationSMS(char *smsMessage)
{
  // Check if SMS contains "REG" keyword
  char *checkREG = strstr(smsMessage, "REG");
  if (checkREG != nullptr)
  {
    checkREG += 3; // Move past "REG"
    strncpy(phone_number, checkREG, sizeof(phone_number) - 1);
    phone_number[sizeof(phone_number) - 1] = '\0'; // Ensure null-termination
    gsm.trim(phone_number);                        // Remove any leading/trailing whitespace
    return true;
  }
  return false;
}

bool initializeGSM()
{
  ErrorCode error;

  gsmSerial.begin(115200);
  // Attempt to set the GSM module's baud rate
  gsm.sendCommand("AT+IPR=9600", "OK", 10000, error);
  // Restart serial communication at the new baud rate
  gsmSerial.end();
  gsmSerial.begin(9600);
  // Verify the module is responsive at the new baud rate
  if (gsm.sendCommand("AT", "OK", 5000, error))
  {
    return true;
  }
  else
  {
    Serial.println(F("GSM module not responsive at 9600 baud."));
  }

  Serial.print(F("Error Code: "));
  Serial.println(static_cast<int>(error));
  return false;
}
