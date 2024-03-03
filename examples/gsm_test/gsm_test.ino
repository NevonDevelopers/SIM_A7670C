#include<SoftwareSerial.h>
#include<SIM_A7670C.h>

SoftwareSerial gsmSerial(2, 3); // Define SoftwareSerial object
SIM_A7670C gsm(gsmSerial);

void setup() {
  Serial.begin(9600);
  gsmSerial.begin(115200);
  // Attempt to set the GSM module's baud rate to 9600 for stable communication
  gsm.sendCommand("AT+IPR=9600", OK, 10000);

    // End and restart serial communication at the new baud rate
    gsmSerial.end();
    gsmSerial.begin(9600);

    // Check if the module is responsive at the new baud rate
    gsm.sendCommand(AT, OK, 5000);

  Serial.begin(9600);
  // put your setup code here, to run once:
  if(gsm.connect())
  {
    Serial.println(F("OK2"));
  }
  else {
  Serial.println("Not OK2");
  }
  int i = gsm.registerNetwork();
  Serial.println(i);
  int j = gsm.registerGPRS();
  Serial.println(j);
  gsm.sendSMS("+917972017465", "Hi Himanshu");
}

void loop() {
  // put your main code here, to run repeatedly:
  // Call the readSMS function to retrieve the SMS message
    char* smsMessage = gsm.readSMS();

    if (smsMessage != nullptr) {
        // If a message is successfully retrieved, print it
        Serial.println(F("Received SMS:"));
        Serial.println(smsMessage);
    } else {
        // If no message is available or an error occurs, print an error message
        Serial.println(F("Failed to read SMS or no SMS available."));
    }

    delay(5000); // Delay before checking for new SMS (adjust as needed)
}
