#include "Arduino.h"
#include "SIM_A7670C.h"

// Constructor for SoftwareSerial
SIM_A7670C::SIM_A7670C(SoftwareSerial &serial) : gsmSerial(serial) {}

// Constructor for HardwareSerial
SIM_A7670C::SIM_A7670C(HardwareSerial &serial) : gsmSerial(serial) {}

/**
 * Initializes the GSM module with a series of AT commands.
 *
 * This method configures the GSM module for operation by sending a sequence of
 * AT commands to set up various functionalities like SMS format, caller ID presentation,
 * and automatic time zone updates. It also checks network signal strength and cleans
 * up any existing SMS messages to ensure the module is ready for use.
 *
 * @return bool True if all commands are executed successfully, indicating the module is ready.
 */
bool SIM_A7670C::connect()
{
    ErrorCode error_code; // Variable to hold the error code for each command execution.

    // Array of AT commands to be sent to the GSM module.
    const char *commands[] = {
        "ATE0",            // Disables command echo.
        "AT+CLIP=1",       // Enables caller line identification.
        "AT+CVHU=0",       // Turns off result code presentation.
        "AT+CTZU=1",       // Enables automatic time zone update.
        "AT+CMGF=1",       // Sets SMS format to text mode.
        "AT+CNMI=0,0,0,0", // Disables new message indications.
        "AT+CSQ",          // Checks network signal strength.
        "AT+CMGD=1,4"      // Deletes all SMS messages.
    };

    // Iterate through the commands and send each to the GSM module.
    for (const char *cmd : commands)
    {
        if (!sendCommand(cmd, "OK", 1000, error_code))
        {
            // If a command fails, return false.
            return false;
        }
    }

    // If all commands were executed successfully, return true.
    return true;
}

/**
 * Checks the GSM network registration status.
 *
 * This method sends an AT command to query the GSM module's network registration status and parses
 * the response to determine if the module is successfully registered on the network.
 *
 * @param status Reference to an integer where the registration status will be stored.
 * @return bool True if the module is successfully registered on the network, false otherwise.
 */
bool SIM_A7670C::registerNetwork(int &status)
{
    ErrorCode error_code; // Placeholder for error code from sendCommand, if needed for debugging.

    // Send AT command to query the GSM network registration status.
    if (!sendCommand("AT+CREG?", "OK", 5000, error_code))
    {
        // If command fails or no response received, return false.
        return false;
    }

    // Look for the "+CREG" response prefix in the received response.
    char *response = strstr(gsmResponse, "+CREG:");
    if (response == nullptr)
    {
        // If the expected "+CREG" prefix is not found, response format is unexpected.
        return false;
    }

    // Parse the response to extract the registration status.
    if (sscanf(response, "+CREG: %*d,%d", &status) != 1)
    {
        // If parsing fails, the response may not include a valid status.
        return false;
    }

    // Check if the registration status indicates successful registration.
    // According to 3GPP TS 27.007, the statuses 1, 5, and 6 indicate registered states.
    switch (status)
    {
    case 1:          // Registered, home network
    case 5:          // Registered, roaming
    case 6:          // Registered for "SMS only", home network (applicable in some networks)
        return true; // Successfully registered.

    default:
        // Any other status value indicates not registered or registration denied.
        return false;
    }
}

/**
 * Registers the device on the GPRS network.
 * This method performs several steps to ensure the device is registered on the GPRS network,
 * handling potential errors at each stage by setting an appropriate error code.
 *
 * @param stat Reference to an integer to store the registration status.
 * @param AcT Reference to an integer to store the Access Technology type.
 * @param error_code Reference to an ErrorCode enum to store the error encountered, if any.
 * @return bool True if registration is successful, False otherwise.
 */
