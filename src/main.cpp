#include <ESP8266WiFi.h>
#include <Ticker.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <OneWire.h>
#include <DallasTemperature.h>

#include "main.h"

/* Put IP Address details */
IPAddress local_ip(192,168,1,1);
IPAddress gateway(192,168,1,1);
IPAddress subnet(255,255,255,0);

AsyncWebServer server(80);

Ticker timerISR;

OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);


void setup() {

  SousVideStatus = false;
  SousVideScheduler = false;
  SousVide_State = SOUSVIDE_STATE_INIT;
  SousVide_GlobalTimer = 0;
  SousVide_CookingTimer = 0;
  SousVide_TempTimer = 0;
  SousVide_Temperature = 0;
  SousVide_Relay_State = 0;
  SousVide_Pump_State = 0;

  Target_temp = 60;

  pinMode(SOUSVIDE_RELAY_VCC, OUTPUT);
  pinMode(SOUSVIDE_RELAY_PIN, OUTPUT);
  pinMode(SOUSVIDE_WATER_PUMP_PIN, OUTPUT);

  digitalWrite(SOUSVIDE_RELAY_VCC, LOW);
  digitalWrite(SOUSVIDE_RELAY_PIN, LOW);
  digitalWrite(SOUSVIDE_WATER_PUMP_PIN, LOW);

  /* begin serial connection */
  Serial.begin(SERIAL_BAUDRATE);

  /* start one wire */
  sensors.begin();

  WiFi.softAP(ssid, password);
  WiFi.softAPConfig(local_ip, gateway, subnet);

  timerISR.attach(1,HandleTimerInterrupt);
  delay(100);
  
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/html", index_html, processor);
  });
  server.on("/tartemp", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/plain", String(Target_temp).c_str());
  });
  server.on("/curtemp", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/plain", String(SousVide_Temperature_string).c_str());
  });
  server.on("/startstop", HTTP_GET, [](AsyncWebServerRequest *request){
    HandleStartStop();
    request->send_P(200, "text/html", index_html,  processor);
  });
  server.on("/up", HTTP_GET, [](AsyncWebServerRequest *request){
    HandleUP();
    request->send_P(200, "text/html", index_html,  processor);
  });
  server.on("/down", HTTP_GET, [](AsyncWebServerRequest *request){
    HandleDOWN();
    request->send_P(200, "text/html", index_html,  processor);
  });

  // Start server
  server.begin();
}
void loop() {
  if (false != SousVideStatus)
  {
    if (false != SousVideScheduler)
    {
      SousVide_Main();
      //sousvide_ReadTemperature();
      SousVideScheduler = false;
    }
    else
    {
      /* wait */
    }
  }
  else
  {
    SousVide_State = SOUSVIDE_STATE_INIT;
  }
}

String processor(const String& var)
{
  //Serial.println("------");
  //Serial.println(var);
 //Serial.println("------");
  String retval = "";
  if(var == "STATE"){
    if(false != SousVideStatus)
    {
      retval = "Stop";
    }
    else
    {
      retval = "Start";
    }
  }
  else if (var == "TARTEMP"){
    retval = String(Target_temp).c_str();
  }
  else if (var == "CURTEMP"){
    retval = String(SousVide_Temperature_string).c_str();
  }
  else
  {
    /* code */
  }
  //Serial.println(retval);
  return retval;
}

void HandleStartStop()
{
  if (false != SousVideStatus)
  {
    SousVideStatus = false;
  }
  else
  {
    SousVideStatus = true;
  }
}

void HandleUP() 
{
  Target_temp++;
}
void HandleDOWN() 
{
  Target_temp--;
}

void HandleTimerInterrupt()
{
  SousVideScheduler = true;
}

