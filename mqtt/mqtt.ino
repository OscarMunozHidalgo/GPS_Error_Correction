/* ---------------------------------------------------------------------
 *  Ejemplo MKR1310_LoRa_SendReceive_WithCallbacks
 *  Práctica 3
 *  Asignatura (GII-IoT)
 *  
 *  Basado en el ejemplo LoRaDuplexCallback de la librería 
 *  LoRa demuestra cómo es posible resolver la recepción  
 *  de mensajes de forma asíncrona.
 *  
 *  Este ejemplo requiere de una versión modificada
 *  de la librería Arduino LoRa (descargable desde 
 *  CV de la asignatura.
 *  
 *  También usa la librería Arduino_BQ24195 
 *  https://github.com/arduino-libraries/Arduino_BQ24195
 * ---------------------------------------------------------------------
 */

#include <SPI.h>             
#include <LoRa.h>
#include <Arduino_PMIC.h>
#include <ArduinoJson.h>

#define TX_LAPSE_MS          10000

// NOTA: Ajustar estas variables 
const uint8_t localAddress = 0x23;     // Dirección de este dispositivo
uint8_t destination = 0x22;            // Dirección de destino, 0xFF es la dirección de broadcast

JsonDocument doc;

struct GNSSPacket {
  long lat;
  long lon;
  byte siv;
};

// --------------------------------------------------------------------
// Setup function
// --------------------------------------------------------------------
void setup() 
{
  Serial.begin(9600);  
  while (!Serial); 

  //Serial.println("LoRa Duplex with callback");

  // Es posible indicar los pines para CS, reset e IRQ pins (opcional)
  // LoRa.setPins(csPin, resetPin, irqPin);// set CS, reset, IRQ pin


  if (!LoRa.begin(868E6)) {      // Initicializa LoRa a 868 MHz
    Serial.println("LoRa init failed. Check your connections.");
    while (true);                
  }

  // Configuramos algunos parámetros de la radio
  LoRa.setSignalBandwidth(125E3); // 7.8E3, 10.4E3, 15.6E3, 20.8E3, 31.25E3
                                  // 41.7E3, 62.5E3, 125E3, 250E3, 500E3 
                                  // Multiplicar por dos el ancho de banda
                                  // supone dividir a la mitad el tiempo de Tx
  LoRa.setSpreadingFactor(7);     // [6, 12] Aumentar el spreading factor incrementa 
                                  // de forma significativa el tiempo de Tx
                                  // SPF = 6 es un valor especial
                                  // Ver tabla 12 del manual del SEMTECH SX1276
  LoRa.setSyncWord(0x12);         // Palabra de sincronización privada por defecto para SX127X 
                                  // Usaremos la palabra de sincronización para crear diferentes
                                  // redes privadas por equipos
  LoRa.setCodingRate4(5);         // [5, 8] 5 da un tiempo de Tx menor
  LoRa.setPreambleLength(8);      // Número de símbolos a usar como preámbulo

  LoRa.setTxPower(3, PA_OUTPUT_PA_BOOST_PIN); // Rango [2, 20] en dBm
                                  // Importante seleccionar un valor bajo para pruebas
                                  // a corta distancia y evitar saturar al receptor

  // Indicamos el callback para cuando se reciba un paquete
  LoRa.onReceive(onReceive);

  // Nótese que la recepción está activada a partir de este punto
  LoRa.receive();
  
  //Serial.println("LoRa init succeeded\n");
}

// --------------------------------------------------------------------
// Loop function
// --------------------------------------------------------------------
void loop() 
{
  
}

// --------------------------------------------------------------------
// Sending message function
// --------------------------------------------------------------------
void sendMessage(char* outgoing, uint8_t msgLength, uint16_t &msgCount) 
{
  LoRa.beginPacket();                     // Comenzamos el empaquetado del mensaje
  LoRa.write(destination);                // Añadimos el ID del destinatario
  LoRa.write(localAddress);               // Añadimos el ID del remitente
  LoRa.write((uint8_t)(msgCount >> 7));   // Añadimos el Id del mensaje (MSB primero)
  LoRa.write((uint8_t)(msgCount & 0xFF)); 
  LoRa.write(msgLength);                  // Añadimos la longitud en bytes del mensaje
  LoRa.print(outgoing);                   // Añadimos el mensaje/payload
  
  // OJO: endPacket(), equivalente a endPacket(false) no vuelve hasta que 
  // la transmisión ha concluido. Por contra, endPacket(true) retorna inmediatamente
  LoRa.endPacket();                       // Finalizamos el paquete y lo enviamos
  msgCount++;                             // Incrementamos el contador de mensajes
}

// --------------------------------------------------------------------
// Receiving message function
// --------------------------------------------------------------------
void onReceive(int packetSize) 
{
  if (packetSize == 0) return;          // Si no hay mensajes, retornamos

  // Leemos los primeros bytes del mensaje
  char buffer[50];                      // Buffer para almacenar el mensaje
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
  buffer[receivedBytes] = '\0';         // Terminamos la cadena

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
    return;
  }

  GNSSPacket receivedPacket;
  memcpy(&receivedPacket, buffer, sizeof(GNSSPacket));

  doc["from"] = String(sender, HEX);
  doc["to"] = String(recipient, HEX);
  doc["id"] = String(incomingMsgId);
  doc["len"] = String(incomingLength);
  doc["lat"] = receivedPacket.lat;
  doc["lon"] = receivedPacket.lon;
  doc["SIV"] = receivedPacket.siv;
  doc["RSSI"] = String(LoRa.packetRssi());
  doc["SNR"] = String(LoRa.packetSnr());

  serializeJson(doc, Serial);

  // Imprimimos los detalles del mensaje recibido
  /*Serial.println("Received from: 0x" + String(sender, HEX));
  Serial.println("Sent to: 0x" + String(recipient, HEX));
  Serial.println("Message ID: " + String(incomingMsgId));
  Serial.println("Message length: " + String(incomingLength));
  Serial.print("Lat: ");
  Serial.println(receivedPacket.lat);
  Serial.print("Lon: ");
  Serial.println(receivedPacket.lon);
  Serial.print("SIV: ");
  Serial.println(receivedPacket.siv);
  Serial.print("RSSI: " + String(LoRa.packetRssi()));
  Serial.println(" dBm\nSNR: " + String(LoRa.packetSnr()));
  Serial.println();*/
}
