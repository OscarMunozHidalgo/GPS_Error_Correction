#include <SPI.h>             
#include <LoRa.h>
#include <Arduino_PMIC.h>
#include <Wire.h> //Needed for I2C to GNSS
#include <SparkFun_u-blox_GNSS_Arduino_Library.h> //http://librarymanager/All#SparkFun_u-blox_GNSS


#define TX_LAPSE_MS          2000

// NOTA: Ajustar estas variables 
const uint8_t localAddress = 0x32;     // Dirección de este dispositivo
uint8_t destination = 0x23;            // Dirección de destino, 0xFF es la dirección de broadcast

volatile bool txDoneFlag = true;       // Flag para indicar cuando ha finalizado una transmisión
volatile bool transmitting = false;

SFE_UBLOX_GNSS myGNSS;

// Estructura para almacenar la configuración de la radio
typedef struct {
  uint8_t bandwidth_index;
  uint8_t spreadingFactor;
  uint8_t codingRate;
  uint8_t txPower; 
} LoRaConfig_t;

double bandwidth_kHz[10] = {7.8E3, 10.4E3, 15.6E3, 20.8E3, 31.25E3,
                            41.7E3, 62.5E3, 125E3, 250E3, 500E3 };

LoRaConfig_t thisNodeConf   = { 8, 7, 5, 2};

volatile bool sendNow = false;

typedef struct{
  long latitude ;
  long longitude ;
  byte SIV ;
} Coordinates;

Coordinates coordinates;
Coordinates error;

// --------------------------------------------------------------------
// Setup function
// --------------------------------------------------------------------
void setup() 
{
  Serial.begin(115200);  
  while (!Serial); 
  Wire.begin();

  if (myGNSS.begin() == false) //Connect to the u-blox module using Wire port
  {
    Serial.println(F("u-blox GNSS not detected at default I2C address. Please check wiring. Freezing."));
    while (1);
  }

  Serial.println("LoRa Duplex with TxDone and Receive callbacks");
  Serial.println("Using binary packets");
  
  // Es posible indicar los pines para CS, reset e IRQ pins (opcional)
  // LoRa.setPins(csPin, resetPin, irqPin);// set CS, reset, IRQ pin

  
  if (!init_PMIC()) {
    Serial.println("Initilization of BQ24195L failed!");
  }
  else {
    Serial.println("Initilization of BQ24195L succeeded!");
  }

  if (!LoRa.begin(868E6)) {      // Initicializa LoRa a 868 MHz
    Serial.println("LoRa init failed. Check your connections.");
    while (true);                
  }

  // Configuramos algunos parámetros de la radio
  LoRa.setSignalBandwidth(long(bandwidth_kHz[thisNodeConf.bandwidth_index])); 
                                  // 7.8E3, 10.4E3, 15.6E3, 20.8E3, 31.25E3
                                  // 41.7E3, 62.5E3, 125E3, 250E3, 500E3 
                                  // Multiplicar por dos el ancho de banda
                                  // supone dividir a la mitad el tiempo de Tx
                                  
  LoRa.setSpreadingFactor(thisNodeConf.spreadingFactor);     
                                  // [6, 12] Aumentar el spreading factor incrementa 
                                  // de forma significativa el tiempo de Tx
                                  // SPF = 6 es un valor especial
                                  // Ver tabla 12 del manual del SEMTECH SX1276
  
  LoRa.setCodingRate4(thisNodeConf.codingRate);         
                                  // [5, 8] 5 da un tiempo de Tx menor
                                  
  LoRa.setTxPower(thisNodeConf.txPower, PA_OUTPUT_PA_BOOST_PIN); 
                                  // Rango [2, 20] en dBm
                                  // Importante seleccionar un valor bajo para pruebas
                                  // a corta distancia y evitar saturar al receptor
  LoRa.setSyncWord(0x12);         // Palabra de sincronización privada por defecto para SX127X 
                                  // Usaremos la palabra de sincronización para crear diferentes
                                  // redes privadas por equipos
  LoRa.setPreambleLength(8);      // Número de símbolos a usar como preámbulo

  
  // Indicamos el callback para cuando se reciba un paquete
  LoRa.onReceive(onReceive);
  
  // Activamos el callback que nos indicará cuando ha finalizado la 
  // transmisión de un mensaje
  LoRa.onTxDone(TxFinished);

  // Nótese que la recepción está activada a partir de este punto
  LoRa.receive();

  Serial.println("LoRa init succeeded.\n");
}

