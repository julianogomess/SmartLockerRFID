#include <SPI.h>        
#include <MFRC522.h>  
#include <ESP8266WiFi.h>
#include <PubSubClient.h>

//Pinos
#define SS_PIN 4
#define RST_PIN 5
#define trava 0
int pinoSensor = 15;
int val = 0;

// definição do leitor RFID
MFRC522 mfrc522(SS_PIN, RST_PIN);   

//Definição do wifi
const char* ssid = "nome do wifi";
const char* password = "senha do wifi";
const char* mqtt_server = "broker.mqtt-dashboard.com"; // broker da sua escolha
WiFiClient espClient;
PubSubClient client(espClient);

//Variaveis auxiliares
long lastMsg = 0;
char msg[50];
int value = 0;
String inString = "";
bool programMode = false;
uint8_t successRead;    
int countC = 0;
bool portaAberta = false;

//Variaveis para guardar salvar cartões
byte sd1[4];
byte sd2[4];
byte sd3[4];
byte sd4[4]; 
byte readCard[4];  


// definição da conexão wifi
void setup_wifi() {
  delay(10);
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  randomSeed(micros());

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

// função para receber os botões via mqtt
void callback(char* topic, byte* payload, unsigned int length) {
  inString="";
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  //Serial.print(payload);
  
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
    inString+=(char)payload[i];
  }
  char a = topic[8];
  bool b = a == 'c';
  Serial.println(b);
  if(b){
    programMode = true;
    client.publish("projeto/ca", " Esperando cartão!!");
  }else{
    client.publish("projeto/sf", " Acesso liberado");
    abrir(4000);
    client.publish("projeto/sf", "Acesso bloqueado");
  }
}
//conectar no broker
void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    String clientId = "ESP8266Client-";
    clientId += String(random(0xffff), HEX);
    // Attempt to connect
    if (client.connect(clientId.c_str())) {
      Serial.println("connected");
      client.subscribe("projeto/trava");
      client.subscribe("projeto/cad");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

//setup inicial
void setup() {
  Serial.begin(115200);   // Inicia a serial
  SPI.begin();      // Inicia  SPI bus
  mfrc522.PCD_Init();   // Inicia MFRC522
  Serial.println("Aproxime o seu cartao do leitor...");
  Serial.println();
  pinMode(trava, OUTPUT);
  pinMode(pinoSensor, INPUT);
  setup_wifi();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);

}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();
  //posição fechada da trava
  digitalWrite(trava, HIGH);
  //leitura de cartão
  if ( ! mfrc522.PICC_IsNewCardPresent()) { 
    return ;
  }
  if ( ! mfrc522.PICC_ReadCardSerial()) {   
    return ;
  }
  Serial.println(F("ID:"));
  for ( uint8_t i = 0; i < 4; i++) {  //
    readCard[i] = mfrc522.uid.uidByte[i];
    Serial.print(readCard[i], HEX);
  }
  Serial.println("");
  mfrc522.PICC_HaltA();
  //leitor de proximidade da porta, ver se esta fechada
  val = digitalRead(pinoSensor);
  client.publish("projeto/sf", "Fechada!");
  if (val == HIGH) {
    portaAberta=true;
    client.publish("projeto/sp", "Aberta!!");
  }else {
    portaAberta=false;
     client.publish("projeto/sp", "Fechada!! Abra a fechadura por cartão ou pelo botão abaixo");
  }
  Serial.println("-----------------");
  if(programMode){
    //modo para cadastrar novos cartões
    if(findID()){
      client.publish("projeto/ca", " Já cadastrado!!");
      }else{
        client.publish("projeto/ca", " Novo!!");
        Serial.println("Novo cartão");
        if(countC==0){
        for ( uint8_t j = 0; j < 4; j++ ) {
          sd1[j]= readCard[j] ;
        }
      }else if(countC == 1){
         for ( uint8_t j = 0; j < 4; j++ ) {
          sd2[j]= readCard[j] ;
        }
      }else if(countC == 2){
         for ( uint8_t j = 0; j < 4; j++ ) {
          sd3[j]= readCard[j] ;
        }
      }else if(countC == 3){
         for ( uint8_t j = 0; j < 4; j++ ) {
          sd4[j]= readCard[j];
        }
        }else{
        Serial.print("Numero maximo de cartões!");
        programMode = false;
        client.publish("projeto/ca", " Esperando cartão!!");
        return;
      }
      countC += 1;
      programMode = false;
      client.publish("projeto/ca", " Para adicionar um cartão clique abaixo!!");
       }
    }
  else{
    //modo para abrir fechadura
   if(findID()){
      Serial.println("Conhecido");
      client.publish("projeto/msg", " Cartão reconhecido!");
      client.publish("projeto/sf", " Acesso liberado por 4 segundos!");
      abrir(4000);
      client.publish("projeto/sf", " Acesso bloqueado!");
      client.publish("projeto/msg", " Esperando cartão!");
    }else{
      client.publish("projeto/msg", " Cartão não cadastrado!!");
    }
  }
  
}

//abrir fechadura por tempo
void abrir ( uint16_t setDelay) {
  digitalWrite(trava, LOW);    
  delay(setDelay);         
  digitalWrite(trava, HIGH);   
  delay(1000);         
}
//checar se o ID é igual
bool checkTwo ( byte a[], byte b[] ) {   
  for ( uint8_t k = 0; k < 4; k++ ) {   // Loop 4 times
    if ( a[k] != b[k] ) {     // IF a != b then false, because: one fails, all fail
       return false;
    }
  }
  return true;  
}
//checar se tem o ID cadastrado
bool findID() {         
    if ( checkTwo( readCard,sd1 ) ) {  
      return true;
    }
    else if ( checkTwo( readCard,sd2 ) ) {  
      return true;
    }
    else if ( checkTwo( readCard,sd3 ) ) {  
      return true;
    }
    if ( checkTwo( readCard,sd4 ) ) {  
      return true;
    }
  
  return false;
}