/************************************************************************/
/***************************** Sous Vide *******************************/
/************************************************************************/
void SousVide_Main (void)
{
    if (SOUSVIDE_MAIN_TIMER_THRESHOLD > SousVide_GlobalTimer)
    {
        SousVide_GlobalTimer++;
    }
    else
    {
        SousVide_GlobalTimer = 0;
    }
    sousvide_Circulator();
    sousvide_ReadTemperature();
    switch (SousVide_State)
    {
        case SOUSVIDE_STATE_INIT:
        {
            //Serial.println("State: Init");
            sousvide_HandleInitState();
            break;
        }
        case SOUSVIDE_STATE_PREHEAT:
        {
            //Serial.println("State: Pre-heat");
            sousvide_HandlePreheatState();
            break;
        }
        case SOUSVIDE_STATE_COOKING:
        {
            //Serial.println("State: Cooking");
            sousvide_HandleCookingtState();
            break;
        }    
        default:
        /* do nothing */
            break;
    }
}
void sousvide_HandleInitState()
{
    Serial.println("Initialization DONE!");
    digitalWrite(SOUSVIDE_RELAY_PIN, HIGH);
    digitalWrite(SOUSVIDE_RELAY_VCC, HIGH);
    digitalWrite(SOUSVIDE_WATER_PUMP_PIN, HIGH);
    SousVide_State = SOUSVIDE_STATE_PREHEAT;
}
void sousvide_HandlePreheatState()
{
    //sousvide_SendTempValues(Target_temp, SousVide_Temperature);

    if((Target_temp*10) <= SousVide_Temperature)
    {
        SousVide_State = SOUSVIDE_STATE_COOKING;
        sousvide_SetRelayStatus(SOUSVIDE_RELAY_STATUS_OFF);
    }
    else
    {
        /* keep preheat state */
        if (PREHEAT_TIMER_THRESHOLD >= SousVide_GlobalTimer)
        {
            /* Keep heating */
            sousvide_SetRelayStatus(SOUSVIDE_RELAY_STATUS_ON);
        }
        else
        {
            /* let heater to cool down */
            sousvide_SetRelayStatus(SOUSVIDE_RELAY_STATUS_OFF);
        }
    } 
}
void sousvide_HandleCookingtState()
{
    //sousvide_SendTempValues(Target_temp, SousVide_Temperature);

    if(((Target_temp*10) - SOUSVIDE_TEMPERATURE_HYSTERESIS) >= SousVide_Temperature)
    {
        if (COOKING_TIMER_THRESHOLD > SousVide_CookingTimer)
        {
            SousVide_CookingTimer++;
        }
        else
        {
            SousVide_CookingTimer = COOKING_TIMER_THRESHOLD;
        }

        if (COOKING_TIMER_THRESHOLD/2 > SousVide_CookingTimer)
        {
            /* Start heating */
            sousvide_SetRelayStatus(SOUSVIDE_RELAY_STATUS_ON);
        }
        else
        {
            /* let heater to cool down */
            sousvide_SetRelayStatus(SOUSVIDE_RELAY_STATUS_OFF);
            if (COOKING_TIMER_THRESHOLD == SousVide_CookingTimer)
            {
                SousVide_CookingTimer = 0;
            }
            else
            {
               /* do nothing */
            }
        }
    }
    else if(((Target_temp*10) + SOUSVIDE_TEMPERATURE_HYSTERESIS) <= SousVide_Temperature)
    {
        /* let heater to cool down */
        sousvide_SetRelayStatus(SOUSVIDE_RELAY_STATUS_OFF);
        /* Reset counter */
        SousVide_CookingTimer = 0;
    }
    else
    {
        if (COOKING_TIMER_THRESHOLD > SousVide_CookingTimer)
        {
            SousVide_CookingTimer++;
        }
        else
        {
            SousVide_CookingTimer = COOKING_TIMER_THRESHOLD;
        }

        if (COOKING_TIMER_THRESHOLD/4 > SousVide_CookingTimer)
        {
            /* Start heating */
            sousvide_SetRelayStatus(SOUSVIDE_RELAY_STATUS_ON);
        }
        else
        {
            /* let heater to cool down */
            sousvide_SetRelayStatus(SOUSVIDE_RELAY_STATUS_OFF);
            if (COOKING_TIMER_THRESHOLD == SousVide_CookingTimer)
            {
                SousVide_CookingTimer = 0;
            }
            else
            {
               /* do nothing */
            }
        }

    }    
}
void sousvide_SendTempValues (int TempSeted,int TempReaded)
{
    String temp_print = "";
    switch (SousVide_State)
    {
        case SOUSVIDE_STATE_INIT:
        {
            temp_print +="State: Init";
            break;
        }
        case SOUSVIDE_STATE_PREHEAT:
        {
            temp_print +="State: Pre-heat";
            break;
        }
        case SOUSVIDE_STATE_COOKING:
        {
            temp_print +="State: Cooking";
            break;
        }    
        default:
        /* do nothing */
            break;
    }
    temp_print +="\n";
    if(false != SousVide_Relay_State)
    {
      temp_print +="Relay ON";
    }
    else
    {
      temp_print +="Relay OFF";
    }
    temp_print +="      ";
    if(false != SousVide_Pump_State)
    {
      temp_print +="Pump ON";
    }
    else
    {
      temp_print +="Pump OFF";
    }
    
    temp_print +="\n";
    temp_print += "Seted Temperature: ";
    temp_print += String(TempSeted);
    temp_print +="      Actual Temperature: ";
    temp_print += String(TempReaded/10);
    temp_print +=".";
    temp_print += String(TempReaded%10);
    Serial.println(temp_print);
}
void sousvide_SetRelayStatus (uint8_t status)
{
    if (false != status)
    {
        /* set relay on */
        digitalWrite(SOUSVIDE_RELAY_PIN, LOW);
        SousVide_Relay_State = 1;
        //Serial.println("Relay ON");
    }
    else
    {
        /* set relay off */
        digitalWrite(SOUSVIDE_RELAY_PIN, HIGH);
        SousVide_Relay_State = 0;
        //Serial.println("Relay OFF");
    } 
}
void sousvide_Circulator (void)
{
    if ((WATER_PUMP_LOW_THRESHOLD <= SousVide_GlobalTimer) && (WATER_PUMP_HIGH_THRESHOLD > SousVide_GlobalTimer))
    {
        /* turn pump on */
        digitalWrite(SOUSVIDE_WATER_PUMP_PIN, LOW);
        SousVide_Pump_State = 1;
    }
    else if ((WATER_PUMP_LOW_THRESHOLD + 10 <= SousVide_GlobalTimer) && (WATER_PUMP_HIGH_THRESHOLD + 10 > SousVide_GlobalTimer))
    {
        /* turn pump on */
        digitalWrite(SOUSVIDE_WATER_PUMP_PIN, LOW);
        SousVide_Pump_State = 1;
    }
    else
    {
        /* turn pump off */
        digitalWrite(SOUSVIDE_WATER_PUMP_PIN, HIGH);
        SousVide_Pump_State = 0;

    }
}

void sousvide_ReadTemperature(void)
{
  unsigned int temp = 0;
  if (TEMP_READ_THRESHOLD <= SousVide_TempTimer)
  {
    sensors.requestTemperatures();  
    temp = sensors.getTempCByIndex(0) *10;
    if(temp < 1100)
    {
      SousVide_Temperature = temp;
    }
    else
    {
      /* do nothing error readed */
    }
    
    SousVide_Temperature_string = "";
    SousVide_Temperature_string = String(SousVide_Temperature/10);
    SousVide_Temperature_string += ".";
    SousVide_Temperature_string += String(SousVide_Temperature%10);

    SousVide_TempTimer = 0;
    sousvide_SendTempValues(Target_temp, SousVide_Temperature);    
  }
  else
  {
    SousVide_TempTimer++;
  }
}
