/* ======= Command format ======== *
 *
 * [MessageType];[PumpCommand];to=[node id];s=[speed];p=[position]
 *                            ^-------command parameters---------^
 *
 *  param       range               notes
 *  -----       -----               -----
 *  MessageType described below
 *  PumpCommand described below
 *  to          1-15                see config.h
 *  s           0.0 - 4000.0
 *  p           -1E6 - 1E6          limit switches ensure pump will not exceed max/min position
 *
 * ========= Message Type ======== *
 *  header      action
 *  ------      ------
 *  S           Send command to single pump
 *  M           Send command to all pumps in network
 *
 * ==== Pump Command headers ===== *
 *  header      action
 *  ------      ------
 *  Q           Adds command to queue
 *  G           Immediately executes command
 *  F           Finds max position
 *  B           Finds home position
 *  R           Run first command in queue
 *  C           Runs to relative position
 *  G           Runs to absolute position
 *  H           Home
 *  S           Stop
 *  L           Get max length
 *  P           Get position
 *  M           Set max length
 *  N           Set position
 *  Q           Add to Queue
 *  X           Clear Queue
 *
 */

#include <Arduino.h>
#include <RF24Network.h>
#include <RF24.h>
#include <QueueArray.h>
#include <main.h>
#include <config.h>

RF24 radio(cePin, csPin);
RF24Network network(radio);
QueueArray<CommandToSend> networkCommandQueue;
QueueArray<ReceivedData> dataQueue;

unsigned long timeOfLastSerial;

void relayMessageSerial(ReceivedData data);

void sendSerialData();

void setup() {
    Serial.begin(115200);

    // RF24 network setup
    this_node = node_address_set[NODE_ADDRESS];
    radio.begin();
    radio.setPALevel(RF24_PA_LOW);
    radio.setDataRate(RF24_2MBPS);
    network.begin(90, this_node);

}

void loop() {
    network.update();

    // ===============Receiving=============== //

    if (Serial.available()) handleSerial();
    if (network.available()) handleNetwork();

    // ================Sending================ //

    // send the current position over the network if time since last message = NETWORK_THROTTLE and pump is running.

    if (millis() - timeOfLastMessage > NETWORK_THROTTLE) sendNetworkMessage();
    if (millis() - timeOfLastSerial > SERIAL_THROTTLE) sendSerialData();


}



//region Serial Input

void handleSerial() {

    char serialBuffer[50];
    Serial.readBytes(serialBuffer, 50);
    unsigned char header = serialBuffer[0];
    switch (header) {
        case 'S': createCommand(serialBuffer, SINGLE);        break;
        case 'M': createCommand(serialBuffer, MULTI);         break;
        default:  sendSerialError(UNRECOGNIZED_HEADER);       break;
    }
}

//endregion

//region Network input

void handleNetwork() {
    unsigned long timeOfMessage = micros();
    RF24NetworkHeader header;
    long message;
    network.read(header, &message, sizeof(message));
    switch (header.type){
        case 'E': sendSerialError(static_cast<ErrorType>(message)); break;
        default: dataQueue.enqueue({timeOfMessage, header, message}); break;
    }
}

void relayMessageSerial(ReceivedData data) {
    Serial.println(data.toString());
}

//endregion

//region Network output

void sendNetworkMessage() {
    if (!networkCommandQueue.isEmpty()) {
        CommandToSend commandToSend = networkCommandQueue.dequeue();
        bool ok;
        printCommandOverSerial(commandToSend, "Sending command");

        uint16_t to = node_address_set[commandToSend.header.to];
        RF24NetworkHeader header(to, commandToSend.header.type);

        switch (commandToSend.messageType) {
            case SINGLE:
                ok = network.write(header, &commandToSend.command, sizeof(commandToSend.command));
                break;
            case MULTI:
                ok = network.multicast(header, &commandToSend.command, sizeof(commandToSend.command), 1);
                break;
            default:
                break;
        }
        if (ok) {
            Serial.print("Success! Sent ");
            Serial.print(commandToSend.header.type);
            Serial.print(" header to ");
            Serial.println(commandToSend.header.to);
        }
        else    {
            Serial.print("Error sending ");
            Serial.print(commandToSend.header.type);
            Serial.print(" header to ");
            Serial.println(commandToSend.header.to);
        }
    }

    timeOfLastMessage = millis();
}

//endregion

//region Parsers

void createCommand(char *message, MessageType type) {
    if (networkCommandQueue.isFull()) sendSerialError(NETWORK_QUEUE_FULL);
    CommandToSend commandToSend = parseCommand(message);
    if (type == MULTI) commandToSend.header.to = 01;
    commandToSend.messageType = type;
    networkCommandQueue.push(commandToSend);

    // Uncomment below for debugging
    //printCommandOverSerial(commandToSend, "Pushed command");
}

CommandToSend parseCommand(char *buffer) {
    unsigned char type = buffer[2];
    CommandToSend parsedCommand;
    parsedCommand.header.type = type;

    char *s;
    s = strtok(buffer, ";");
    while (s){
        char* equalsSign = strchr(s, '=');
        if (equalsSign) {
            *equalsSign = 0;
            equalsSign ++;
            if (0 == strcmp(s, "t")) {
                parsedCommand.header.to = strtol(equalsSign, &equalsSign, 10);
                Serial.print("t = ");
                Serial.println(parsedCommand.header.to);
            }
            if (0 == strcmp(s, "p")) {
                parsedCommand.command.goTo = strtol(equalsSign, &equalsSign, 10);
                Serial.print("p = ");
                Serial.println(parsedCommand.command.goTo);
            }
            if (0 == strcmp(s, "s")) {
                parsedCommand.command.speed = strtod(equalsSign, &equalsSign);
                Serial.print("s = ");
                Serial.println(parsedCommand.command.speed);
            }
        }
        s = strtok(nullptr, ";");
    }
    //printCommandOverSerial(parsedCommand, "Parsed command");
    return parsedCommand;
}

//endregion

//region Serial output

void sendSerialData() {
    if (!dataQueue.isEmpty()) {
        ReceivedData data = dataQueue.dequeue();
        relayMessageSerial(data);
    }
}

void sendSerialError(ErrorType type) {
    switch (type) {
        case PUMP_QUEUE_FULL: Serial.println("Error: pump queue is full!"); break;
        case NETWORK_QUEUE_FULL: Serial.println("Error: network queue is full!"); break;
        case UNRECOGNIZED_HEADER: Serial.println("Error: unrecognized header!"); break;
    }
}

void printCommandOverSerial(CommandToSend commandToSend, const char* messageHeading) {
    Serial.print("---------------");
    Serial.print(messageHeading);
    Serial.println("---------------");
    Serial.print(commandToSend.toString());
}

//endregion
ReceivedData::ReceivedData(unsigned long timeReceived, RF24NetworkHeader header, long message) {
    this->time = timeReceived;
    this->type = header.type;
    this->from = header.from_node;
    this->message = message;
}
