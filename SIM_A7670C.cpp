#include "Arduino.h"
#include "SIM_A7670C.h"

// Constructor for SoftwareSerial
SIM_A7670C::SIM_A7670C(SoftwareSerial &serial) : gsmSerial(serial) {}

// Constructor for HardwareSerial
SIM_A7670C::SIM_A7670C(HardwareSerial &serial) : gsmSerial(serial) {}

/**
 * @brief Initializes the GSM module by configuring its settings for optimized operation.
 * This includes disabling command echo, configuring call and message settings, and
 * ensuring the module's time is synchronized with the network. These configurations
 * are essential for efficient communication and resource management.
 *
 * @return true if all commands are successfully executed, indicating the module is ready for operation.
 * @return false if any command fails, which suggests an issue with the module or its connection to the network.
 */
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
        "AT+CMGD=1,4"      // Deletes all SMS messages from memory, including read and unread messages.
    };

    for (const char *cmd : commands)
    {
        if (!sendCommand(cmd, OK, 1000))
        {
            return false;
        }
    }

    return true;
}

/**
 * @brief Function to verify if the module is correctly registered for communication.
 *
 * @return int
 */
int SIM_A7670C::registerNetwork()
{
    // Queries the GSM network registration status to verify if the module is correctly registered for communication.
    if (sendCommand("AT+CREG?", OK, 5000))
    {
        // Look for the "+CREG" response prefix to parse the registration status.
        char *response = strstr(gsmResponse, "+CREG:");
        if (response != nullptr)
        {
            int status;
            // Parse the response to extract the registration status. Assumes format is "+CREG: <n>,<status>"
            // The sscanf function skips the <n> part and directly reads the <status>.
            if (sscanf(response, "+CREG: %*d,%d", &status) == 1)
            {
                return status; // Return the extracted status.
            }
        }
    }
    // Log an error if the query failed or the response could not be parsed.
    Serial.println(F("ERROR: Failed to query or parse network registration status."));
    return -1; // Indicates an error condition, distinguishing between communication and parsing failures.
}

/**
 * @brief Function to check GPRS network registration status and access technology
 *
 * @return int
 */
bool SIM_A7670C::registerGPRS(int &stat, int &AcT)
{
    // Queries registration status in GPRS network for data transmission verification.
    if (sendCommand("AT+CGREG?", OK, 5000))
    {
        // Check if the response contains +CGREG
        if (strstr(gsmResponse, "+CGREG:"))
        {
            int n;
            // Attempt to parse the response
            // Note: This assumes <lac> and <ci> are not needed directly, but adjust if you need them
            if (sscanf(gsmResponse, "+CGREG: %d,%d,,,%d", &n, &stat, &AcT) >= 2)
            {
                return true; // Return the registration status
            }
        }
    }

    Serial.println(F("Failed to parse CGREG response."));
    return false; // Indicates failure to parse the response
}

/**
 * @brief Reads the first SMS message from the GSM module and deletes it.
 *
 * @return char* Pointer to the SMS message content, or nullptr if an error occurs.
 */
char *SIM_A7670C::readSMS()
{
    // Attempt to read an SMS
    if (sendCommand("AT+CMGL=\"REC UNREAD\"", OK, 5000))
    {
        char *newlinePos = strchr(gsmResponse, '\n');
        if (newlinePos != nullptr)
        {
            size_t remainingLength = strlen(newlinePos + 1);
            if (remainingLength > 0 && remainingLength < MAX_SMS_LENGTH)
            {
                char *smsMessage = (char *)malloc(remainingLength + 1); // Allocate memory for the SMS message
                if (smsMessage != nullptr)
                {
                    // Check if the allocation was successful
                    strncpy(smsMessage, newlinePos + 1, remainingLength);
                    smsMessage[remainingLength] = '\0'; // Ensure null-termination

                    // Find the position of "OK" in smsMessage
                    char *okPosition = strstr(smsMessage, "OK");

                    if (okPosition != nullptr)
                    {
                        // Calculate the length of the message without "OK"
                        size_t messageLength = okPosition - smsMessage - 3;

                        // Create a temporary buffer to store the modified message
                        char tempMessage[messageLength + 1];

                        // Copy the part of smsMessage before "OK" to tempMessage
                        strncpy(tempMessage, smsMessage, messageLength);
                        tempMessage[messageLength] = '\0'; // Null-terminate the string

                        // Copy the modified message back to smsMessage
                        strcpy(smsMessage, tempMessage);
                    }

                    sendCommand("AT+CMGD=1,4", OK, 5000); // Delete all SMS messages to clean up
                    return smsMessage;                    // Return the dynamically allocated message
                }
            }
        }
    }
    return nullptr; // Return nullptr if reading failed or memory allocation was unsuccessful
}

/**
 * @brief Sends an SMS message to the specified phone number.
 *
 * @param phoneNumber
 * @param message
 * @return true
 * @return false
 */
bool SIM_A7670C::sendSMS(const char *phoneNumber, const char *message)
{
    // Construct the AT command to send SMS
    char command[50];
    snprintf(command, sizeof(command), "AT+CMGS=\"%s\"", phoneNumber);

    // Send the AT command to initiate SMS sending
    if (!sendCommand(command, '>', 5000))
    {
        Serial.println(F("Failed to initiate SMS sending."));
        return false;
    }

    // Send the SMS message
    gsmSerial.print(message);

    // Send the Ctrl+Z character to indicate the end of the message
    gsmSerial.write(0x1A);

    // Wait for the module to send the SMS
    if (!sendCommand("", "OK", 10000))
    {
        Serial.println(F("Failed to send SMS."));
        return false;
    }

    Serial.println(F("SMS sent successfully."));
    return true;
}

/**
 * @brief This function sends a command to the SIM A7670C 4G GSM module and waits for a specific response within a given timeout period.
 *
 * @param cmd The AT command to send to the GSM module.
 * @param response The expected substring to look for in the module's response.
 * @param timeout he period (in milliseconds) to wait for a response.
 * @return true if the expected response is found or specific conditions are met.
 * @return false otherwise.
 */

bool SIM_A7670C::sendCommand(const char *cmd, const char *response, int timeout)
{
    gsmSerial.flush();                     // Clear any previous input.
    unsigned long current_time = millis(); // Stores the start time
    gsmSerial.println(cmd);                // Send command to GSM module

    while (millis() - current_time < timeout)
    {
        if (gsmSerial.available())
        {
            // Clear the buffer to ensure no residual data
            memset(gsmResponse, '\0', sizeof(gsmResponse));

            // Read response into buffer, ensuring null termination
            gsmSerial.readBytesUntil(response, gsmResponse, sizeof(gsmResponse) - 1);

            // Trim for cleaner comparison and logging
            trim(gsmResponse);

            // Check if the response contains the expected response string
            if (strstr(gsmResponse, response) != nullptr)
            {
                return true;
            }
        }
    }
    return false; // Return the result of the operation
}

/**
 * @brief Returns a pointer to a constant character array, preventing modification of the buffer
 *
 * @return const char*
 */
const char *SIM_A7670C::getResponse() const
{
    return gsmResponse;
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
