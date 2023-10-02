
#include <SDS011.h>
#define TINY_GSM_MODEM_A7
#include <TinyGsmClient.h>
#include <Adafruit_MQTT.h>
#include <Adafruit_MQTT_Client.h>
#include <LiquidCrystal_I2C.h>

#include <Arduino_JSON.h>
#include <WiFi.h>
#include "FirebaseESP32.h"
#include <EEPROM.h>

#include <HardwareSerial.h>
#include <WebServer.h>
#include <WiFiClient.h>

#include <TinyGPS++.h>
TinyGPSPlus gps;
HardwareSerial A7GSM(1);
LiquidCrystal_I2C lcd(0x3F, 20, 4);

//variables
int variable1 = 0;
int variable2 = 0;
int variable3 = 0;

#define pin_on 23    //pin para encender el A7GSM

const char apn[]  = "web.colombiamovil.com.co";   // Datos del operador al que se va a conectar
const char user[] = "";                           // Dejar vacio si no tiene usuario ni contraseña
const char pass[] = "";

/************************* Adafruit.io Setup *********************************/
#define AIO_SERVER      "io.adafruit.com"
#define AIO_SERVERPORT  1883    // con ssl es 8883
#define AIO_USERNAME   "camilosotog"//"alejolopez03"//
#define AIO_KEY   "817af98e39944ddb82bd47dd95a70b3d"// "86a4021f11ed49a68c98167164d0e4c2"    // 

TinyGsm modem(A7GSM);
TinyGsmClient client(modem);

Adafruit_MQTT_Client mqtt(&client, AIO_SERVER, AIO_SERVERPORT, AIO_USERNAME, AIO_KEY);

//FUNCIONES
void MQTT_connect();
void conexioninternet ();
void poweron();       // funcion para encender el modulo A7

Adafruit_MQTT_Publish xP10   = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/feeds/p10");
Adafruit_MQTT_Publish xP25  = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/feeds/p25");
Adafruit_MQTT_Publish xlatitud = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/feeds/latitud");
Adafruit_MQTT_Publish xlongitud = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/feeds/longitud");

//PAGINA

String path = "/Mediciones";

String slatitud;
String slongitud;
String stimestamp;
String sbuffer;

String sP25;
String sP10;

String timestamp;

float  latitud = 5.0899523;
float  longitud = -75.5344814;

FirebaseData firebaseData;

//-------------------VARIABLES GLOBALES--------------------------
int contconexion = 0;
unsigned long previousMillis = 0;

char ssid[50];
char passw[50];
char host[50];
char auth[50];

const char *ssidConf = "CALIDAD_DE_AIRE_IOT";
const char *passConf = "12345678";

String mensaje = "";

String pagina = "<!DOCTYPE html>"
                "<html>"
                "<head>"
                "<title>Calidad de Aire IOT</title>"
                "<meta charset='UTF-8'>"
                "</head>"
                "<body>"
                "<H1>"
                "<center>"
                "</form>"
                "<form action='guardar_conf' method='get'>"
                "<H2> UNIVERSIDAD DE MANIZALES</H2>"
                "<H3> APOYA: RED TECNOPARQUE NODO MANIZALES </H3>"
                "<br><br>"
                "NOMBRE DE LA RED:<br><br>"
                "<input class='input1' name='ssid' type='text' style='height:35px'>"
                "<br><br>"
                "CONTRASEÑA:<br><br>"
                "<input class='input1' name='passw' type='password' style='height:35px'><br><br>"
                "SERVIDOR: calidad-del-aire-f74bd.firebaseio.com<br><br>"
                "<input class='input1' name='host' type='text' style='height:35px'><br><br>"
                "CLAVE SERVIDOR: 2IRFe1vnHLmTkBivYi3VGFEVUII1NMwkPVUsu4Ed<br><br>"
                "<input class='input1' name='auth' type='text' style='height:35px'><br><br>"
                "<input class='boton' type='submit' value='GUARDAR'/style='width:150px;height:45px'><br><br>"
                "</form>"
                "<center>"
                "<a href='escanear'><button class='boton' style='width:150px;height:45px'>ESCANEAR REDES</button></a><br><br></H1>";
