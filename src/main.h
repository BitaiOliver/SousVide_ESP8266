#ifndef MAIN_HEADER

#define MAIN_HEADER 1

#define SERIAL_BAUDRATE                 9600U

#define SOUSVIDE_RELAY_VCC              14U /* D05 */
#define SOUSVIDE_RELAY_PIN              12U /* D06 */
#define SOUSVIDE_WATER_PUMP_PIN         13U  /* D07 */

#define ONE_WIRE_BUS                    4U  /*D02*/


#define PREHEAT_TIMER_THRESHOLD         ((uint8_t) 26U) /* ex: Set 20 for 20 seconds */
#define COOKING_TIMER_THRESHOLD         ((uint8_t) 24U) /* ex: Set 10 for 10 seconds */
#define WATER_PUMP_LOW_THRESHOLD        ((uint8_t) 10U) 
#define WATER_PUMP_HIGH_THRESHOLD       ((uint8_t) 12U) 
#define TEMP_READ_THRESHOLD             ((uint8_t) 3U) 

#define SOUSVIDE_STATE_INIT             0U
#define SOUSVIDE_STATE_PREHEAT          1U
#define SOUSVIDE_STATE_COOKING          2U

#define SOUSVIDE_RELAY_STATUS_OFF       0U
#define SOUSVIDE_RELAY_STATUS_ON        1U

#define SOUSVIDE_MAIN_TIMER_THRESHOLD   30U
#define SOUSVIDE_TEMPERATURE_HYSTERESIS 2U /* 0.4 degree */

/* Put your SSID & Password */
const char* ssid = "SousVide";  // Enter SSID here
const char* password = "12345678";  //Enter Password here

bool SousVideStatus;
bool SousVideScheduler;

uint8_t Target_temp;
uint8_t SousVide_State;
uint8_t SousVide_Relay_State;
uint8_t SousVide_Pump_State;
uint8_t SousVide_GlobalTimer;
uint8_t SousVide_CookingTimer;
uint8_t SousVide_TempTimer;
unsigned int SousVide_Temperature;
String SousVide_Temperature_string;

String processor(const String& var);

void HandleStartStop();
void HandleUP();
void HandleDOWN();
void HandleTimerInterrupt();

void SousVide_Main(void);
void sousvide_ReadTemperature (void);
void sousvide_HandleInitState (void);
void sousvide_HandlePreheatState (void);
void sousvide_HandleCookingtState (void);
void sousvide_SendTempValues (int TempSeted,int TempReaded);
void sousvide_SetRelayStatus (uint8_t status);
void sousvide_Circulator (void);


const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html>
<head>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <title>EuroSteaks</title>
	<style>
	html { font-family: Helvetica; display: inline-block; margin: 0px auto; text-align: center;}
	body{margin-top: 50px;} h1 {color: #444444;margin: 50px auto 30px;} h3 {color: #444444;margin-bottom: 20px;}
	</style>
</head>
<body>
  <h2>Oliver's SteakHouse</h2>
  <p>
    <a href="/startstop"><button class="button">%STATE%</button></a>
  </p>
  <p>
    <a href="/up"><button class="button button2">Increase</button></a>
  </p>
  <p>
    <a href="/down"><button class="button button3">Decrease</button></a>
  </p>

  <p>
    <span class="dht-labels">Target Temperature:</span> 
    <span id="temperature"><font color=green size="6">%TARTEMP%</font></span>
    <sup class="units">&deg;C</sup>
  </p>
  <p>
    <span class="dht-labels">Current Temperature:</span> 
    <font color=red size="6">
      <span id="curtemp">%CURTEMP%</span>
    </font>
    <sup class="units">&deg;C</sup>
  </p>
</body>
<script>
setInterval(function ( ) {
  var xhttp = new XMLHttpRequest();
  xhttp.onreadystatechange = function() {
    if (this.readyState == 4 && this.status == 200) {
      document.getElementById("curtemp").innerHTML = this.responseText;
    }
  };
  xhttp.open("GET", "/curtemp", true);
  xhttp.send();
}, 3000);

</script>
</html>)rawliteral";

#endif