// --------------------------------------------------------------------
// Loop function
// --------------------------------------------------------------------
void loop() 
{
  static uint32_t lastSendTime_ms = 0;
  static uint16_t msgCount = 0;
  static uint32_t txInterval_ms = TX_LAPSE_MS;
  static uint32_t tx_begin_ms = 0;

  if (sendNow && !transmitting && ((millis() - lastSendTime_ms) > txInterval_ms)) {
    sendNow = false;
    uint8_t payload[50];       // Array para almacenar el payload
    uint8_t payloadLength = 0; // Longitud inicial del payload

    long lat = myGNSS.getLatitude();
    long lon = myGNSS.getLongitude();

    Serial.print("Lat medida: "); Serial.println(lat);
    Serial.print("Lon medida: "); Serial.println(lon);
    Serial.print("Error Lat medida: "); Serial.println(error.latitude);
    Serial.print("Error Lon medida: "); Serial.println(error.longitude);

    coordinates.latitude = lat - error.latitude;
    coordinates.longitude = lon - error.longitude;

    error.latitude = 0;
    error.longitude = 0;

    Serial.println(coordinates.latitude);
    Serial.println(coordinates.longitude);

    // Añadimos los bytes de la latitud al payload
    payload[payloadLength++] = coordinates.latitude & 0xFF;          // Byte menos significativo
    payload[payloadLength++] = (coordinates.latitude >> 8) & 0xFF;
    payload[payloadLength++] = (coordinates.latitude >> 16) & 0xFF;
    payload[payloadLength++] = (coordinates.latitude >> 24) & 0xFF; // Byte más significativo

    // Añadimos los bytes de la longitud al payload
    payload[payloadLength++] = coordinates.longitude & 0xFF;         // Byte menos significativo
    payload[payloadLength++] = (coordinates.longitude >> 8) & 0xFF;
    payload[payloadLength++] = (coordinates.longitude >> 16) & 0xFF;
    payload[payloadLength++] = (coordinates.longitude >> 24) & 0xFF; // Byte más significativo

    payload[payloadLength++] = coordinates.SIV;

    transmitting = true; // Activamos la transmisión
    txDoneFlag = false;  // Reiniciamos el flag de transmisión
    tx_begin_ms = millis(); // Tiempo en el que se inició la transmisión

    sendMessage(payload, payloadLength, msgCount); // Enviamos el mensaje
    Serial.print("Sending packet ");
    Serial.print(msgCount++);
    Serial.print(": ");
    printBinaryPayload(payload, payloadLength);
  }
                
  if (transmitting && txDoneFlag) {
    uint32_t TxTime_ms = millis() - tx_begin_ms;
    Serial.print("----> TX completed in ");
    Serial.print(TxTime_ms);
    Serial.println(" msecs");
    
    // Ajustamos txInterval_ms para respetar un duty cycle del 1% 
    uint32_t lapse_ms = tx_begin_ms - lastSendTime_ms;
    lastSendTime_ms = tx_begin_ms; 
    float duty_cycle = (100.0f * TxTime_ms) / lapse_ms;
    
    Serial.print("Duty cycle: ");
    Serial.print(duty_cycle, 1);
    Serial.println(" %\n");

    // Solo si el ciclo de trabajo es superior al 1% lo ajustamos
    if (duty_cycle > 1.0f) {
      txInterval_ms = TxTime_ms * 100;
    }
    
    transmitting = false;
    
    // Reactivamos la recepción de mensajes, que se desactiva
    // en segundo plano mientras se transmite
    LoRa.receive();
  }
}

