//
// Created by Jonathan Taylor on 10/7/20.
//

#ifndef BASEMODULE_MAIN_H
#define BASEMODULE_MAIN_H




enum MessageType{SINGLE, MULTI};

enum ErrorType{NETWORK_QUEUE_FULL, PUMP_QUEUE_FULL, UNRECOGNIZED_HEADER};

struct Command{
    double          speed{};
    long            goTo{};
};
struct Header{
    uint16_t        to{};
    unsigned char   type{};
};
struct ReceivedData{
    ReceivedData(unsigned long timeReceived, RF24NetworkHeader header, long message);

    unsigned long time{};
    long message{};
    uint16_t from{};
    const char *type{};
};

struct CommandToSend{
    Command         command{};
    Header          header{};
    MessageType     messageType{};
};

void handleSerial();

CommandToSend parseCommand(char *buffer);

void createCommand(char *message, MessageType type);

void sendNetworkMessage();

void handleNetwork();

void sendSerialError(ErrorType type);

void printCommandOverSerial(CommandToSend commandToSend, const char *messageHeading);

unsigned long timeOfLastMessage = 0;

#endif //BASEMODULE_MAIN_H
