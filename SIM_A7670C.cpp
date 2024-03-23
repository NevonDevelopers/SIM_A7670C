#include "Arduino.h"
#include "SIM_A7670C.h"

// Constructor for SoftwareSerial
SIM_A7670C::SIM_A7670C(SoftwareSerial &serial) : gsmSerial(serial) {}

// Constructor for HardwareSerial
SIM_A7670C::SIM_A7670C(HardwareSerial &serial) : gsmSerial(serial) {}

/**
 * @brief Establishes a connection with the GSM module by sending a series of AT commands.
 *
 * @return Returns true if the connection is established successfully, otherwise false.
 */
bool SIM_A7670C::connect()
{
    // Array of AT commands to be sent to the GSM module
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

    // Iterate through the array of commands and send each command to the GSM module
    for (const char *cmd : commands)
    {
        // Send the command and check if it executed successfully
        if (sendCommand(cmd, OK, 1000) != ErrorCode::NO_ERROR)
        {
            // If any command fails, return false to indicate connection failure
            return false;
        }
    }

    // All commands executed successfully, return true to indicate successful connection
    return true;
}

/**
 * @brief Queries the GSM network registration status and parses the response.
 *
 * @param[out] status Reference to an integer to store the network registration status.
 *
 * @return Returns true if the network registration is successful, otherwise false.
 */
bool SIM_A7670C::registerNetwork(int &status)
{
    // Send command to query the GSM network registration status
    if (!sendCommand("AT+CREG?", OK, 5000))
    {
        // Failed to send command or receive response
        return false;
    }

    // Look for the "+CREG" response prefix in the received response
    char *response = strstr(gsmResponse, "+CREG:");
    if (response == nullptr)
    {
        // Response format doesn't match expected format
        return false;
    }

    // Parse the response to extract the registration status
    if (sscanf(response, "+CREG: %*d,%d", &status) != 1)
    {
        // Failed to parse response
        return false;
    }

    // Check if the registration status indicates successful registration
    switch (status)
    {
    case 1:
    case 5:
    case 6:
        return true; // Successfully registered
    default:
        return false; // Not registered
    }
}

/**
 * @brief Registers the device on the GPRS network for data transmission.
 *
 * @param[out] stat Reference to an integer to store the registration status.
 * @param[out] AcT Reference to an integer to store the Access Technology.
 *
 * @return Returns an error code indicating the success or failure of the operation.
 */
int SIM_A7670C::registerGPRS(int &stat, int &AcT)
{
    // Query COPS response to extract Access Technology (AcT)
    if (sendCommand("AT+COPS?", "OK", 1000) != ErrorCode::NO_ERROR ||
        sscanf(gsmResponse, "+COPS: %*d,%*d,\"%*[^\"]\",%d", &AcT) != 1)
    {
        return ErrorCode::OPERATOR_ERROR;
    }

    // Check CGATT status for packet domain registration
    if (sendCommand("AT+CGATT?", "OK", 1000) != ErrorCode::NO_ERROR ||
        sscanf(gsmResponse, "+CGATT: %d", &stat) != 1 || stat != 1)
    {
        return ErrorCode::PACKET_DOMAIN_ERROR;
    }

    // Activate PDP context for data transmission
    if (sendCommand("AT+CGACT=1,1", "OK", 5000) != ErrorCode::NO_ERROR)
    {
        return ErrorCode::PDP_CONTEXT_ERROR;
    }

    // Query CGREG response for registration status and AcT
    if (sendCommand("AT+CGREG?", "OK", 5000) != ErrorCode::NO_ERROR ||
        sscanf(gsmResponse, "+CGREG: %*d,%d,,,%d", &stat, &AcT) < 1)
    {
        return ErrorCode::GPRS_NETWORK_ERROR;
    }

    return ErrorCode::NO_ERROR;
}

/**
 * @brief Performs an HTTP GET request and retrieves the HTTP response code.
 *
 * @param[in] url The URL to request.
 * @param[in] port The port to use for the request.
 * @param[out] response_code The HTTP response code.
 *
 * @return Returns true if the request is successful, false otherwise.
 */