String paginafin = "</body>"
                   "</html>";

//--------------------------------------------------------------
WiFiClient espClient;
WebServer server(80);
//--------------------------------------------------------------

//-------------------PAGINA DE CONFIGURACION--------------------
void paginaconf() {
  server.send(200, "text/html", pagina + mensaje + paginafin);
}

//----------------Función para grabar en la EEPROM-------------------
void grabar(int addr, String a) {
  int tamano = a.length();
  char inchar[50];
  a.toCharArray(inchar, tamano + 1);
  for (int i = 0; i < tamano; i++) {
    EEPROM.write(addr + i, inchar[i]);
  }
  for (int i = tamano; i < 50; i++) {
    EEPROM.write(addr + i, 255);
  }
  EEPROM.commit();
}
//---------------------GUARDAR CONFIGURACION-------------------------
void guardar_conf() {

  //Serial.println(server.arg("ssid"));//Recibimos los valores que envia por GET el formulario web
  grabar(0, server.arg("ssid"));
  //Serial.println(server.arg("pass"));
  grabar(50, server.arg("passw"));
  if ( server.arg("host").length() > 0)
  {
    grabar(100, server.arg("host"));
  }
  if ( server.arg("auth").length() > 0)
  {
    grabar(150, server.arg("auth"));
  }

  mensaje = "<center><H1>CONFIGURACIÓN GUARDADA, PRESIONE RESET EN EL DISPOSITIVO...</H1><center>";

  paginaconf();
//  lcd.clear();
//  lcd.setCursor(0, 1);
//  lcd.print("PRESIONA EL BOTON ");
//  lcd.setCursor(0, 2);
//  lcd.print("     RESET...");

}

//---------------------------ESCANEAR----------------------------
void escanear() {
  int n = WiFi.scanNetworks(); //devuelve el número de redes encontradas
  //Serial.println("escaneo terminado");
  if (n == 0) { //si no encuentra ninguna red
    // Serial.println("no se encontraron redes");
    mensaje = "no se encontraron redes";
  }
  else
  {
    //Serial.print(n);
    //Serial.println(" redes encontradas");
    mensaje = "";
    for (int i = 0; i < n; ++i)
    {
      // agrega al STRING "mensaje" la información de las redes encontradas
      mensaje = (mensaje) + "<p>" + String(i + 1) + ": " + WiFi.SSID(i) + " (" + WiFi.RSSI(i) + ") Ch: " + WiFi.channel(i) + " Enc: " + WiFi.encryptionType(i) + " </p>\r\n";
      //WiFi.encryptionType 5:WEP 2:WPA/PSK 4:WPA2/PSK 7:open network 8:WPA/WPA2/PSK
      delay(10);
    }
    //Serial.println(mensaje);
    paginaconf();
  }
}

//--------------------MODO_CONFIGURACION------------------------
void modoconf() {

  delay(100);

  WiFi.softAP(ssidConf, passConf);
  IPAddress myIP = WiFi.softAPIP();
  Serial.print("IP del acces point: ");
  Serial.println(myIP);
  Serial.println("WebServer iniciado...");

  server.on("/", paginaconf); //esta es la pagina de configuracion

  server.on("/guardar_conf", guardar_conf); //Graba en la eeprom la configuracion

  server.on("/escanear", escanear); //Escanean las redes wifi disponibles

  server.begin();

  while (true) {
    server.handleClient();
  }
}

//-----------------Función para leer la EEPROM------------------------
String leer(int addr) {
  byte lectura;
  String strlectura;
  for (int i = addr; i < addr + 50; i++) {
    lectura = EEPROM.read(i);
    if (lectura != 255) {
      strlectura += (char)lectura;
    }
  }
  return strlectura;
}


float p10, p25;
int err;

SDS011 my_sds;

#ifdef ESP32
HardwareSerial port(2);
#endif