bool SIM_A7670C::registerGPRS(int &stat, int &AcT, ErrorCode &error_code)
{
    // Step 1: Query COPS response to extract Access Technology (AcT)
    if (sendCommand("AT+COPS?", "OK", 1000, error_code) != true ||
        sscanf(gsmResponse, "+COPS: %*d,%*d,\"%*[^\"]\",%d", &AcT) != 1)
    {
        error_code = OPERATOR_ERROR;
        return false; // Failed to get operator info or AcT
    }

    // Step 2: Check CGATT status for packet domain registration
    if (sendCommand("AT+CGATT?", "OK", 1000, error_code) != true ||
        sscanf(gsmResponse, "+CGATT: %d", &stat) != 1 || stat != 1)
    {
        error_code = PACKET_DOMAIN_ERROR;
        return false; // Packet domain not attached
    }

    // Step 3: Activate PDP context for data transmission
    if (sendCommand("AT+CGACT=1,1", "OK", 5000, error_code) != true)
    {
        error_code = PDP_CONTEXT_ERROR;
        return false; // Failed to activate PDP context
    }

    // Step 4: Query CGREG response for registration status and AcT
    if (sendCommand("AT+CGREG?", "OK", 5000, error_code) != true ||
        sscanf(gsmResponse, "+CGREG: %*d,%d,,,%d", &stat, &AcT) < 1)
    {
        error_code = GPRS_NETWORK_ERROR;
        return false; // Failed to get GPRS network registration status
    }

    // Successfully registered on the GPRS network
    error_code = NO_ERROR;
    return true;
}

/**
 * Perform an HTTP GET request to the specified URL and port.
 *
 * This method handles the complete process of initializing the HTTP service,
 * configuring the request, sending it, and then reading the response. It ensures
 * that resources are cleanly managed by terminating the HTTP service even if an
 * error occurs at any step in the process.
 *
 * @param url The URL for the HTTP GET request.
 * @param port The port number to use for the request.
 * @param response_code Reference to store the HTTP response code received.
 * @param error_code Reference to store an error code indicating the operation's success or failure.
 * @return bool True if the request was successfully sent and a response received; false otherwise.
 */
bool SIM_A7670C::httpGET(const char *url, const int port, int &response_code, ErrorCode &error_code)
{
    char commandBuffer[255]; // Buffer to hold full command strings.

    // Terminate any existing HTTP service for a clean state.
    if (!sendCommand("AT+HTTPTERM", "OK", 1000, error_code))
    {
        // Note: Not critical if this fails, but you can consider logging this event.
    }

    // Initialize HTTP service. Exit if this fails.
    if (!sendCommand("AT+HTTPINIT", "OK", 1000, error_code))
    {
        error_code = HTTP_INIT_ERROR;
        return false;
    }

    // Set URL for the GET request.
    snprintf(commandBuffer, sizeof(commandBuffer), "AT+HTTPPARA=\"URL\",\"%s:%d\"", url, port);
    if (!sendCommand(commandBuffer, "OK", 1000, error_code))
    {
        error_code = URL_CONFIG_ERROR;
        goto terminateHttpService; // Use goto for cleanup to avoid repetition.
    }

    // Set Content-Type header. This example assumes all requests are plaintext.
    if (!sendCommand("AT+HTTPPARA=\"CONTENT\",\"text/plain\"", "OK", 1000, error_code))
    {
        error_code = CONTENT_TYPE_ERROR;
        goto terminateHttpService;
    }

    // Perform the HTTP GET request.
    if (!sendCommand("AT+HTTPACTION=0", "+HTTPACTION:", 20000, error_code))
    {
        error_code = HTTP_ACTION_ERROR;
        goto terminateHttpService;
    }

    // Extract HTTP response code.
    if (sscanf(gsmResponse, "+HTTPACTION: %*d,%d", &response_code) != 1)
    {
        error_code = HTTP_RESPONSE_CODE_ERROR;
        goto terminateHttpService;
    }

    // Check for HTTP 200 OK. If not, don't attempt to read the response.
    if (response_code != 200)
    {
        error_code = HTTP_RESPONSE_CODE_ERROR;
        goto terminateHttpService;
    }

    // Read HTTP response content.
    if (!sendCommand("AT+HTTPREAD=0,500", "OK", 10000, error_code))
    {
        error_code = HTTP_READ_ERROR;
        goto terminateHttpService;
    }

    // Success path.
    error_code = NO_ERROR;
    return true;

terminateHttpService:
    // Terminate HTTP service. Even if previous steps fail, always try to terminate.
    sendCommand("AT+HTTPTERM", "OK", 1000, error_code);
    return false;
}

