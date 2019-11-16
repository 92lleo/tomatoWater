// Do not remove the include below
#include "tomatowater.h"
#include <ESP8266WiFi.h>
#include <ArduinoJson.h>
#include <ESP8266httpUpdate.h>
#include <ESP8266WebServer.h>
#include <ESP8266HTTPUpdateServer.h>

const char* ssid = "your-ssid";
const char* password = "your-password";

const int sensorSwitch = D3;

Plant tomato { D6, D5, "Tomaten", 20, 80, 30, 60, 5,  // min
		90, // max
		0, 0, 0,
		LOW };

Plant strawberry { D4, 111, "Erbeeren", 20, 80, 20, 60, 7, // min
		96, // max
		0, 0, 0,
		HIGH };

Plant plants[] = { tomato, strawberry };

ESP8266WebServer webServer(80);

ESP8266HTTPUpdateServer httpUpdater;
WiFiClient client;
Environment res;

//The setup function is called once at startup of the sketch
void setup() {

	for (Plant plant : plants) {
		if (plant.pump_pin < 100) {
			digitalWrite(plant.pump_pin, LOW);
			pinMode(plant.pump_pin, OUTPUT);
		}


		digitalWrite(plant.moisture_pin, LOW);
		pinMode(plant.moisture_pin, OUTPUT);
	}

	Serial.begin(9600);


	for (int i = 0; i < 20; i++) {
		Serial.print(".");
		delay(500);
	}
	Serial.println(".");

	Serial.println("*****************************************************");

	for (Plant plant : plants) {
		digitalWrite(plant.moisture_pin, HIGH);
	}

	pinMode(LED_BUILTIN, OUTPUT);
	digitalWrite(sensorSwitch, LOW);
	pinMode(sensorSwitch, OUTPUT);

	WiFi.mode(WIFI_STA);
	Serial.println();
	Serial.print("Connecting to ");
	Serial.println(ssid);

	WiFi.begin(ssid, password);

	while (WiFi.status() != WL_CONNECTED) {
		delay(500);
		Serial.print(".");
	}
	Serial.println("");
	Serial.println("WiFi connected");

	// Start the server
	webServer.on("/", handleRoot);
//	webServer.on("/env", handleEnv);
//	webServer.on("/env/", handleEnv);
	webServer.on("/water", handlePump);
	webServer.on("?water", handlePump);
	webServer.on("/water/", handlePump);
	webServer.onNotFound(handleRoot);
	webServer.on("/inline", []() {
		webServer.send(200, "text/plain", "this works as well");
	});

	httpUpdater.setup(&webServer);

	webServer.begin();
//	server.begin();


	Serial.println("Server started");

	// Print the IP address
	Serial.println(WiFi.localIP());
}
// The loop function is called in an endless loop
void loop() {

	if (millis() % 10000 == 0) {

		for (Plant &plant : plants) {
			Serial.println("checking plant: " + plant.name);
			Serial.print("MOISTURE LEVEL : ");
			plant.moisture_level = getMoisturePercentage(plant);
			Serial.println(plant.moisture_level);

			if (plant.moisture_level < plant.min_moisture_percent
					&& (plant.moisture_pin < 100)
					&& (millis() - plant.lastWater > plant.wait_time_seconds*1000)) {
				Serial.print("Below minimum (");
				Serial.print(plant.min_moisture_percent);
				Serial.println("), watering...");
				digitalWrite(LED_BUILTIN, LOW);
				waterPlant(plant);
				digitalWrite(LED_BUILTIN, HIGH);
			} else {
				Serial.println("No need to water.");
			}

			plant.lastCheck = millis();
		}
	}

	webServer.handleClient();
}

void waterPlant(Plant plant){
	waterPlant(plant.pump_pin, plant.pump_time_seconds);
	plant.lastWater = millis();
}

void waterPlant(int pump_pin, int waterTimeSeconds) {
	digitalWrite(pump_pin, HIGH);
	delay(waterTimeSeconds * 1000);
	digitalWrite(pump_pin, LOW);
	delay(1000);
}

void waitForWater(int seconds) {
	delay(seconds * 1000);
}

