#include "web_manager.h"
#include "config.h"

#if WEB_SERVER_ENABLED

#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>

ESP8266WebServer server(WEB_SERVER_PORT);

static void handleRoot()
{
    String html;

    html += "<!DOCTYPE html>";
    html += "<html>";
    html += "<head>";
    html += "<title>TerraLink Rescue Command Center</title>";
    html += "</head>";

    html += "<body>";

    html += "<h1>TerraLink Rescue Command Center</h1>";

    html += "<p>System: ONLINE</p>";

    html += "<hr>";

    html += "<h2>Emergency Console</h2>";

    html += "<button>Latest SOS</button>";
    html += "<button>SOS Events</button>";
    html += "<button>Node Registry</button>";
    html += "<button>Network</button>";

    html += "</body>";
    html += "</html>";

    server.send(
        200,
        "text/html",
        html
    );
}

void webManagerBegin()
{
    WiFi.mode(WIFI_AP);

    WiFi.softAP(
        WIFI_AP_NAME,
        WIFI_AP_PASSWORD
    );

    Serial.println();
    Serial.println("[WEB] Rescue Command Center");

    Serial.print("[WEB] AP: ");
    Serial.println(WIFI_AP_NAME);

    Serial.print("[WEB] IP: ");
    Serial.println(WiFi.softAPIP());

    server.on(
        "/",
        handleRoot
    );

    server.begin();

    Serial.println("[WEB] SERVER READY");
}

void webManagerLoop()
{
    server.handleClient();
}

#else

void webManagerBegin()
{
}

void webManagerLoop()
{
}

#endif