/**
 * Reads the first unread SMS message from the module's storage.
 *
 * This method sends an AT command to list all unread SMS messages, then processes
 * the response to extract the first unread message. It dynamically allocates memory
 * for the SMS content, which the caller is responsible for freeing.
 *
 * Note: The method automatically attempts to delete all read and unread SMS messages
 * after processing to prevent memory overflow in the module.
 *
 * @return char* A pointer to a dynamically allocated string containing the SMS message.
 *               Returns nullptr if no unread messages are available or in case of errors.
 */
char *SIM_A7670C::readSMS()
{
    ErrorCode error_code; // Placeholder for error code from sendCommand, if needed for debugging.

    // Send command to list all unread SMS messages.
    if (!sendCommand("AT+CMGL=\"REC UNREAD\"", "OK", 5000, error_code))
    {
        return nullptr; // Command failed or no unread messages.
    }

    // Find the start of the first SMS message, if any.
    char *startOfSMS = strchr(gsmResponse, '\n');
    if (startOfSMS == nullptr)
    {
        return nullptr; // No SMS content found.
    }
    startOfSMS++; // Move past the newline character.

    // Ensure the message does not exceed predefined limits.
    char *endOfSMS = strstr(startOfSMS, "\r\nOK");
    if (endOfSMS == nullptr || endOfSMS - startOfSMS >= MAX_SMS_LENGTH)
    {
        return nullptr; // No end found or message too long.
    }

    // Allocate memory for the message, plus null terminator.
    size_t messageLength = endOfSMS - startOfSMS;
    char *smsMessage = (char *)malloc(messageLength + 1);
    if (smsMessage == nullptr)
    {
        return nullptr; // Failed to allocate memory.
    }

    // Copy the message into the newly allocated buffer.
    strncpy(smsMessage, startOfSMS, messageLength);
    smsMessage[messageLength] = '\0'; // Null-terminate the string.

    // Optional: Clean up by deleting all SMS to prevent overflow.
    // Consider the implications of automatically deleting messages in your use case.
    sendCommand("AT+CMGD=1,4", "OK", 5000, error_code); // Delete all SMS messages.

    return smsMessage; // Return the dynamically allocated message.
}

/**
 * Send an SMS message to a specified phone number.
 *
 * This method constructs and sends an AT command to initiate the SMS sending process,
 * followed by the message itself and the Ctrl+Z character to indicate the end of the message.
 * It then checks the module's response to verify successful transmission.
 *
 * @param phoneNumber The recipient's phone number as a character array.
 * @param message The SMS message to be sent as a character array.
 * @return bool True if the SMS was sent successfully, false otherwise.
 */
bool SIM_A7670C::sendSMS(const char *phoneNumber, const char *message)
{
    ErrorCode error_code; // Placeholder for error code from sendCommand, if needed for debugging.

    // Ensure phone number and message are not null pointers.
    if (phoneNumber == nullptr || message == nullptr)
    {
        return false;
    }

    // Construct the AT command to send SMS.
    char commandBuffer[50];
    snprintf(commandBuffer, sizeof(commandBuffer), "AT+CMGS=\"%s\"", phoneNumber);

    // Send the AT command to initiate SMS sending.
    if (!sendCommand(commandBuffer, '>', 5000, error_code))
    {
        return false;
    }

    // Send the SMS message.
    gsmSerial.print(message);

    // Send the Ctrl+Z character (0x1A) to indicate the end of the message.
    gsmSerial.write(0x1A);

    // Wait for the module to confirm SMS has been sent.
    if (!sendCommand("", "OK", 10000, error_code))
    {
        return false;
    }

    // Confirm successful SMS transmission to the user.
    return true;
}