int getMoistureLevel(Plant &plant) {
	int moisture_value = -1;
	//digitalWrite(plant.moisture_pin, HIGH);
	digitalWrite(sensorSwitch, plant.sensorSwitchVal);
	delay(750);
	moisture_value = analogRead((unsigned char) A0);
	delay(150);
	//digitalWrite(plant.moisture_pin, LOW);
	Serial.print("\n\n"+plant.name+":");
	Serial.println(moisture_value);
	Serial.println();
	return moisture_value;
}

int getMoisturePercentage(Plant &plant) {
	int moisture_value = -1;
	const int moisture_max = 600;
	moisture_value = getMoistureLevel(plant);
	if (moisture_value > moisture_max) {
		moisture_value = moisture_max;
	}
	moisture_value = moisture_max - moisture_value;
	moisture_value = moisture_value * 100;
	moisture_value = moisture_value / moisture_max;

	// normalize
	moisture_value = (moisture_value - plant.moisture_low) * (100)/(plant.moisture_high-plant.moisture_low);

	moisture_value = min(moisture_value, 100);
	moisture_value = max(moisture_value, 0);

	return moisture_value;
}

String getColorFromMoisture(const int moisturePercentage) {
	if(moisturePercentage < 20){
		return "bg-danger";
	} else if(moisturePercentage < 60){
		return "bg-warning";
	} else if(moisturePercentage < 99){
		return "bg-success";
	} else {
		return "bg-warning";
	}
}

String getColorFromTemperature(const int temperatureCelsius) {
	if(temperatureCelsius < 10){
		return "bg-danger";
	} else if(temperatureCelsius < 17){
		return "bg-warning";
	} else if(temperatureCelsius < 22){
		return "bg-success";
	} else if(temperatureCelsius < 30){
		return "bg-warning";
	} else {
		return "bg-danger";
	}
}

void handleRoot() {
		String response;
		response += getPageHeader();

		for (Plant &plant : plants) {
			response += "<div class=\"card shadow mb-4\">\n<div class=\"card-header py-3\">\n    <h6 class=\"m-0 font-weight-bold text-primary\">";
			response += plant.name;
			response += "</h6>\n</div>\n<div class=\"card-body\">\n    <h4 class=\"small font-weight-bold\">Erdfeuchtigkeit<span class=\"float-right\">";
			response += plant.moisture_level;
			response += "%</span></h4>\n    <div class=\"progress mb-4\">\n        <div class=\"progress-bar ";
			response += getColorFromMoisture(plant.moisture_level);
			response += "\" role=\"progressbar\" style=\"width: ";
			response += plant.moisture_level;
			response += "%\" aria-valuenow=\"";
			response += plant.moisture_level;
			response += "\" aria-valuemin=\"0\" aria-valuemax=\"100\"></div>\n    </div>";
			if (plant.pump_pin <= 100) {
				int distance = millis() - plant.lastWater;
				response += "<p>Zuletzt gegossen: vor ";
				response += distance / 1000 / 60;
				response += " Minuten</p>\n    <a href=\"water\" class=\"btn btn-outline-primary btn-icon-split\">\n        <span class=\"icon text-primary-50\">									        <i class=\"fas fa-arrow-right\"></i>									    </span>\n        <span class=\"text\">Jetzt gie&szlig;en</span>\n    </a>\n</div>                            </div>";
			} else {
				response += "<p>(Keine Pumpe angeschlossen)</p>\n</div>                            </div>";
			}
		}

		response += getPageFooter();
		webServer.send(200, "text/html", response);

}

void handlePump() {
	if (!webServer.authenticate("leo", "tomato")) {
		return webServer.requestAuthentication();
	}

	webServer.sendHeader("Location", "/", true);
	webServer.send(302, "text/plane", "");

	waterPlant(plants[0]);
}


