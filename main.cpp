#include "mbed.h"
#include "easy-connect.h"
#include "MQTTNetwork.h"
#include "MQTTmbed.h"
#include "MQTTClient.h"
#include "credentials.h"

AnalogIn tempSensor1(p20);
AnalogIn tempSensor2(p18);

bool successSent = false;

float getTemp(float raw){
    return (9.0*(((raw*3.3)-0.500)*100.0))/5.0 + 32.0;
}

void messageArrived(MQTT::MessageData& md)
{
    MQTT::Message &message = md.message;
    printf("Message arrived: qos %d, retained %d, dup %d, packetid %d\r\n", message.qos, message.retained, message.dup, message.id);
    printf("Payload %.*s\r\n", message.payloadlen, (char*)message.payload);
    successSent = true;
}

int main(){
    extern const char* hostname;
    extern const int port;
    extern char* username;
    extern char* password;

    NetworkInterface* network = easy_connect(true); 
    MQTTNetwork mqttNetwork(network);

    MQTT::Client<MQTTNetwork, Countdown> client(mqttNetwork);
    printf("Connecting to %s:%d\r\n", hostname, port);
    int rc = mqttNetwork.connect(hostname, port);
    if (rc != 0)
        printf("rc from TCP connect is %d\r\n", rc);

    MQTTPacket_connectData data = MQTTPacket_connectData_initializer;
    data.MQTTVersion = 3;

    char clientID[100];
    strcpy(clientID, "mbed-");
    strcat(clientID, network->get_mac_address());

    data.clientID.cstring = clientID;
    data.username.cstring = username;
    data.password.cstring = password;
    if ((rc = client.connect(data)) != 0)
        printf("rc from MQTT connect is %d\r\n", rc);

    if ((rc = client.subscribe("temperature", MQTT::QOS2, messageArrived)) != 0)
        printf("rc from MQTT subscribe is %d\r\n", rc);

    MQTT::Message message;
    message.qos = MQTT::QOS0;
    message.retained = false;
    message.dup = false;
    
    while(true){
        // Take 10 readings
        float temperature = 0.0;
        for(int i = 0; i < 10; i++){
            temperature += getTemp(tempSensor1);
            temperature += getTemp(tempSensor2);
            Thread::wait(500);
        }
        int final_temperature = (temperature/20.0);
        printf("temperature: %d\r\n", final_temperature);
        char buf[100];
        sprintf(buf, "%d", final_temperature);
        message.payload = (void*)buf;
        message.payloadlen = strlen(buf)+1;
        client.publish("temperature", message);
        
        int tries = 0;
        while(!successSent && tries < 10){
            client.yield();
            tries++;
        }
        successSent = false;

    }

    return 0;
}
