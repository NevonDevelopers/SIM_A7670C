#include "Arduino.h"
#include "SIM_A7670C.h"

SIM_A7670C::SIM_A7670C(uint8_t rxpin, uint8_t txpin) : gsmSerial(rxpin, txpin)
{
    // Constructor implementation: Initialize SoftwareSerial with rxpin and txpin
}

void SIM_A7670C::initialize()
{
    Serial.begin(9600);
    Serial.println(F("Setting GSM baudrate.."));
    gsmSerial.begin(115200);
    getResponse("AT+IPR=9600", OK, 10000);
    gsmSerial.end();
    gsmSerial.begin(9600);

    Serial.println(F("Waiting for GSM to response.."));
    while (!getResponse("AT", "OK", 5000))
    {
        delay(200);
    }
    Serial.println(F("GSM is ready now!"));
}

bool SIM_A7670C::connect()
{
    const char *commands[] = {
        "ATE0",            // Disables command echo to improve communication speed and efficiency.
        "AT+CLIP=1",       // Enables caller line identification presentation for incoming calls.
        "AT+CVHU=0",       // Turns off result code presentation mode to reduce unnecessary data.
        "AT+CTZU=1",       // Enables automatic time zone update based on network time zone information.
        "AT+CMGF=1",       // Sets SMS message format to text mode for human-readable messages.
        "AT+CNMI=0,0,0,0", // Disables new message indication, preventing automatic notification of new SMS messages.
        "AT+CSQ",          // Checks GSM network signal strength (RSSI) for connectivity status assessment.
        "AT+CREG?",        // Queries registration status of GSM network for proper communication verification.
        "AT+CGREG?",       // Queries registration status in GPRS network for data transmission verification.
        "AT+CMGD=1,4"      // Deletes all SMS messages from memory, including read and unread messages.
    };

    const int numCommands = sizeof(commands) / sizeof(commands[0]);

    for (int i = 0; i < numCommands; i++)
    {
        if (!getResponse(commands[i], OK, 1000))
        {
            return false;
        }
    }

    return true;
}

uint8_t SIM_A7670C::getResponse(const char *cmd, const char *response, int timeout)
{
    current_time = millis();
    int index = -1;
    uint8_t network_status = 0;
    Serial.print(F("Sending to GSM: "));
    Serial.println(cmd);
    gsmSerial.println(cmd);

    while (millis() - current_time < timeout)
    {
        if (gsmSerial.available())
        {
            // Read response from GSM module into gsm_response buffer
            gsmSerial.readBytesUntil('\n', gsm_response, sizeof(gsm_response));

            // Null-terminate the string to ensure it's properly terminated
            gsm_response[sizeof(gsm_response) - 1] = '\0';

            // Trim leading and trailing whitespace from the response
            trim(gsm_response);

            // Print trimmed response to serial monitor
            Serial.print(F("Data received: "));
            Serial.println(gsm_response);

            // Find the index of the specified substring (response) in the trimmed response
            char *index_ptr = strstr(gsm_response, response);
            int index = index_ptr ? index_ptr - gsm_response : -1;

            // To clear the character array
            memset(gsm_response, '\0', sizeof(gsm_response));

            if (cmd == "AT+CREG?")
            {
                // network_status = gsm_response.substring(gsm_response.indexOf(',') + 1, gsm_response.indexOf(',') + 2).toInt();
                // if (network_status == 1 || network_status == 5 || network_status == 6)
                // {
                //     Serial.println(F("GSM Network registered!"));
                //     return true;
                // }
                // else
                // {
                //     Serial.println(F("ERROR: Failed to register network."));
                //     Serial.println(F("       Press RESET."));
                //     return false;
                // }
            }
            else
            {
                if (index >= 0)
                {
                    return true;
                }
            }
        }
    }
}

void SIM_A7670C::trim(char *str)
{
    // Trim leading whitespace
    char *start = str;
    while (*start && isspace(*start))
    {
        start++;
    }

    // Trim trailing whitespace
    char *end = start + strlen(start) - 1;
    while (end > start && isspace(*end))
    {
        *end-- = '\0';
    }

    // Shift trimmed string to the beginning
    if (start != str)
    {
        memmove(str, start, end - start + 2);
    }
}