void datos();
void setup() {
  my_sds.begin(&port);
  Serial.begin(115200);
  A7GSM.begin(115200, SERIAL_8N1,4,2); //16,17

 delay(1000);
 Serial.println("Iniciando...");
 pinMode(22, OUTPUT);
  //pantalla lcd

  lcd.begin();
  lcd.backlight();
  lcd.clear();
//  lcd.print("  UNIVERSIDAD DE ");
//  lcd.setCursor(0, 1);
//  lcd.print("     MANIZALES");
  lcd.setCursor(0, 1);
  lcd.print(" SENA TECNOPARQUE");
  delay(10000);

  pinMode(5, INPUT); // PULSADOR CONFIGURACION WIFI
  pinMode(14, INPUT);// switch WIFI
  pinMode(15, INPUT);// switch GSM
  pinMode(18, INPUT);// switch GPS

  pinMode(25, OUTPUT);// LED
  pinMode(26, OUTPUT);// LED
  pinMode(27, OUTPUT);// LED

  digitalWrite(25,HIGH);
  digitalWrite(26,HIGH);
  digitalWrite(27,HIGH);
  
  digitalWrite(25,LOW);
  delay(2000);
  digitalWrite(25,HIGH);
  delay(2000);
   
  digitalWrite(26,LOW);
  delay(2000);
  digitalWrite(26,HIGH);
  delay(2000);
    
  digitalWrite(27,LOW);
  delay(2000);
  digitalWrite(27,HIGH);
  delay(2000);
  
//ACTIVAR WIFI
  if (digitalRead(14) == 0) //14 == 0
  {
    variable1 = 1;
    lcd.clear();
    lcd.setCursor(0, 1);
    lcd.print("    WIFI ACTIVADO");
    Serial.println("WIFI ACTIVADO");
    delay(5000);
  }
  else
    variable1 = 0;

//ACTIVAR GSM 
  if (digitalRead(18) == 0)//18 == 1
  {
    variable2 = 1;
    lcd.clear();
    lcd.setCursor(0, 1);
    lcd.print("    GSM ACTIVADO");
    Serial.println("GSM ACTIVADO");
    delay(5000);
  }
  else
    variable2 = 0;

    //ACTIVAR GPS
  if (digitalRead(15) == 0)//15 == 1
  {
    variable3 = 1;
    lcd.clear();
    lcd.setCursor(0, 1);
    lcd.print("    GPS ACTIVADO");
    Serial.println("GPS ACTIVADO");
    delay(5000);
  }
  else
    variable3 = 0;

  //CONFIGURACION WIFI

  EEPROM.begin(512);

  if (digitalRead(5) == 0) {
    lcd.begin();
    lcd.backlight();
    lcd.clear();
    lcd.setCursor(0, 1);
    lcd.print("MODO CONFIGURACION");
    delay(1000);
    lcd.clear();
    lcd.setCursor(0, 1);
    lcd.print("RED WIFI: ");
    lcd.setCursor(0, 2);
    lcd.print("CALIDAD_DE_AIRE_IOT");
    lcd.setCursor(0, 3);
    lcd.print("PASS: 12345678");
    modoconf();
  }

// GUARDAR LOS DATOS EN LA MEMORIA EEPROM
  leer(0).toCharArray(ssid, 50);
  leer(50).toCharArray(passw, 50);
  leer(100).toCharArray(host, 50);
  leer(150).toCharArray(auth, 50);

  Serial.print("host:");
  Serial.println(host);
  Serial.print("auth:");
  Serial.println(auth);

  lcd.clear();
  lcd.setCursor(0, 1);
  lcd.print(" PROYECTO: CALIDAD ");
  lcd.setCursor(0, 2); //Saltamos a la segunda linea
  lcd.print(" DE AIRE IOT ");
  delay(5000);
  
//---------------------------------------------------------------
//CONECTANDO WIFI
  if (variable1 == 1)
  {
    //Serial.println("Conectando...");
    lcd.clear();
    lcd.setCursor(0, 1);
    lcd.print(" CONECTANDO WIFI");
    lcd.setCursor(0, 2); //Saltamos a la segunda linea
    lcd.print(" POR FAVOR ESPERE...");
    while (!Serial) ; //1
    delay(250);
    //  Serial.print("Connecting to ");
    //  Serial.println(ssid);
    WiFi.begin(ssid, passw);

    while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      Serial.print(".");
    }
    //
    lcd.clear();
    lcd.setCursor(0, 1);
    lcd.print("    WIFI CONECTADO");
    Serial.print("IP number assigned by DHCP is ");
    Serial.println(WiFi.localIP());//1
  }

  Firebase.begin(host, auth);

  pinMode(pin_on, OUTPUT);
  digitalWrite(pin_on, LOW);
  
  delay(3000);