String getPageHeader() {
	return "<!DOCTYPE html><html lang=\"de\"><head>    <meta charset=\"utf-8\">    <meta http-equiv=\"X-UA-Compatible\" content=\"IE=edge\">    <meta name=\"viewport\" content=\"width=device-width, initial-scale=1, shrink-to-fit=no\">    <meta name=\"description\" content=\"\">    <meta name=\"author\" content=\"\">    <title>Leos Tomaten&uuml;berwachung</title>    <!-- Custom fonts for this template-->    <link href=\"https://kuenzler.io/tomatoes/vendor/fontawesome-free/css/all.min.css\" rel=\"stylesheet\" type=\"text/css\">    <link href=\"https://fonts.googleapis.com/css?family=Nunito:200,200i,300,300i,400,400i,600,600i,700,700i,800,800i,900,900i\" rel=\"stylesheet\">    <!-- Custom styles for this template-->    <link href=\"https://kuenzler.io/tomatoes/css/sb-admin-2.min.css\" rel=\"stylesheet\"></head><body id=\"page-top\">    <!-- Page Wrapper -->    <div id=\"wrapper\">        <!-- Content Wrapper -->        <div id=\"content-wrapper\" class=\"d-flex flex-column\">            <!-- Main Content -->            <div id=\"content\">                <!-- Begin Page Content -->                <div class=\"container-fluid\">                    <!-- Page Heading -->                    <div class=\"d-sm-flex align-items-center justify-content-between mb-4\">                        <br>                        <br>                        <h1 class=\"h3 mb-0 text-gray-800\">Leos Tomaten&uuml;berwachung</h1>                        <a href=\"https://kuenzler.io/\" class=\"d-none d-sm-inline-block btn btn-sm btn-primary shadow-sm\"><i class=\"fas fa-arrow-right fa-sm text-white-50\"></i>Zur&uuml;ck zur Hauptseite</a>                    </div>                    <!-- Content Row -->                    <div class=\"row\">                        <!-- Content Column -->                        <div class=\"col-lg-7 mb-4\">";
}

String getPageFooter() {
	return "<div class=\"card shadow mb-4\">                                <div class=\"card-header py-3\">                                    <h6 class=\"m-0 font-weight-bold text-primary\">Umgebung</h6>                                </div>                                <div class=\"card-body\">                                    <h4 class=\"small font-weight-bold\">Temperatur<span class=\"float-right\">0&#176;C</span></h4>                                    <div class=\"progress mb-4\">                                        <div class=\"progress-bar bg-danger\" role=\"progressbar\" style=\"width: 40%\" aria-valuenow=\"0\" aria-valuemin=\"-20\" aria-valuemax=\"40\"></div>                                    </div>                                    <p>no data</p>                                </div>                                <div class=\"card-body\">                                    <h4 class=\"small font-weight-bold\">Luftfeuchtigkeit<span class=\"float-right\">0%</span></h4>                                    <div class=\"progress mb-4\">                                        <div class=\"progress-bar bg-info\" role=\"progressbar\" style=\"width: 0%\" aria-valuenow=\"17\" aria-valuemin=\"0\" aria-valuemax=\"100\"></div>                                    </div>                                    <p>no data</p>                                </div>                            </div>                            </div>                            <!-- /.container-fluid -->                        </div>                        <!-- End of Main Content -->                        <!-- Footer -->                        <footer class=\"sticky-footer bg-white\">                            <div class=\"container my-auto\">                                <div class=\"copyright text-center my-auto\">                                    <span>Copyright &copy; kuenzler.io 2019</span>                                </div>                            </div>                        </footer>                        <!-- End of Footer -->                    </div>                    <!-- End of Content Wrapper -->                </div>                <!-- End of Page Wrapper -->                <!-- Scroll to Top Button-->                <a class=\"scroll-to-top rounded\" href=\"#page-top\">                    <i class=\"fas fa-angle-up\"></i>                </a>                <!-- Bootstrap core JavaScript-->                <script src=\"https://kuenzler.io/tomatoes/vendor/jquery/jquery.min.js\"></script>                <script src=\"https://kuenzler.io/tomatoes/vendor/bootstrap/js/bootstrap.bundle.min.js\"></script>                <!-- Core plugin JavaScript-->                <script src=\"https://kuenzler.io/tomatoes/vendor/jquery-easing/jquery.easing.min.js\"></script>                <!-- Custom scripts for all pages-->                <script src=\"https://kuenzler.io/tomatoes/js/sb-admin-2.min.js\"></script>                <!-- Page level plugins -->                <script src=\"https://kuenzler.io/tomatoes/vendor/chart.js/Chart.min.js\"></script>                <!-- Page level custom scripts -->                <script src=\"https://kuenzler.io/tomatoes/js/demo/chart-area-demo.js\"></script>                <script src=\"https://kuenzler.io/tomatoes/js/demo/chart-pie-demo.js\"></script></body></html>";
}