// --------------------------------------------------------------------
// Sending message function
// --------------------------------------------------------------------
void sendMessage(uint8_t* payload, uint8_t payloadLength, uint16_t msgCount) 
{
  while(!LoRa.beginPacket()) {            // Comenzamos el empaquetado del mensaje
    delay(10);                            // 
  }
  LoRa.write(destination);                // Añadimos el ID del destinatario
  LoRa.write(localAddress);               // Añadimos el ID del remitente
  LoRa.write((uint8_t)(msgCount >> 7));   // Añadimos el Id del mensaje (MSB primero)
  LoRa.write((uint8_t)(msgCount & 0xFF)); 
  LoRa.write(payloadLength);              // Añadimos la longitud en bytes del mensaje
  LoRa.write(payload, (size_t)payloadLength); // Añadimos el mensaje/payload 
  LoRa.endPacket(true);                   // Finalizamos el paquete, pero no esperamos a
                                          // finalice su transmisión
}

// --------------------------------------------------------------------
// Receiving message function
// --------------------------------------------------------------------
void onReceive(int packetSize) 
{
  if (transmitting && !txDoneFlag) txDoneFlag = true;
  
  if (packetSize == 0) {
    LoRa.receive();
    return;
  }          // Si no hay mensajes, retornamos

  // Leemos los primeros bytes del mensaje
  uint8_t buffer[10];                   // Buffer para almacenar el mensaje
  int recipient = LoRa.read();          // Dirección del destinatario
  uint8_t sender = LoRa.read();         // Dirección del remitente
                                        // msg ID (High Byte first)
  uint16_t incomingMsgId = ((uint16_t)LoRa.read() << 7) | 
                            (uint16_t)LoRa.read();
  
  uint8_t incomingLength = LoRa.read(); // Longitud en bytes del mensaje
  
  uint8_t receivedBytes = 0;            // Leemos el mensaje byte a byte
  while (LoRa.available() && (receivedBytes < uint8_t(sizeof(buffer)-1))) {            
    buffer[receivedBytes++] = (char)LoRa.read();
  }
  
  if (incomingLength != receivedBytes) {// Verificamos la longitud del mensaje
    Serial.print("Receiving error: declared message length " + String(incomingLength));
    Serial.println(" does not match length " + String(receivedBytes));
    return;                             
  }

  // Verificamos si se trata de un mensaje en broadcast o es un mensaje
  // dirigido específicamente a este dispositivo.
  // Nótese que este mecanismo es complementario al uso de la misma
  // SyncWord y solo tiene sentido si hay más de dos receptores activos
  // compartiendo la misma palabra de sincronización
  if ((recipient & localAddress) != localAddress ) {
    Serial.println("Receiving error: This message is not for me.");
    LoRa.receive();
    return;
  }

  // Imprimimos los detalles del mensaje recibido
  Serial.println("Received from: 0x" + String(sender, HEX));
  Serial.println("Sent to: 0x" + String(recipient, HEX));
  Serial.println("Message ID: " + String(incomingMsgId));
  Serial.println("Payload length: " + String(incomingLength));
  printBinaryPayload(buffer, receivedBytes);

  LoRa.receive();
}

void TxFinished()
{
  txDoneFlag = true;
}

union {
    uint8_t bytes[4];
    long valor; // Usa signed long para admitir valores negativos
} conversor;

void printBinaryPayload(uint8_t * payload, uint8_t payloadLength)
{
  // Verificar que el payload tiene al menos 8 bytes
  if (payloadLength < 8) {
    Serial.println("Error: Payload demasiado corto, no se pueden obtener latitud y longitud.");
    return;
  }

  // Procesar los primeros 4 bytes para la latitud (ajustando endian)
  conversor.bytes[0] = payload[3];
  conversor.bytes[1] = payload[2];
  conversor.bytes[2] = payload[1];
  conversor.bytes[3] = payload[0];
  long Lat = conversor.valor;

  error.latitude = Lat;

  // Procesar los siguientes 4 bytes para la longitud (ajustando endian)
  conversor.bytes[0] = payload[7];
  conversor.bytes[1] = payload[6];
  conversor.bytes[2] = payload[5];
  conversor.bytes[3] = payload[4];
  long Long = conversor.valor;
  error.longitude = Long;

  coordinates.SIV = payload[8];
  sendNow = true;
}