// CONECTANDO GSM
  poweron();

 if (variable2 == 1)
  {
    lcd.clear();
    lcd.setCursor(0, 1);
    lcd.print(" CONECTANDO GSM");
    lcd.setCursor(0, 2); //Saltamos a la segunda linea
    lcd.print(" POR FAVOR ESPERE...");
    Serial.println(F("Initializing modem..."));
    modem.restart();
    String modemInfo = modem.getModemInfo();
    Serial.print("Modem: ");
    Serial.println(modemInfo);

    Serial.print(F("Waiting for network..."));
    if (!modem.waitForNetwork()) {
      Serial.println(" fail");
      delay(10000);
      return;
    }
    Serial.println(" OK");

    Serial.print(F("Connecting to "));
    Serial.print(apn);
    if (!modem.gprsConnect(apn, user, pass)) {
      Serial.println(" fail");
      delay(10000);
      return;
    }

    lcd.clear();
    lcd.setCursor(0, 1);
    lcd.print("    GSM CONECTADO");

    Serial.println(" OK");

    mqtt.connect();  //conectar una sola vez a mqtt
    delay(10000);

  }

  //CONECTANDO GPS
 if (variable3 == 1)
  {
  delay(6000); // tiempo para conectarse a la red
  A7GSM.print("AT\r"); // AT OK
  A7GSM.println("");
  delay(100);
  A7GSM.print("AT+GPSRD=1\r");
  // Enciende GPS
  delay(100);
  A7GSM.print("AT+GPS=1\r");

  lcd.clear();
  lcd.setCursor(0, 1);
  lcd.print(" CONECTANDO GPS");
  lcd.setCursor(0, 2); //Saltamos a la segunda linea
  lcd.print(" POR FAVOR ESPERE...");

Serial.print(" POR FAVOR ESPERE...");
  }
}

void loop() {
  
if (variable3 == 1) 
{
  if (Serial.available())
    A7GSM.print((char)Serial.read());
  else if (A7GSM.available())
    Serial.print((char)A7GSM.read());

  while (A7GSM.available() > 0)
    if (gps.encode(A7GSM.read())) {
      if ((gps.location.lat() || gps.location.lng()) != 0) {
        Serial.print("Latitud: ");
        Serial.println(gps.location.lat(), 6);
        Serial.print("Longitud: ");
        Serial.println(gps.location.lng(), 6);
        latitud = gps.location.lat();
        longitud = gps.location.lng();
        
  if (digitalRead(14) == 0)
        {
          variable1 = 1;
        }
        else
          variable1 = 0;

        if (digitalRead(15) == 0)
        {
          variable2 = 1;
        }
        else
          variable2 = 0;
          
 datos();
 
      }

      }
}
 else

 datos();

 
delay(3000);

}

//FUNCION PARA ENCENDER EL MODULO GSM
void poweron() {
  digitalWrite(pin_on, HIGH);
  delay(2000);
  digitalWrite(pin_on, LOW);
}

void conexioninternet () {
  if (WiFi.status()== WL_CONNECTED)
      {   return; }
  else { 
    WiFi.begin(ssid, passw);
      while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
       }
    mqtt.disconnect();
    delay(3000);
    mqtt.connect();  //conectar una sola vez a mqtt
    delay(3000); 
  }
}