/**
 * Send a command to the GSM module and wait for a specified response.
 *
 * This method sends a command to the GSM module via serial and waits for a response that includes a specified
 * keyword or phrase. It implements timeout logic to prevent indefinite blocking if the expected response is not received.
 *
 * @param cmd The command string to send to the GSM module.
 * @param response The response string to wait for.
 * @param timeout The maximum time to wait for a response, in milliseconds.
 * @param error_code Reference to store the error code encountered during command execution.
 * @return bool True if the expected response is received within the timeout period, false otherwise.
 */
bool SIM_A7670C::sendCommand(const char *cmd, const char *response, int timeout, ErrorCode &error_code)
{
    gsmSerial.flush();      // Clear any previous input.
    gsmSerial.println(cmd); // Send command to GSM module.

    if (showATcommands)
    {
        Serial.print(F("CMD > "));
        Serial.println(cmd); // Echo command to debug serial.
    }

    unsigned long start_time = millis();
    int index = 0; // Index for tracking position in response buffer.

    memset(gsmResponse, 0, sizeof(gsmResponse)); // Clear response buffer.

    while (millis() - start_time < timeout)
    {
        if (gsmSerial.available())
        {
            char c = gsmSerial.read();

            // Store valid characters in buffer.
            if (isprint(c) || c == '\r' || c == '\n')
            {
                if (index < sizeof(gsmResponse) - 1)
                {
                    gsmResponse[index++] = c;  // Save character.
                    gsmResponse[index] = '\0'; // Ensure null-termination.
                }
                else
                {
                    // Prevent buffer overflow.
                    error_code = BUFFER_OVERFLOW_ERROR;
                    return false;
                }
            }

            // Check for the expected response in the buffer.
            if (strstr(gsmResponse, response) != nullptr)
            {
                trim(gsmResponse);
                if (showATcommands)
                {
                    Serial.print(F("RESPONSE > "));
                    Serial.println(gsmResponse);
                }
                error_code = NO_ERROR;
                return true;
            }
        }
    }

    // If we reach here, the command has timed out.
    error_code = TIMEOUT_ERROR;
    return false;
}

/**
 * Get the last response received from the GSM module.
 *
 * This method provides access to the internal buffer that stores the last response received from the GSM module.
 * It allows users of the library to retrieve the content of the response directly, which can be useful for
 * custom parsing or logging purposes.
 *
 * @return A pointer to a null-terminated string containing the last response from the GSM module.
 */
const char *SIM_A7670C::getResponse() const
{
    return gsmResponse;
}

/**
 * Trims leading and trailing whitespace from a C-string in place.
 *
 * This function moves through the string to remove any whitespace characters
 * from the beginning and end of the string. It modifies the original string
 * by shifting non-whitespace characters forward and inserting a null terminator
 * to mark the new end of the string.
 *
 * @param str A pointer to the string to be trimmed.
 */
void SIM_A7670C::trim(char *str)
{
    // Pointer to the start of the string, used to find the first non-whitespace character.
    char *start = str;
    while (*start && isspace((unsigned char)*start)) // Ensure unsigned char for correct isspace behavior.
    {
        start++;
    }

    // If the entire string was whitespace, return an empty string.
    if (*start == 0)
    {
        *str = '\0';
        return;
    }

    // Pointer to the end of the string, used to find the last non-whitespace character.
    char *end = start + strlen(start) - 1;
    while (end > start && isspace((unsigned char)*end)) // Ensure unsigned char for correct isspace behavior.
    {
        *end-- = '\0';
    }

    // Shift the trimmed string to the beginning if necessary.
    if (start > str)
    {
        memmove(str, start, end - start + 1);
        str[end - start + 1] = '\0'; // Null-terminate the shifted string.
    }
}