bool SIM_A7670C::httpGET(const char *url, const int port, int &response_code)
{
    // Terminate any existing HTTP service
    sendCommand("AT+HTTPTERM", OK, 1000);

    // Initialize HTTP service
    if (sendCommand("AT+HTTPINIT", OK, 1000) != ErrorCode::NO_ERROR)
    {
        Serial.println("Failed to initialize HTTP service.");
        return false;
    }

    // Set URL for GET request
    char httpRequest[255];
    snprintf(httpRequest, sizeof(httpRequest), "AT+HTTPPARA=\"URL\",\"%s:%d\"", url, port);
    if (sendCommand(httpRequest, "OK", 1000) != ErrorCode::NO_ERROR)
    {
        Serial.println("Failed to set URL for HTTP GET request.");
        return false;
    }

    // Set Content-Type header
    if (sendCommand("AT+HTTPPARA=\"CONTENT\",\"text/plain\"", "OK", 1000) != ErrorCode::NO_ERROR)
    {
        Serial.println("Failed to set Content-Type header.");
        return false;
    }

    // Perform HTTP GET request
    if (sendCommand("AT+HTTPACTION=0", "+HTTPACTION:", 20000) != ErrorCode::NO_ERROR)
    {
        Serial.println("Failed to perform HTTP GET request.");
        return false;
    }

    // Extract HTTP response code
    if (sscanf(gsmResponse, "+HTTPACTION: %*d,%d", &response_code) != 1)
    {
        Serial.println("Failed to extract HTTP response code.");
        return false;
    }

    // Check if the HTTP response code indicates success (200)
    if (response_code != 200)
    {
        Serial.println("HTTP GET request failed with response code: " + String(response_code));
        return false;
    }

    // Read HTTP response
    if (sendCommand("AT+HTTPREAD=0,500", "OK", 10000) != ErrorCode::NO_ERROR)
    {
        Serial.println("Failed to read HTTP response.");
        return false;
    }

    // Terminate HTTP service
    if (sendCommand("AT+HTTPTERM", "OK", 1000) != ErrorCode::NO_ERROR)
    {
        Serial.println("Failed to terminate HTTP service.");
        return false;
    }

    return true; // HTTP GET request successful
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
 * @brief Sends a command to the GSM module and waits for a response.
 *
 * @param cmd The command to send.
 * @param response The expected response from the GSM module.
 * @param timeout The maximum time to wait for the response (in milliseconds).
 * @return Returns an error code indicating the result of the operation.
 */
int SIM_A7670C::sendCommand(const char *cmd, const char *response, int timeout)
{
    gsmSerial.flush();                   // Clear any previous input.
    unsigned long start_time = millis(); // Stores the start time
    gsmSerial.println(cmd);              // Send command to GSM module
    Serial.println(cmd);

    int index = 0; // Index to keep track of the position in the buffer

    // Clear the buffer to ensure no residual data
    memset(gsmResponse, '\0', sizeof(gsmResponse));

    while (millis() - start_time < timeout)
    {
        while (gsmSerial.available())
        {
            char c = gsmSerial.read(); // Read a character from the serial buffer

            // Character is alphanumeric, a space, or a punctuation character
            if (isalnum(c) || isspace(c) || ispunct(c))
            {
                gsmResponse[index++] = c;  // Store the character in the buffer
                gsmResponse[index] = '\0'; // Null-terminate the string

                // Check if the response buffer contains the expected response string
                if (strstr(gsmResponse, response) != nullptr)
                {
                    trim(gsmResponse);           // Trim whitespace from the response
                    Serial.println(gsmResponse); // Print the response to serial monitor
                    return ErrorCode::NO_ERROR;  // Command executed successfully
                }

                // Check if the buffer is about to overflow
                if (index >= sizeof(gsmResponse) - 1)
                {
                    // Buffer overflow, return error code
                    return ErrorCode::BUFFER_OVERFLOW_ERROR;
                }
            }
        }
    }
    return ErrorCode::TIMEOUT_ERROR; // Timeout waiting for response
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