void MQTT_connect() {
  int8_t ret;

  // Stop if already connected.
  if (mqtt.connected()) {
    return;
  }

  Serial.print("Connecting to MQTT... ");

  while ((ret = mqtt.connect()) != 0) { // connect will return 0 for connected
    Serial.println(mqtt.connectErrorString(ret));
    Serial.println("Retrying MQTT connection in 5 seconds...");
    mqtt.disconnect();
    delay(5000);  // wait 5 seconds
  }
  Serial.println("MQTT Connected!");
}

void datos()
{
            err = my_sds.read(&p25, &p10);
  if (!err) {
    Serial.println("P2.5: " + String(p25));
    Serial.println("P10:  " + String(p10));
  }

if (p25<=25 )
{
   digitalWrite(25,LOW);
   digitalWrite(26,HIGH);
   digitalWrite(27,HIGH);
}
else
if (p25 >25  &&  p25<=50 )
{
   digitalWrite(25,HIGH);
   digitalWrite(26,LOW);
   digitalWrite(27,HIGH);
}
else
if (p25 > 50)
{
   digitalWrite(25,HIGH);
   digitalWrite(26,HIGH);
   digitalWrite(27,LOW);
}
  Serial.println("--");
  Serial.println("  ");

  
  //
  lcd.clear();//Limpiamos la LCD
  lcd.print("P2.5: ");
  lcd.print(String(p25));//Escribimos en la primera linea
  lcd.print(" ug");
  lcd.setCursor(0, 1); //Saltamos a la segunda linea
  lcd.print("P10: ");
  lcd.print(String(p10));//Escribimos en la segunda linea
  lcd.print(" ug");
  lcd.setCursor(0, 2); //Saltamos a la tercera linea
  lcd.print("Latitud: ");
  lcd.print(latitud, 6);//Escribimos en la tercera linea
  lcd.setCursor(0, 3); //Saltamos a la cuarta linea
  lcd.print("Longitud: ");
  lcd.print(longitud, 6);//Escribimos en la cuarta linea


  delay(10000);


 //firebase

  if (variable1 == 1)
  {
    conexioninternet ();
    Serial.println("enviando por Firebase");
    slatitud = "{\"latitud" + String() + "\":" + String(latitud, 6) + ",";
    slongitud = "\"longitud" + String() + "\":" + String(longitud, 6) + ",";
    sP10 = "\"pm10" + String() + "\":" + String(p10) + ",";
    sP25 = "\"pm25" + String() + "\":" + String(p25) + ",";
    timestamp = "{\".sv\": \"timestamp\"}";
    stimestamp = "\"timestamp" + String() + "\":" + String(timestamp) + "}";

    //sbuffer = (slatitud + slongitud + srealPressure + sabsoluteAltitude + slecturaPorcentaje + slux + sh +  st + swindspeedmph + srainin + stimestamp);
    sbuffer = (slatitud + slongitud + sP10 + sP25 + stimestamp);
    Serial.println(sbuffer);
    Firebase.pushJSON(firebaseData, path, sbuffer);
    delay(5000);
  }

  //GPRS

  if (variable2 == 1)
  {
    Serial.println("envia datos");
    //MQTT_connect();
    Serial.print(F("\nEnviando Pm 10 "));
    Serial.print(p10);
    Serial.print("...");
    if (! xP10.publish(p10)) {
      Serial.println(F("Failed"));
    }
    else {
      Serial.println(F("OK!"));
    }
    delay(3000);
    Serial.print(F("\nEnviando Pm 25 "));
    Serial.print(p25);
    Serial.print("...");
    if (! xP25.publish(p25)) {
      Serial.println(F("Failed"));
    }
    else {
      Serial.println(F("OK!"));
    }    
    delay(3000);

    Serial.print(F("\nEnviando Latitud "));
    Serial.print(latitud);
    Serial.print("...");
    if (! xlatitud.publish(latitud)) {
      Serial.println(F("Failed"));
    }
    else {
      Serial.println(F("OK!"));
    }
    delay(3000);

    Serial.print(F("\nEnviando Longitud "));
    Serial.print(latitud);
    Serial.print("...");
    if (! xlongitud.publish(longitud)) {
      Serial.println(F("Failed"));
    }
    else {
      Serial.println(F("OK!"));
    }

  }

}
