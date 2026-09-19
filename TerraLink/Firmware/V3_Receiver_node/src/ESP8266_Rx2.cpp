#include <Arduino.h>
#include <SoftwareSerial.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <LittleFS.h>

/*
============================================================
                TERRALINK RECEIVER #2
             RESCUE COMMAND CENTER
============================================================

ESP8266 NodeMCU

UART:
RX = D8
TX = D0

KEYPAD:

COL1 = D1
COL2 = D2
COL3 = D5
COL4 = D6

ROW1 = D7
ROW2 = GPIO3 / RX
ROW3 = D4
ROW4 = D3

WiFi AP:
SSID     = TerraLink-RX2
PASSWORD = terralink123

============================================================
*/

/* =========================================================
                         WIFI
========================================================= */

const char* AP_SSID =
  "TerraLink-RX2";

const char* AP_PASSWORD =
  "terralink123";

ESP8266WebServer server(80);

/* =========================================================
                         UART
========================================================= */

#define UART_RX_PIN D8
#define UART_TX_PIN D0

SoftwareSerial rescueSerial(
  UART_RX_PIN,
  UART_TX_PIN
);

#define UART_BAUD 9600

String uartBuffer = "";

/* =========================================================
                         KEYPAD
========================================================= */

#define COL1 D1
#define COL2 D2
#define COL3 D5
#define COL4 D6

#define ROW1 D7
#define ROW2 3
#define ROW3 D4
#define ROW4 D3

uint8_t rowPins[4] =
{
  ROW1,
  ROW2,
  ROW3,
  ROW4
};

uint8_t colPins[4] =
{
  COL1,
  COL2,
  COL3,
  COL4
};

char keyMap[4][4] =
{
  {'1','2','3','A'},
  {'4','5','6','B'},
  {'7','8','9','C'},
  {'*','0','#','D'}
};

/* =========================================================
                     DATABASE
========================================================= */

#define MAX_NODES 15

struct NodeRecord
{
  bool valid;

  uint16_t nodeId;

  uint32_t sequence;

  String latitude;
  String longitude;

  int hop;
  int rssi;

  bool sosActive;
  bool resolved;

  unsigned long firstSeen;
  unsigned long lastSeen;
};

NodeRecord nodes[MAX_NODES];

int selectedNodeIndex = -1;

/* =========================================================
                     INCIDENT
========================================================= */

struct IncidentRecord
{
  bool valid;

  String incidentId;

  uint16_t nodeId;

  uint32_t sequence;

  String latitude;
  String longitude;

  int hop;
  int rssi;

  String status;

  unsigned long createdAt;
  unsigned long updatedAt;
};

#define MAX_INCIDENTS 20

IncidentRecord incidents[MAX_INCIDENTS];

/* =========================================================
                    MESSAGE RECORD
========================================================= */

struct MessageRecord
{
  bool valid;

  uint16_t nodeId;

  uint32_t sequence;

  String message;

  String status;

  unsigned long timestamp;
};

#define MAX_MESSAGES 25

MessageRecord messages[MAX_MESSAGES];

/* =========================================================
                    RX1 STATUS
========================================================= */

bool rx1Online = false;

unsigned long rx1LastSeen = 0;

unsigned long rx1LoRaRx = 0;
unsigned long rx1LoRaTx = 0;

unsigned long rx1SOS = 0;
unsigned long rx1ACK = 0;
unsigned long rx1LOC = 0;
unsigned long rx1MSG = 0;

/* =========================================================
                     UI STATE
========================================================= */

enum UIState
{
  UI_HOME,
  UI_NODE_SELECT,
  UI_NODE_CONTROL,
  UI_ACTION_CONFIRM,
  UI_NODE_INFO,
  UI_RESOLVE_CONFIRM,
  UI_SENT_STATUS
};

UIState uiState = UI_HOME;

/* =========================================================
                     ACTION TYPE
========================================================= */

enum ActionType
{
  ACT_NONE,

  ACT_STAY_CALM,
  ACT_RESCUE,
  ACT_DO_NOT_MOVE,
  ACT_LOCATION_AGAIN,
  ACT_HELP_APPROACHING,
  ACT_GPS_REQUEST,
  ACT_EMERGENCY,
  ACT_RESOLVE
};

ActionType pendingAction = ACT_NONE;

/* =========================================================
                       WEB STATE
========================================================= */

String webMessage =
  "System Ready";

unsigned long webMessageTime = 0;

/* =========================================================
                      PROTOTYPES
========================================================= */

char scanKeypadRaw();
char getKeypadKey();
char translateKey(char key);
void handleKey(char key);

void showHomeScreen();
void showNodeSelectionScreen();
void showNodeControlScreen();
void showActionConfirmScreen();
void showNodeInfoScreen();
void showResolveConfirmScreen();
void showSentScreen();

String actionName(ActionType action);
String getUIStateName();

int findNode(uint16_t nodeId);
int allocateNodeSlot();

int getNodeCount();
int getActiveSOSCount();

bool isNodeValid(int index);

void updateNodeFromSOS(
  uint16_t nodeId,
  uint32_t sequence,
  String latitude,
  String longitude,
  int hop,
  int rssi
);

void processUART();
void processUARTLine(String line);

void processSOS(String line);
void processLOCATION(String line);
void processACK(String line);
void processMSGACK(String line);
void processRESOLVEACK(String line);
void processRX1Status(String line);
void processTXStatus(String line);

void sendCommandToRX1(
  String type,
  uint16_t nodeId,
  String payload
);

void executePendingAction();

void resolveSelectedNode();

void createIncident(
  uint16_t nodeId,
  uint32_t sequence,
  String lat,
  String lon,
  int hop,
  int rssi
);

void updateIncidentStatus(
  uint16_t nodeId,
  String status
);

void addMessageLog(
  uint16_t nodeId,
  uint32_t sequence,
  String message,
  String status
);

void updateMessageStatus(
  uint16_t nodeId,
  uint32_t sequence,
  String status
);

String createIncidentId(
  uint16_t nodeId,
  uint32_t seq
);

void initDatabase();

void saveNodes();
void saveIncidents();
void saveMessages();

void loadNodes();
void loadIncidents();
void loadMessages();

void clearDatabase();

String htmlEscape(String value);

void setupWebServer();

void handleRoot();
void handleAPIStatus();
void handleAPINodes();
void handleAPIIncidents();
void handleAPIMessages();

void handleWebCommand();
void handleWebResolve();
void handleWebClear();

String makeDashboardHTML();

/* =========================================================
                    DATABASE INIT
========================================================= */

void initDatabase()
{
  for (int i = 0; i < MAX_NODES; i++)
  {
    nodes[i].valid = false;
    nodes[i].nodeId = 0;
    nodes[i].sequence = 0;
    nodes[i].latitude = "";
    nodes[i].longitude = "";
    nodes[i].hop = 0;
    nodes[i].rssi = 0;
    nodes[i].sosActive = false;
    nodes[i].resolved = false;
    nodes[i].firstSeen = 0;
    nodes[i].lastSeen = 0;
  }

  for (int i = 0; i < MAX_INCIDENTS; i++)
  {
    incidents[i].valid = false;
  }

  for (int i = 0; i < MAX_MESSAGES; i++)
  {
    messages[i].valid = false;
  }
}

/* =========================================================
                    FIND NODE
========================================================= */

int findNode(
  uint16_t nodeId
)
{
  for (int i = 0; i < MAX_NODES; i++)
  {
    if (
      nodes[i].valid &&
      nodes[i].nodeId == nodeId
    )
    {
      return i;
    }
  }

  return -1;
}

/* =========================================================
                  ALLOCATE NODE
========================================================= */

int allocateNodeSlot()
{
  for (int i = 0; i < MAX_NODES; i++)
  {
    if (!nodes[i].valid)
      return i;
  }

  int oldest = 0;

  unsigned long oldestTime =
    nodes[0].lastSeen;

  for (int i = 1; i < MAX_NODES; i++)
  {
    if (
      nodes[i].lastSeen <
      oldestTime
    )
    {
      oldestTime =
        nodes[i].lastSeen;

      oldest = i;
    }
  }

  return oldest;
}

/* =========================================================
                    NODE COUNT
========================================================= */

int getNodeCount()
{
  int count = 0;

  for (int i = 0; i < MAX_NODES; i++)
  {
    if (nodes[i].valid)
      count++;
  }

  return count;
}

/* =========================================================
                  ACTIVE SOS COUNT
========================================================= */

int getActiveSOSCount()
{
  int count = 0;

  for (int i = 0; i < MAX_NODES; i++)
  {
    if (
      nodes[i].valid &&
      nodes[i].sosActive &&
      !nodes[i].resolved
    )
    {
      count++;
    }
  }

  return count;
}

/* =========================================================
                    NODE VALID
========================================================= */

bool isNodeValid(int index)
{
  if (
    index < 0 ||
    index >= MAX_NODES
  )
  {
    return false;
  }

  return nodes[index].valid;
}

/* =========================================================
                 CREATE INCIDENT ID
========================================================= */

String createIncidentId(
  uint16_t nodeId,
  uint32_t seq
)
{
  return
    "TL-" +
    String(nodeId) +
    "-" +
    String(seq);
}

/* =========================================================
                    UPDATE NODE
========================================================= */

void updateNodeFromSOS(
  uint16_t nodeId,
  uint32_t sequence,
  String latitude,
  String longitude,
  int hop,
  int rssi
)
{
  int index =
    findNode(nodeId);

  if (index < 0)
  {
    index =
      allocateNodeSlot();

    nodes[index].valid = true;

    nodes[index].firstSeen =
      millis();
  }

  nodes[index].nodeId =
    nodeId;

  nodes[index].sequence =
    sequence;

  nodes[index].latitude =
    latitude;

  nodes[index].longitude =
    longitude;

  nodes[index].hop =
    hop;

  nodes[index].rssi =
    rssi;

  nodes[index].sosActive =
    true;

  nodes[index].resolved =
    false;

  nodes[index].lastSeen =
    millis();

  selectedNodeIndex =
    index;

  createIncident(
    nodeId,
    sequence,
    latitude,
    longitude,
    hop,
    rssi
  );

  saveNodes();

  Serial.println();
  Serial.println("======================================");
  Serial.println("          NEW SOS INCIDENT");
  Serial.println("======================================");

  Serial.print("NODE: ");
  Serial.println(nodeId);

  Serial.print("SEQ: ");
  Serial.println(sequence);

  Serial.print("LAT: ");
  Serial.println(latitude);

  Serial.print("LON: ");
  Serial.println(longitude);

  Serial.print("HOP: ");
  Serial.println(hop);

  Serial.print("RSSI: ");
  Serial.println(rssi);

  Serial.println("======================================");

  uiState = UI_HOME;

  showHomeScreen();
}

/* =========================================================
                  CREATE INCIDENT
========================================================= */

void createIncident(
  uint16_t nodeId,
  uint32_t sequence,
  String lat,
  String lon,
  int hop,
  int rssi
)
{
  /*
     Check whether incident already exists.
  */

  for (int i = 0; i < MAX_INCIDENTS; i++)
  {
    if (
      incidents[i].valid &&
      incidents[i].nodeId == nodeId &&
      incidents[i].sequence == sequence
    )
    {
      incidents[i].latitude = lat;
      incidents[i].longitude = lon;
      incidents[i].hop = hop;
      incidents[i].rssi = rssi;
      incidents[i].updatedAt = millis();

      saveIncidents();

      return;
    }
  }

  int slot = -1;

  for (int i = 0; i < MAX_INCIDENTS; i++)
  {
    if (!incidents[i].valid)
    {
      slot = i;
      break;
    }
  }

  if (slot < 0)
  {
    slot = 0;

    for (int i = 1; i < MAX_INCIDENTS; i++)
    {
      if (
        incidents[i].createdAt <
        incidents[slot].createdAt
      )
      {
        slot = i;
      }
    }
  }

  incidents[slot].valid = true;

  incidents[slot].incidentId =
    createIncidentId(
      nodeId,
      sequence
    );

  incidents[slot].nodeId =
    nodeId;

  incidents[slot].sequence =
    sequence;

  incidents[slot].latitude =
    lat;

  incidents[slot].longitude =
    lon;

  incidents[slot].hop =
    hop;

  incidents[slot].rssi =
    rssi;

  incidents[slot].status =
    "ACTIVE";

  incidents[slot].createdAt =
    millis();

  incidents[slot].updatedAt =
    millis();

  saveIncidents();
}

/* =========================================================
                 UPDATE INCIDENT STATUS
========================================================= */

void updateIncidentStatus(
  uint16_t nodeId,
  String status
)
{
  for (int i = 0; i < MAX_INCIDENTS; i++)
  {
    if (
      incidents[i].valid &&
      incidents[i].nodeId == nodeId &&
      incidents[i].status != "RESOLVED"
    )
    {
      incidents[i].status =
        status;

      incidents[i].updatedAt =
        millis();
    }
  }

  saveIncidents();
}

/* =========================================================
                    MESSAGE LOG
========================================================= */

void addMessageLog(
  uint16_t nodeId,
  uint32_t sequence,
  String message,
  String status
)
{
  /*
     Shift oldest message when full.
  */

  int slot = -1;

  for (int i = 0; i < MAX_MESSAGES; i++)
  {
    if (!messages[i].valid)
    {
      slot = i;
      break;
    }
  }

  if (slot < 0)
  {
    for (int i = 1; i < MAX_MESSAGES; i++)
    {
      messages[i - 1] =
        messages[i];
    }

    slot =
      MAX_MESSAGES - 1;
  }

  messages[slot].valid = true;

  messages[slot].nodeId =
    nodeId;

  messages[slot].sequence =
    sequence;

  messages[slot].message =
    message;

  messages[slot].status =
    status;

  messages[slot].timestamp =
    millis();

  saveMessages();
}

/* =========================================================
                 UPDATE MESSAGE STATUS
========================================================= */

void updateMessageStatus(
  uint16_t nodeId,
  uint32_t sequence,
  String status
)
{
  for (int i = MAX_MESSAGES - 1; i >= 0; i--)
  {
    if (
      messages[i].valid &&
      messages[i].nodeId == nodeId &&
      messages[i].sequence == sequence
    )
    {
      messages[i].status =
        status;

      saveMessages();

      return;
    }
  }
}

/* =========================================================
                    PROCESS UART
========================================================= */

void processUART()
{
  while (rescueSerial.available())
  {
    char c =
      rescueSerial.read();

    if (c == '\r')
      continue;

    if (c == '\n')
    {
      uartBuffer.trim();

      if (uartBuffer.length() > 0)
      {
        processUARTLine(
          uartBuffer
        );
      }

      uartBuffer = "";

      continue;
    }

    uartBuffer += c;

    if (uartBuffer.length() > 250)
    {
      uartBuffer = "";

      Serial.println(
        "[ERROR] UART overflow"
      );
    }
  }
}

/* =========================================================
                 PROCESS UART LINE
========================================================= */

void processUARTLine(
  String line
)
{
  line.trim();

  Serial.print("[UART RX] ");
  Serial.println(line);

  if (line.startsWith("SOS,"))
  {
    processSOS(line);
    return;
  }

  if (line.startsWith("LOCATION,"))
  {
    processLOCATION(line);
    return;
  }

  if (line.startsWith("ACK,"))
  {
    processACK(line);
    return;
  }

  if (line.startsWith("MSG_ACK,"))
  {
    processMSGACK(line);
    return;
  }

  if (line.startsWith("RESOLVE_ACK,"))
  {
    processRESOLVEACK(line);
    return;
  }

  if (line.startsWith("RX1_STATUS,"))
  {
    processRX1Status(line);
    return;
  }

  if (line.startsWith("TX_STATUS,"))
  {
    processTXStatus(line);
    return;
  }

  if (line.startsWith("RX1,ONLINE"))
  {
    rx1Online = true;
    rx1LastSeen = millis();
    return;
  }

  Serial.println("[UART] Unknown packet");
}

/* =========================================================
                     PROCESS SOS
========================================================= */

void processSOS(
  String line
)
{
  /*
     Expected:

     SOS,node,seq,lat,lon,hop,rssi,ttl
  */

  int p1 = line.indexOf(',');
  int p2 = line.indexOf(',', p1 + 1);
  int p3 = line.indexOf(',', p2 + 1);
  int p4 = line.indexOf(',', p3 + 1);
  int p5 = line.indexOf(',', p4 + 1);
  int p6 = line.indexOf(',', p5 + 1);
  int p7 = line.indexOf(',', p6 + 1);

  if (
    p1 < 0 ||
    p2 < 0 ||
    p3 < 0 ||
    p4 < 0
  )
  {
    Serial.println("[ERROR] Invalid SOS");
    return;
  }

  String nodeString =
    line.substring(p1 + 1, p2);

  String seqString =
    line.substring(p2 + 1, p3);

  String lat =
    line.substring(p3 + 1, p4);

  String lon;

  int hop = 0;
  int rssi = 0;

  if (p5 >= 0)
  {
    lon =
      line.substring(p4 + 1, p5);
  }
  else
  {
    lon =
      line.substring(p4 + 1);
  }

  if (p6 >= 0)
  {
    hop =
      line.substring(p5 + 1, p6)
      .toInt();

    rssi =
      line.substring(p6 + 1,
                     p7 >= 0 ? p7 : line.length())
      .toInt();
  }

  uint16_t node =
    nodeString.toInt();

  uint32_t seq =
    strtoul(
      seqString.c_str(),
      NULL,
      10
    );

  updateNodeFromSOS(
    node,
    seq,
    lat,
    lon,
    hop,
    rssi
  );
}

/* =========================================================
                   PROCESS LOCATION
========================================================= */

void processLOCATION(
  String line
)
{
  /*
     LOCATION,node,seq,lat,lon,hop,rssi
  */

  int p1 = line.indexOf(',');
  int p2 = line.indexOf(',', p1 + 1);
  int p3 = line.indexOf(',', p2 + 1);
  int p4 = line.indexOf(',', p3 + 1);
  int p5 = line.indexOf(',', p4 + 1);
  int p6 = line.indexOf(',', p5 + 1);

  if (
    p4 < 0
  )
  {
    return;
  }

  uint16_t node =
    line.substring(
      p1 + 1,
      p2
    ).toInt();

  uint32_t seq =
    strtoul(
      line.substring(
        p2 + 1,
        p3
      ).c_str(),
      NULL,
      10
    );

  String lat =
    line.substring(
      p3 + 1,
      p4
    );

  String lon;

  int hop = 0;
  int rssi = 0;

  if (p5 >= 0)
  {
    lon =
      line.substring(
        p4 + 1,
        p5
      );
  }
  else
  {
    lon =
      line.substring(
        p4 + 1
      );
  }

  if (p6 >= 0)
  {
    hop =
      line.substring(
        p5 + 1,
        p6
      ).toInt();

    rssi =
      line.substring(
        p6 + 1
      ).toInt();
  }

  int index =
    findNode(node);

  if (index < 0)
  {
    index =
      allocateNodeSlot();

    nodes[index].valid =
      true;

    nodes[index].nodeId =
      node;

    nodes[index].firstSeen =
      millis();
  }

  nodes[index].sequence =
    seq;

  nodes[index].latitude =
    lat;

  nodes[index].longitude =
    lon;

  nodes[index].hop =
    hop;

  nodes[index].rssi =
    rssi;

  nodes[index].lastSeen =
    millis();

  saveNodes();

  Serial.println(
    "[OK] GPS location updated"
  );
}

/* =========================================================
                       PROCESS ACK
========================================================= */

void processACK(
  String line
)
{
  /*
     ACK,node,seq
  */

  int p1 =
    line.indexOf(',');

  int p2 =
    line.indexOf(
      ',',
      p1 + 1
    );

  if (p2 < 0)
    return;

  uint16_t node =
    line.substring(
      p1 + 1,
      p2
    ).toInt();

  uint32_t seq =
    strtoul(
      line.substring(
        p2 + 1
      ).c_str(),
      NULL,
      10
    );

  rx1ACK++;

  updateMessageStatus(
    node,
    seq,
    "ACKNOWLEDGED"
  );

  updateIncidentStatus(
    node,
    "ACKNOWLEDGED"
  );

  Serial.println(
    "[OK] ACK received"
  );
}

/* =========================================================
                   PROCESS MESSAGE ACK
========================================================= */

void processMSGACK(
  String line
)
{
  /*
     MSG_ACK,node,seq,status
  */

  int p1 = line.indexOf(',');
  int p2 = line.indexOf(',', p1 + 1);
  int p3 = line.indexOf(',', p2 + 1);

  if (p3 < 0)
    return;

  uint16_t node =
    line.substring(
      p1 + 1,
      p2
    ).toInt();

  uint32_t seq =
    strtoul(
      line.substring(
        p2 + 1,
        p3
      ).c_str(),
      NULL,
      10
    );

  String status =
    line.substring(
      p3 + 1
    );

  status.trim();

  updateMessageStatus(
    node,
    seq,
    status
  );

  Serial.println(
    "[OK] Message delivery status updated"
  );
}

/* =========================================================
                  PROCESS RESOLVE ACK
========================================================= */

void processRESOLVEACK(
  String line
)
{
  /*
     RESOLVE_ACK,node,seq
  */

  int p1 = line.indexOf(',');
  int p2 = line.indexOf(',', p1 + 1);

  if (p2 < 0)
    return;

  uint16_t node =
    line.substring(
      p1 + 1,
      p2
    ).toInt();

  updateIncidentStatus(
    node,
    "RESOLVED"
  );

  int index =
    findNode(node);

  if (index >= 0)
  {
    nodes[index].sosActive =
      false;

    nodes[index].resolved =
      true;

    saveNodes();
  }
}

/* =========================================================
                    RX1 STATUS
========================================================= */

void processRX1Status(
  String line
)
{
  /*
     RX1_STATUS,rx,tx,sos,ack,loc,msg
  */

  int p1 = line.indexOf(',');
  int p2 = line.indexOf(',', p1 + 1);
  int p3 = line.indexOf(',', p2 + 1);
  int p4 = line.indexOf(',', p3 + 1);
  int p5 = line.indexOf(',', p4 + 1);
  int p6 = line.indexOf(',', p5 + 1);

  if (p6 < 0)
    return;

  rx1LoRaRx =
    line.substring(
      p1 + 1,
      p2
    ).toInt();

  rx1LoRaTx =
    line.substring(
      p2 + 1,
      p3
    ).toInt();

  rx1SOS =
    line.substring(
      p3 + 1,
      p4
    ).toInt();

  rx1ACK =
    line.substring(
      p4 + 1,
      p5
    ).toInt();

  rx1LOC =
    line.substring(
      p5 + 1,
      p6
    ).toInt();

  rx1MSG =
    line.substring(
      p6 + 1
    ).toInt();

  rx1Online = true;

  rx1LastSeen =
    millis();
}

/* =========================================================
                    TX STATUS
========================================================= */

void processTXStatus(
  String line
)
{
  /*
     TX_STATUS,SENT,node,seq,message
  */

  int p1 = line.indexOf(',');
  int p2 = line.indexOf(',', p1 + 1);
  int p3 = line.indexOf(',', p2 + 1);
  int p4 = line.indexOf(',', p3 + 1);

  if (p3 < 0)
    return;

  String status =
    line.substring(
      p1 + 1,
      p2
    );

  uint16_t node =
    line.substring(
      p2 + 1,
      p3
    ).toInt();

  uint32_t seq = 0;

  String message = "";

  if (p4 >= 0)
  {
    seq =
      strtoul(
        line.substring(
          p3 + 1,
          p4
        ).c_str(),
        NULL,
        10
      );

    message =
      line.substring(
        p4 + 1
      );
  }
  else
  {
    seq =
      strtoul(
        line.substring(
          p3 + 1
        ).c_str(),
        NULL,
        10
      );
  }

  addMessageLog(
    node,
    seq,
    message,
    status
  );
}

/* =========================================================
                 SEND COMMAND TO RX1
========================================================= */

void sendCommandToRX1(
  String type,
  uint16_t nodeId,
  String payload
)
{
  payload.replace("\r", " ");
  payload.replace("\n", " ");
  payload.replace(",", " ");

  String command =
    "TL," +
    type +
    "," +
    String(nodeId);

  if (payload.length() > 0)
  {
    command += ",";
    command += payload;
  }

  rescueSerial.println(
    command
  );

  rescueSerial.flush();

  Serial.print(
    "[UART TX -> RX1] "
  );

  Serial.println(
    command
  );

  webMessage =
    "Command sent";

  webMessageTime =
    millis();
}

/* =========================================================
                  EXECUTE ACTION
========================================================= */

void executePendingAction()
{
  if (
    selectedNodeIndex < 0 ||
    !isNodeValid(selectedNodeIndex)
  )
  {
    uiState = UI_HOME;
    return;
  }

  uint16_t node =
    nodes[selectedNodeIndex].nodeId;

  String type;
  String payload;

  switch (pendingAction)
  {
    case ACT_STAY_CALM:
      type = "MSG";
      payload = "STAY CALM";
      break;

    case ACT_RESCUE:
      type = "MSG";
      payload =
        "RESCUE TEAM IS ON THE WAY";
      break;

    case ACT_DO_NOT_MOVE:
      type = "MSG";
      payload =
        "DO NOT MOVE";
      break;

    case ACT_LOCATION_AGAIN:
      type = "LOC_AGAIN";
      break;

    case ACT_HELP_APPROACHING:
      type = "MSG";
      payload =
        "HELP IS APPROACHING";
      break;

    case ACT_GPS_REQUEST:
      type = "GPS_REQUEST";
      break;

    case ACT_EMERGENCY:
      type = "EMERGENCY";
      break;

    default:
      return;
  }

  unsigned long seq =
    nodes[selectedNodeIndex].sequence;

  if (seq == 0)
    seq = millis();

  sendCommandToRX1(
    type,
    node,
    payload
  );

  if (type == "MSG" ||
      type == "EMERGENCY")
  {
    addMessageLog(
      node,
      seq,
      payload.length() > 0
        ? payload
        : "EMERGENCY PRIORITY",
      "QUEUED"
    );
  }

  uiState =
    UI_SENT_STATUS;

  showSentScreen();
}

/* =========================================================
                    RESOLVE NODE
========================================================= */

void resolveSelectedNode()
{
  if (
    selectedNodeIndex < 0 ||
    !isNodeValid(selectedNodeIndex)
  )
  {
    uiState = UI_HOME;
    return;
  }

  uint16_t node =
    nodes[selectedNodeIndex].nodeId;

  uint32_t seq =
    nodes[selectedNodeIndex].sequence;

  sendCommandToRX1(
    "RESOLVE",
    node,
    ""
  );

  nodes[selectedNodeIndex].sosActive =
    false;

  nodes[selectedNodeIndex].resolved =
    true;

  updateIncidentStatus(
    node,
    "RESOLVED"
  );

  saveNodes();

  uiState =
    UI_HOME;

  showHomeScreen();
}

/* =========================================================
                     CLEAR DATABASE
========================================================= */

void clearDatabase()
{
  initDatabase();

  LittleFS.remove("/nodes.db");
  LittleFS.remove("/incidents.db");
  LittleFS.remove("/messages.db");

  selectedNodeIndex = -1;

  Serial.println(
    "[DATABASE] Cleared"
  );
}

/* =========================================================
                       KEYPAD SCAN
========================================================= */

char scanKeypadRaw()
{
  for (int r = 0; r < 4; r++)
  {
    digitalWrite(
      rowPins[r],
      HIGH
    );
  }

  for (int r = 0; r < 4; r++)
  {
    digitalWrite(
      rowPins[r],
      LOW
    );

    delayMicroseconds(100);

    for (int c = 0; c < 4; c++)
    {
      if (
        digitalRead(
          colPins[c]
        ) == LOW
      )
      {
        digitalWrite(
          rowPins[r],
          HIGH
        );

        return keyMap[r][c];
      }
    }

    digitalWrite(
      rowPins[r],
      HIGH
    );
  }

  return 0;
}

/* =========================================================
                   KEYPAD DEBOUNCE
========================================================= */

char getKeypadKey()
{
  static char lastRaw = 0;
  static char stable = 0;

  static unsigned long changeTime = 0;

  char raw =
    scanKeypadRaw();

  if (raw != lastRaw)
  {
    lastRaw = raw;
    changeTime = millis();
  }

  if (
    millis() - changeTime >= 40
  )
  {
    if (raw != stable)
    {
      stable = raw;

      if (stable != 0)
      {
        return translateKey(stable);
      }
    }
  }

  return 0;
}

/* =========================================================
                    TRANSLATE KEYS
========================================================= */

char translateKey(char key)
{
  if (key == '*')
    return 'F';

  if (key == '#')
    return 'E';

  return key;
}

/* =========================================================
                    HANDLE KEY
========================================================= */

void handleKey(char key)
{
  if (key == '0')
  {
    uiState = UI_HOME;
    pendingAction = ACT_NONE;

    showHomeScreen();
    return;
  }

  if (uiState == UI_HOME)
  {
    if (key == '1')
    {
      if (getNodeCount() > 0)
      {
        if (
          selectedNodeIndex < 0 ||
          !isNodeValid(selectedNodeIndex)
        )
        {
          selectedNodeIndex = 0;
        }

        uiState =
          UI_NODE_SELECT;

        showNodeSelectionScreen();
      }

      return;
    }

    if (key == 'E')
    {
      if (
        selectedNodeIndex >= 0 &&
        isNodeValid(selectedNodeIndex)
      )
      {
        uiState =
          UI_NODE_INFO;

        showNodeInfoScreen();
      }

      return;
    }
  }

  if (uiState == UI_NODE_SELECT)
  {
    if (key == '2')
    {
      int start =
        selectedNodeIndex;

      do
      {
        selectedNodeIndex++;

        if (
          selectedNodeIndex >= MAX_NODES
        )
        {
          selectedNodeIndex = 0;
        }

      } while (
        !isNodeValid(selectedNodeIndex) &&
        selectedNodeIndex != start
      );

      showNodeSelectionScreen();
      return;
    }

    if (key == '3')
    {
      int start =
        selectedNodeIndex;

      do
      {
        selectedNodeIndex--;

        if (selectedNodeIndex < 0)
        {
          selectedNodeIndex =
            MAX_NODES - 1;
        }

      } while (
        !isNodeValid(selectedNodeIndex) &&
        selectedNodeIndex != start
      );

      showNodeSelectionScreen();
      return;
    }

    if (key == 'A')
    {
      uiState =
        UI_NODE_CONTROL;

      showNodeControlScreen();
      return;
    }

    if (key == 'B' ||
        key == 'C')
    {
      uiState = UI_HOME;
      showHomeScreen();
      return;
    }
  }

  if (uiState == UI_NODE_CONTROL)
  {
    switch (key)
    {
      case '4':
        pendingAction =
          ACT_STAY_CALM;
        break;

      case '5':
        pendingAction =
          ACT_RESCUE;
        break;

      case '6':
        pendingAction =
          ACT_DO_NOT_MOVE;
        break;

      case '7':
        pendingAction =
          ACT_LOCATION_AGAIN;
        break;

      case '8':
        pendingAction =
          ACT_HELP_APPROACHING;
        break;

      case '9':
        pendingAction =
          ACT_GPS_REQUEST;
        break;

      case 'F':
        pendingAction =
          ACT_EMERGENCY;
        break;

      case 'D':
        uiState =
          UI_RESOLVE_CONFIRM;

        showResolveConfirmScreen();
        return;

      case 'E':
        uiState =
          UI_NODE_INFO;

        showNodeInfoScreen();
        return;

      case 'B':
        uiState =
          UI_NODE_SELECT;

        showNodeSelectionScreen();
        return;

      default:
        return;
    }

    uiState =
      UI_ACTION_CONFIRM;

    showActionConfirmScreen();

    return;
  }

  if (uiState == UI_ACTION_CONFIRM)
  {
    if (key == 'A')
    {
      executePendingAction();
      return;
    }

    if (
      key == 'B' ||
      key == 'C'
    )
    {
      uiState =
        UI_NODE_CONTROL;

      showNodeControlScreen();

      return;
    }
  }

  if (uiState == UI_RESOLVE_CONFIRM)
  {
    if (key == 'A')
    {
      resolveSelectedNode();
      return;
    }

    if (
      key == 'B' ||
      key == 'C'
    )
    {
      uiState =
        UI_NODE_CONTROL;

      showNodeControlScreen();
      return;
    }
  }

  if (uiState == UI_NODE_INFO)
  {
    if (
      key == 'B' ||
      key == 'C'
    )
    {
      uiState =
        UI_NODE_CONTROL;

      showNodeControlScreen();
    }
  }

  if (uiState == UI_SENT_STATUS)
  {
    if (
      key == 'A' ||
      key == 'B' ||
      key == 'C'
    )
    {
      uiState =
        UI_NODE_CONTROL;

      showNodeControlScreen();
    }
  }
}

/* =========================================================
                     SERIAL UI
========================================================= */

void showHomeScreen()
{
  Serial.println();
  Serial.println(
    "========================================"
  );
  Serial.println(
    "       TERRALINK RESCUE COMMAND"
  );
  Serial.println(
    "========================================"
  );

  Serial.print(
    "ACTIVE SOS : "
  );

  Serial.println(
    getActiveSOSCount()
  );

  Serial.print(
    "NODES      : "
  );

  Serial.println(
    getNodeCount()
  );

  Serial.print(
    "RX1        : "
  );

  Serial.println(
    rx1Online
      ? "ONLINE"
      : "OFFLINE"
  );

  Serial.println();

  Serial.println(
    "1 = SELECT NODE"
  );

  Serial.println(
    "E = NODE INFO"
  );
}

/* =========================================================
                NODE SELECTION SCREEN
========================================================= */

void showNodeSelectionScreen()
{
  Serial.println();
  Serial.println(
    "----------- SELECT NODE -----------"
  );

  if (
    selectedNodeIndex < 0 ||
    !isNodeValid(selectedNodeIndex)
  )
  {
    Serial.println("NO NODE");
    return;
  }

  NodeRecord &n =
    nodes[selectedNodeIndex];

  Serial.print(
    "NODE: "
  );

  Serial.println(
    n.nodeId
  );

  Serial.print(
    "STATUS: "
  );

  Serial.println(
    n.sosActive &&
    !n.resolved
      ? "SOS ACTIVE"
      : "NORMAL"
  );

  Serial.println(
    "2 NEXT"
  );

  Serial.println(
    "3 PREVIOUS"
  );

  Serial.println(
    "A SELECT"
  );

  Serial.println(
    "B BACK"
  );
}

/* =========================================================
                   NODE CONTROL
========================================================= */

void showNodeControlScreen()
{
  Serial.println();
  Serial.println(
    "----------- NODE CONTROL -----------"
  );

  if (
    selectedNodeIndex < 0 ||
    !isNodeValid(selectedNodeIndex)
  )
  {
    return;
  }

  Serial.print(
    "NODE: "
  );

  Serial.println(
    nodes[selectedNodeIndex].nodeId
  );

  Serial.println(
    "4 STAY CALM"
  );

  Serial.println(
    "5 RESCUE ON WAY"
  );

  Serial.println(
    "6 DO NOT MOVE"
  );

  Serial.println(
    "7 LOCATION AGAIN"
  );

  Serial.println(
    "8 HELP APPROACHING"
  );

  Serial.println(
    "9 GPS REQUEST"
  );

  Serial.println(
    "F EMERGENCY"
  );

  Serial.println(
    "E NODE INFO"
  );

  Serial.println(
    "D RESOLVE"
  );
}

/* =========================================================
                   ACTION CONFIRM
========================================================= */

void showActionConfirmScreen()
{
  Serial.println();

  Serial.print(
    "ACTION: "
  );

  Serial.println(
    actionName(
      pendingAction
    )
  );

  Serial.println(
    "A = SEND"
  );

  Serial.println(
    "B/C = CANCEL"
  );
}

/* =========================================================
                    NODE INFO
========================================================= */

void showNodeInfoScreen()
{
  if (
    selectedNodeIndex < 0 ||
    !isNodeValid(selectedNodeIndex)
  )
  {
    return;
  }

  NodeRecord &n =
    nodes[selectedNodeIndex];

  Serial.println();

  Serial.println(
    "----------- NODE INFORMATION -----------"
  );

  Serial.print("NODE: ");
  Serial.println(n.nodeId);

  Serial.print("SEQ: ");
  Serial.println(n.sequence);

  Serial.print("LAT: ");
  Serial.println(n.latitude);

  Serial.print("LON: ");
  Serial.println(n.longitude);

  Serial.print("RSSI: ");
  Serial.println(n.rssi);

  Serial.print("HOPS: ");
  Serial.println(n.hop);

  Serial.print("SOS: ");

  Serial.println(
    n.sosActive
      ? "ACTIVE"
      : "NO"
  );

  Serial.print("RESOLVED: ");

  Serial.println(
    n.resolved
      ? "YES"
      : "NO"
  );

  Serial.println(
    "B = BACK"
  );
}

/* =========================================================
                 RESOLVE CONFIRM
========================================================= */

void showResolveConfirmScreen()
{
  Serial.println();
  Serial.println(
    "RESOLVE THIS INCIDENT?"
  );

  Serial.println(
    "A = YES"
  );

  Serial.println(
    "B/C = CANCEL"
  );
}

/* =========================================================
                    SENT SCREEN
========================================================= */

void showSentScreen()
{
  Serial.println();
  Serial.println(
    "----------- COMMAND SENT -----------"
  );

  Serial.println(
    actionName(
      pendingAction
    )
  );

  Serial.println(
    "A/B/C = CONTINUE"
  );
}

/* =========================================================
                    ACTION NAME
========================================================= */

String actionName(
  ActionType action
)
{
  switch (action)
  {
    case ACT_STAY_CALM:
      return "STAY CALM";

    case ACT_RESCUE:
      return "RESCUE TEAM IS ON THE WAY";

    case ACT_DO_NOT_MOVE:
      return "DO NOT MOVE";

    case ACT_LOCATION_AGAIN:
      return "LOCATION AGAIN";

    case ACT_HELP_APPROACHING:
      return "HELP APPROACHING";

    case ACT_GPS_REQUEST:
      return "GPS REQUEST";

    case ACT_EMERGENCY:
      return "EMERGENCY PRIORITY";

    case ACT_RESOLVE:
      return "RESOLVE";

    default:
      return "NONE";
  }
}

/* =========================================================
                    UI STATE NAME
========================================================= */

String getUIStateName()
{
  switch (uiState)
  {
    case UI_HOME:
      return "HOME";

    case UI_NODE_SELECT:
      return "NODE_SELECT";

    case UI_NODE_CONTROL:
      return "NODE_CONTROL";

    case UI_ACTION_CONFIRM:
      return "ACTION_CONFIRM";

    case UI_NODE_INFO:
      return "NODE_INFO";

    case UI_RESOLVE_CONFIRM:
      return "RESOLVE_CONFIRM";

    case UI_SENT_STATUS:
      return "SENT";

    default:
      return "UNKNOWN";
  }
}

/* =========================================================
                     HTML ESCAPE
========================================================= */

String htmlEscape(
  String value
)
{
  value.replace(
    "&",
    "&amp;"
  );

  value.replace(
    "<",
    "&lt;"
  );

  value.replace(
    ">",
    "&gt;"
  );

  value.replace(
    "\"",
    "&quot;"
  );

  return value;
}

/* =========================================================
                    WEB SERVER SETUP
========================================================= */

void setupWebServer()
{
  server.on(
    "/",
    HTTP_GET,
    handleRoot
  );

  server.on(
    "/api/status",
    HTTP_GET,
    handleAPIStatus
  );

  server.on(
    "/api/nodes",
    HTTP_GET,
    handleAPINodes
  );

  server.on(
    "/api/incidents",
    HTTP_GET,
    handleAPIIncidents
  );

  server.on(
    "/api/messages",
    HTTP_GET,
    handleAPIMessages
  );

  server.on(
    "/cmd",
    HTTP_GET,
    handleWebCommand
  );

  server.on(
    "/resolve",
    HTTP_GET,
    handleWebResolve
  );

  server.on(
    "/clear",
    HTTP_GET,
    handleWebClear
  );
}

/* =========================================================
                    WEB ROOT
========================================================= */

void handleRoot()
{
  server.send(
    200,
    "text/html",
    makeDashboardHTML()
  );
}

/* =========================================================
                    STATUS API
========================================================= */

void handleAPIStatus()
{
  String json;

  json += "{";

  json += "\"receiver\":2,";

  json += "\"rx1_online\":";
  json += rx1Online
    ? "true,"
    : "false,";

  json += "\"nodes\":";
  json += String(
    getNodeCount()
  );
  json += ",";

  json += "\"active_sos\":";
  json += String(
    getActiveSOSCount()
  );
  json += ",";

  json += "\"rx\":";
  json += String(
    rx1LoRaRx
  );
  json += ",";

  json += "\"tx\":";
  json += String(
    rx1LoRaTx
  );
  json += ",";

  json += "\"sos_packets\":";
  json += String(
    rx1SOS
  );
  json += ",";

  json += "\"ack_packets\":";
  json += String(
    rx1ACK
  );
  json += ",";

  json += "\"loc_packets\":";
  json += String(
    rx1LOC
  );
  json += ",";

  json += "\"msg_packets\":";
  json += String(
    rx1MSG
  );

  json += "}";

  server.send(
    200,
    "application/json",
    json
  );
}

/* =========================================================
                       NODES API
========================================================= */

void handleAPINodes()
{
  String json = "[";

  bool first = true;

  for (int i = 0; i < MAX_NODES; i++)
  {
    if (!nodes[i].valid)
      continue;

    if (!first)
      json += ",";

    first = false;

    json += "{";

    json += "\"id\":";
    json += String(
      nodes[i].nodeId
    );
    json += ",";

    json += "\"seq\":";
    json += String(
      nodes[i].sequence
    );
    json += ",";

    json += "\"lat\":\"";
    json += htmlEscape(
      nodes[i].latitude
    );
    json += "\",";

    json += "\"lon\":\"";
    json += htmlEscape(
      nodes[i].longitude
    );
    json += "\",";

    json += "\"hop\":";
    json += String(
      nodes[i].hop
    );
    json += ",";

    json += "\"rssi\":";
    json += String(
      nodes[i].rssi
    );
    json += ",";

    json += "\"sos\":";
    json += nodes[i].sosActive
      ? "true,"
      : "false,";

    json += "\"resolved\":";
    json += nodes[i].resolved
      ? "true"
      : "false";

    json += "}";
  }

  json += "]";

  server.send(
    200,
    "application/json",
    json
  );
}

/* =========================================================
                    INCIDENTS API
========================================================= */

void handleAPIIncidents()
{
  String json = "[";

  bool first = true;

  for (int i = 0; i < MAX_INCIDENTS; i++)
  {
    if (!incidents[i].valid)
      continue;

    if (!first)
      json += ",";

    first = false;

    json += "{";

    json += "\"id\":\"";
    json += incidents[i].incidentId;
    json += "\",";

    json += "\"node\":";
    json += String(
      incidents[i].nodeId
    );
    json += ",";

    json += "\"seq\":";
    json += String(
      incidents[i].sequence
    );
    json += ",";

    json += "\"lat\":\"";
    json += incidents[i].latitude;
    json += "\",";

    json += "\"lon\":\"";
    json += incidents[i].longitude;
    json += "\",";

    json += "\"hop\":";
    json += String(
      incidents[i].hop
    );
    json += ",";

    json += "\"rssi\":";
    json += String(
      incidents[i].rssi
    );
    json += ",";

    json += "\"status\":\"";
    json += incidents[i].status;
    json += "\"";

    json += "}";
  }

  json += "]";

  server.send(
    200,
    "application/json",
    json
  );
}

/* =========================================================
                    MESSAGES API
========================================================= */

void handleAPIMessages()
{
  String json = "[";

  bool first = true;

  for (int i = 0; i < MAX_MESSAGES; i++)
  {
    if (!messages[i].valid)
      continue;

    if (!first)
      json += ",";

    first = false;

    json += "{";

    json += "\"node\":";
    json += String(
      messages[i].nodeId
    );
    json += ",";

    json += "\"seq\":";
    json += String(
      messages[i].sequence
    );
    json += ",";

    json += "\"message\":\"";
    json += htmlEscape(
      messages[i].message
    );
    json += "\",";

    json += "\"status\":\"";
    json += messages[i].status;
    json += "\"";

    json += "}";
  }

  json += "]";

  server.send(
    200,
    "application/json",
    json
  );
}

/* =========================================================
                   WEB COMMAND
========================================================= */

void handleWebCommand()
{
  if (
    !server.hasArg("type") ||
    !server.hasArg("node")
  )
  {
    server.send(
      400,
      "text/plain",
      "Missing type/node"
    );

    return;
  }

  String type =
    server.arg("type");

  uint16_t node =
    server.arg("node").toInt();

  String payload = "";

  if (server.hasArg("msg"))
  {
    payload =
      server.arg("msg");
  }

  sendCommandToRX1(
    type,
    node,
    payload
  );

  uint32_t seq = millis();

  if (
    type == "MSG" ||
    type == "EMERGENCY"
  )
  {
    String stored =
      payload;

    if (
      type == "EMERGENCY" &&
      stored.length() == 0
    )
    {
      stored =
        "EMERGENCY PRIORITY";
    }

    addMessageLog(
      node,
      seq,
      stored,
      "QUEUED"
    );
  }

  server.send(
    200,
    "application/json",
    "{\"ok\":true}"
  );
}

/* =========================================================
                   WEB RESOLVE
========================================================= */

void handleWebResolve()
{
  if (!server.hasArg("node"))
  {
    server.send(
      400,
      "text/plain",
      "Missing node"
    );

    return;
  }

  uint16_t node =
    server.arg("node").toInt();

  sendCommandToRX1(
    "RESOLVE",
    node,
    ""
  );

  int index =
    findNode(node);

  if (index >= 0)
  {
    nodes[index].sosActive =
      false;

    nodes[index].resolved =
      true;
  }

  updateIncidentStatus(
    node,
    "RESOLVED"
  );

  saveNodes();

  server.send(
    200,
    "application/json",
    "{\"ok\":true}"
  );
}

/* =========================================================
                     WEB CLEAR
========================================================= */

void handleWebClear()
{
  clearDatabase();

  server.send(
    200,
    "application/json",
    "{\"ok\":true}"
  );
}

/* =========================================================
                 DASHBOARD HTML
========================================================= */

String makeDashboardHTML()
{
  String html;

  html.reserve(15000);

  html += F(
    "<!DOCTYPE html>"
    "<html>"
    "<head>"
    "<meta name='viewport' content='width=device-width,initial-scale=1'>"
    "<title>TerraLink Rescue Command Center</title>"
    "<style>"

    "body{"
    "font-family:Arial,sans-serif;"
    "margin:0;"
    "background:#10151c;"
    "color:#eee;"
    "}"

    ".header{"
    "padding:20px;"
    "background:#171e27;"
    "border-bottom:1px solid #333;"
    "}"

    ".title{"
    "font-size:26px;"
    "font-weight:bold;"
    "}"

    ".sub{"
    "opacity:.7;"
    "margin-top:5px;"
    "}"

    ".grid{"
    "display:grid;"
    "grid-template-columns:repeat(auto-fit,minmax(180px,1fr));"
    "gap:12px;"
    "padding:15px;"
    "}"

    ".card{"
    "background:#181f28;"
    "border:1px solid #303944;"
    "border-radius:12px;"
    "padding:15px;"
    "}"

    ".number{"
    "font-size:30px;"
    "font-weight:bold;"
    "margin-top:8px;"
    "}"

    ".danger{"
    "border:2px solid #d33;"
    "}"

    ".section{"
    "margin:15px;"
    "padding:18px;"
    "background:#181f28;"
    "border-radius:12px;"
    "border:1px solid #303944;"
    "}"

    "button{"
    "padding:10px 14px;"
    "margin:4px;"
    "border:0;"
    "border-radius:7px;"
    "cursor:pointer;"
    "}"

    "input,select,textarea{"
    "width:100%;"
    "box-sizing:border-box;"
    "padding:10px;"
    "margin:5px 0;"
    "background:#0e1319;"
    "color:#fff;"
    "border:1px solid #39434f;"
    "border-radius:6px;"
    "}"

    "textarea{"
    "height:90px;"
    "}"

    "table{"
    "width:100%;"
    "border-collapse:collapse;"
    "}"

    "th,td{"
    "padding:9px;"
    "border-bottom:1px solid #303944;"
    "text-align:left;"
    "}"

    ".active{"
    "font-weight:bold;"
    "}"

    ".online{"
    "font-weight:bold;"
    "}"

    ".mapbox{"
    "min-height:220px;"
    "background:#0e1319;"
    "border-radius:10px;"
    "padding:20px;"
    "}"

    ".route{"
    "font-family:monospace;"
    "font-size:16px;"
    "padding:10px;"
    "}"

    "</style>"
    "</head>"

    "<body>"

    "<div class='header'>"
    "<div class='title'>"
    "TERRALINK RESCUE COMMAND CENTER"
    "</div>"
    "<div class='sub'>"
    "Offline Emergency Communication Gateway"
    "</div>"
    "</div>"

    "<div class='grid'>"

    "<div class='card'>"
    "<div>ACTIVE SOS</div>"
    "<div id='sos' class='number'>0</div>"
    "</div>"

    "<div class='card'>"
    "<div>KNOWN NODES</div>"
    "<div id='nodes' class='number'>0</div>"
    "</div>"

    "<div class='card'>"
    "<div>RX1 GATEWAY</div>"
    "<div id='rx1' class='number'>OFFLINE</div>"
    "</div>"

    "<div class='card'>"
    "<div>LoRa RX</div>"
    "<div id='rx' class='number'>0</div>"
    "</div>"

    "<div class='card'>"
    "<div>LoRa TX</div>"
    "<div id='tx' class='number'>0</div>"
    "</div>"

    "<div class='card'>"
    "<div>SOS PACKETS</div>"
    "<div id='sosp' class='number'>0</div>"
    "</div>"

    "</div>"

    "<div class='section'>"
    "<h2>🚨 Active Incidents</h2>"
    "<div id='incidents'>Loading...</div>"
    "</div>"

    "<div class='section'>"
    "<h2>📍 Live Node Information</h2>"
    "<div id='nodeTable'>Loading...</div>"
    "</div>"

    "<div class='section'>"
    "<h2>📡 Mesh Route</h2>"
    "<div class='mapbox'>"
    "<div class='route'>"
    "VICTIM → RELAY → RELAY → RX1 → RX2"
    "</div>"
    "<p>"
    "Hop count and RSSI are taken from the received LoRa packet."
    "</p>"
    "</div>"
    "</div>"

    "<div class='section'>"
    "<h2>💬 Rescue Communication</h2>"

    "<label>Target Node</label>"
    "<input id='target' type='number' placeholder='Node ID'>"

    "<label>Quick Message</label>"

    "<button onclick=\"sendMsg('STAY CALM')\">"
    "STAY CALM"
    "</button>"

    "<button onclick=\"sendMsg('RESCUE TEAM IS ON THE WAY')\">"
    "RESCUE ON WAY"
    "</button>"

    "<button onclick=\"sendMsg('DO NOT MOVE')\">"
    "DO NOT MOVE"
    "</button>"

    "<button onclick=\"sendMsg('HELP IS APPROACHING')\">"
    "HELP APPROACHING"
    "</button>"

    "<button onclick=\"sendType('GPS_REQUEST')\">"
    "REQUEST GPS"
    "</button>"

    "<button onclick=\"sendType('LOC_AGAIN')\">"
    "LOCATION AGAIN"
    "</button>"

    "<button onclick=\"sendType('EMERGENCY')\">"
    "🚨 EMERGENCY"
    "</button>"

    "<h3>Custom Message</h3>"

    "<textarea id='custom' "
    "placeholder='Type message to victim...'></textarea>"

    "<button onclick='sendCustom()'>"
    "SEND CUSTOM MESSAGE"
    "</button>"

    "</div>"

    "<div class='section'>"
    "<h2>📨 Communication Log</h2>"
    "<div id='messages'>Loading...</div>"
    "</div>"

    "<div class='section'>"
    "<h2>⚙ System</h2>"

    "<p>"
    "Wi-Fi AP: TerraLink-RX2"
    "</p>"

    "<p>"
    "IP: 192.168.4.1"
    "</p>"

    "<button onclick='location.reload()'>"
    "REFRESH"
    "</button>"

    "<button onclick=\"clearDB()\">"
    "CLEAR DATABASE"
    "</button>"

    "</div>"

    "<script>"

    "function sendType(type){"
    "let n=document.getElementById('target').value;"
    "if(!n){alert('Enter target node');return;}"
    "fetch('/cmd?type='+encodeURIComponent(type)+'&node='+encodeURIComponent(n))"
    ".then(()=>alert('Command sent'));"
    "}"

    "function sendMsg(msg){"
    "let n=document.getElementById('target').value;"
    "if(!n){alert('Enter target node');return;}"
    "fetch('/cmd?type=MSG&node='+encodeURIComponent(n)+'&msg='+encodeURIComponent(msg))"
    ".then(()=>alert('Message queued'));"
    "}"

    "function sendCustom(){"
    "let n=document.getElementById('target').value;"
    "let m=document.getElementById('custom').value;"
    "if(!n||!m){alert('Enter node and message');return;}"
    "fetch('/cmd?type=MSG&node='+encodeURIComponent(n)+'&msg='+encodeURIComponent(m))"
    ".then(()=>alert('Custom message queued'));"
    "}"

    "function resolveNode(n){"
    "if(!confirm('Resolve SOS for node '+n+'?'))return;"
    "fetch('/resolve?node='+n)"
    ".then(()=>loadAll());"
    "}"

    "function clearDB(){"
    "if(!confirm('Clear all local records?'))return;"
    "fetch('/clear')"
    ".then(()=>location.reload());"
    "}"

    "function loadStatus(){"
    "fetch('/api/status')"
    ".then(r=>r.json())"
    ".then(d=>{"
    "document.getElementById('sos').innerText=d.active_sos;"
    "document.getElementById('nodes').innerText=d.nodes;"
    "document.getElementById('rx1').innerText=d.rx1_online?'ONLINE':'OFFLINE';"
    "document.getElementById('rx').innerText=d.rx;"
    "document.getElementById('tx').innerText=d.tx;"
    "document.getElementById('sosp').innerText=d.sos_packets;"
    "});"
    "}"

    "function loadNodes(){"
    "fetch('/api/nodes')"
    ".then(r=>r.json())"
    ".then(data=>{"
    "let h='<table><tr><th>Node</th><th>SOS</th><th>GPS</th><th>Hops</th><th>RSSI</th><th>Action</th></tr>';"
    "data.forEach(n=>{"
    "h+='<tr>';"
    "h+='<td><b>'+n.id+'</b></td>';"
    "h+='<td>'+(n.sos?'🔴 ACTIVE':'NORMAL')+'</td>';"
    "h+='<td>'+n.lat+', '+n.lon+'</td>';"
    "h+='<td>'+n.hop+'</td>';"
    "h+='<td>'+n.rssi+' dBm</td>';"
    "h+='<td>';"
    "h+='<button onclick=\"document.getElementById(\\'target\\').value='+n.id+'\">SELECT</button>';"
    "if(n.sos){"
    "h+='<button onclick=\"resolveNode('+n.id+')\">RESOLVE</button>';"
    "}"
    "h+='</td>';"
    "h+='</tr>';"
    "});"
    "h+='</table>';"
    "document.getElementById('nodeTable').innerHTML=h;"
    "});"
    "}"

    "function loadIncidents(){"
    "fetch('/api/incidents')"
    ".then(r=>r.json())"
    ".then(data=>{"
    "if(data.length==0){"
    "document.getElementById('incidents').innerHTML='No incidents';"
    "return;"
    "}"
    "let h='<table><tr><th>Incident</th><th>Node</th><th>Status</th><th>GPS</th><th>Hops</th><th>RSSI</th><th>Action</th></tr>';"
    "data.slice().reverse().forEach(i=>{"
    "h+='<tr>';"
    "h+='<td>'+i.id+'</td>';"
    "h+='<td>'+i.node+'</td>';"
    "h+='<td>'+i.status+'</td>';"
    "h+='<td>'+i.lat+', '+i.lon+'</td>';"
    "h+='<td>'+i.hop+'</td>';"
    "h+='<td>'+i.rssi+'</td>';"
    "h+='<td><button onclick=\"document.getElementById(\\'target\\').value='+i.node+'\">CONTROL</button></td>';"
    "h+='</tr>';"
    "});"
    "h+='</table>';"
    "document.getElementById('incidents').innerHTML=h;"
    "});"
    "}"

    "function loadMessages(){"
    "fetch('/api/messages')"
    ".then(r=>r.json())"
    ".then(data=>{"
    "if(data.length==0){"
    "document.getElementById('messages').innerHTML='No messages';"
    "return;"
    "}"
    "let h='<table><tr><th>Node</th><th>Seq</th><th>Message</th><th>Status</th></tr>';"
    "data.slice().reverse().forEach(m=>{"
    "h+='<tr>';"
    "h+='<td>'+m.node+'</td>';"
    "h+='<td>'+m.seq+'</td>';"
    "h+='<td>'+m.message+'</td>';"
    "h+='<td>'+m.status+'</td>';"
    "h+='</tr>';"
    "});"
    "h+='</table>';"
    "document.getElementById('messages').innerHTML=h;"
    "});"
    "}"

    "function loadAll(){"
    "loadStatus();"
    "loadNodes();"
    "loadIncidents();"
    "loadMessages();"
    "}"

    "loadAll();"

    "setInterval(loadAll,3000);"

    "</script>"

    "</body>"
    "</html>"
  );

  return html;
}

/* =========================================================
                    SAVE NODES
========================================================= */

void saveNodes()
{
  File file =
    LittleFS.open(
      "/nodes.db",
      "w"
    );

  if (!file)
    return;

  for (int i = 0; i < MAX_NODES; i++)
  {
    if (!nodes[i].valid)
      continue;

    file.print(
      nodes[i].nodeId
    );

    file.print("|");

    file.print(
      nodes[i].sequence
    );

    file.print("|");

    file.print(
      nodes[i].latitude
    );

    file.print("|");

    file.print(
      nodes[i].longitude
    );

    file.print("|");

    file.print(
      nodes[i].hop
    );

    file.print("|");

    file.print(
      nodes[i].rssi
    );

    file.print("|");

    file.print(
      nodes[i].sosActive
        ? 1
        : 0
    );

    file.print("|");

    file.println(
      nodes[i].resolved
        ? 1
        : 0
    );
  }

  file.close();
}

/* =========================================================
                  SAVE INCIDENTS
========================================================= */

void saveIncidents()
{
  File file =
    LittleFS.open(
      "/incidents.db",
      "w"
    );

  if (!file)
    return;

  for (int i = 0; i < MAX_INCIDENTS; i++)
  {
    if (!incidents[i].valid)
      continue;

    file.print(
      incidents[i].incidentId
    );

    file.print("|");

    file.print(
      incidents[i].nodeId
    );

    file.print("|");

    file.print(
      incidents[i].sequence
    );

    file.print("|");

    file.print(
      incidents[i].latitude
    );

    file.print("|");

    file.print(
      incidents[i].longitude
    );

    file.print("|");

    file.print(
      incidents[i].hop
    );

    file.print("|");

    file.print(
      incidents[i].rssi
    );

    file.print("|");

    file.println(
      incidents[i].status
    );
  }

  file.close();
}

/* =========================================================
                   SAVE MESSAGES
========================================================= */

void saveMessages()
{
  File file =
    LittleFS.open(
      "/messages.db",
      "w"
    );

  if (!file)
    return;

  for (int i = 0; i < MAX_MESSAGES; i++)
  {
    if (!messages[i].valid)
      continue;

    String msg =
      messages[i].message;

    msg.replace(
      "|",
      "/"
    );

    file.print(
      messages[i].nodeId
    );

    file.print("|");

    file.print(
      messages[i].sequence
    );

    file.print("|");

    file.print(
      msg
    );

    file.print("|");

    file.println(
      messages[i].status
    );
  }

  file.close();
}

/* =========================================================
                    LOAD NODES
========================================================= */

void loadNodes()
{
  if (
    !LittleFS.exists(
      "/nodes.db"
    )
  )
    return;

  File file =
    LittleFS.open(
      "/nodes.db",
      "r"
    );

  if (!file)
    return;

  int index = 0;

  while (
    file.available() &&
    index < MAX_NODES
  )
  {
    String line =
      file.readStringUntil(
        '\n'
      );

    line.trim();

    if (line.length() == 0)
      continue;

    int p[7];

    int last = -1;

    bool validLine = true;

    for (int j = 0; j < 7; j++)
    {
      p[j] =
        line.indexOf(
          '|',
          last + 1
        );

      if (p[j] < 0)
      {
        validLine = false;
        break;
      }

      last = p[j];
    }

    if (!validLine)
      continue;

    nodes[index].valid = true;

    nodes[index].nodeId =
      line.substring(
        0,
        p[0]
      ).toInt();

    nodes[index].sequence =
      strtoul(
        line.substring(
          p[0] + 1,
          p[1]
        ).c_str(),
        NULL,
        10
      );

    nodes[index].latitude =
      line.substring(
        p[1] + 1,
        p[2]
      );

    nodes[index].longitude =
      line.substring(
        p[2] + 1,
        p[3]
      );

    nodes[index].hop =
      line.substring(
        p[3] + 1,
        p[4]
      ).toInt();

    nodes[index].rssi =
      line.substring(
        p[4] + 1,
        p[5]
      ).toInt();

    nodes[index].sosActive =
      line.substring(
        p[5] + 1,
        p[6]
      ).toInt();

    nodes[index].resolved =
      line.substring(
        p[6] + 1
      ).toInt();

    nodes[index].lastSeen =
      millis();

    index++;
  }

  file.close();
}

/* =========================================================
                  LOAD INCIDENTS
========================================================= */

void loadIncidents()
{
  if (
    !LittleFS.exists(
      "/incidents.db"
    )
  )
    return;

  File file =
    LittleFS.open(
      "/incidents.db",
      "r"
    );

  if (!file)
    return;

  int index = 0;

  while (
    file.available() &&
    index < MAX_INCIDENTS
  )
  {
    String line =
      file.readStringUntil(
        '\n'
      );

    line.trim();

    if (line.length() == 0)
      continue;

    String fields[8];

    int start = 0;

    bool ok = true;

    for (int f = 0; f < 7; f++)
    {
      int pos =
        line.indexOf(
          '|',
          start
        );

      if (pos < 0)
      {
        ok = false;
        break;
      }

      fields[f] =
        line.substring(
          start,
          pos
        );

      start = pos + 1;
    }

    if (!ok)
      continue;

    fields[7] =
      line.substring(
        start
      );

    incidents[index].valid = true;

    incidents[index].incidentId =
      fields[0];

    incidents[index].nodeId =
      fields[1].toInt();

    incidents[index].sequence =
      strtoul(
        fields[2].c_str(),
        NULL,
        10
      );

    incidents[index].latitude =
      fields[3];

    incidents[index].longitude =
      fields[4];

    incidents[index].hop =
      fields[5].toInt();

    incidents[index].rssi =
      fields[6].toInt();

    incidents[index].status =
      fields[7];

    incidents[index].createdAt =
      millis();

    incidents[index].updatedAt =
      millis();

    index++;
  }

  file.close();
}

/* =========================================================
                  LOAD MESSAGES
========================================================= */

void loadMessages()
{
  if (
    !LittleFS.exists(
      "/messages.db"
    )
  )
    return;

  File file =
    LittleFS.open(
      "/messages.db",
      "r"
    );

  if (!file)
    return;

  int index = 0;

  while (
    file.available() &&
    index < MAX_MESSAGES
  )
  {
    String line =
      file.readStringUntil(
        '\n'
      );

    line.trim();

    if (line.length() == 0)
      continue;

    int p1 =
      line.indexOf('|');

    int p2 =
      line.indexOf(
        '|',
        p1 + 1
      );

    int p3 =
      line.indexOf(
        '|',
        p2 + 1
      );

    if (
      p1 < 0 ||
      p2 < 0 ||
      p3 < 0
    )
    {
      continue;
    }

    messages[index].valid =
      true;

    messages[index].nodeId =
      line.substring(
        0,
        p1
      ).toInt();

    messages[index].sequence =
      strtoul(
        line.substring(
          p1 + 1,
          p2
        ).c_str(),
        NULL,
        10
      );

    messages[index].message =
      line.substring(
        p2 + 1,
        p3
      );

    messages[index].status =
      line.substring(
        p3 + 1
      );

    messages[index].timestamp =
      millis();

    index++;
  }

  file.close();
}

/* =========================================================
                         SETUP
========================================================= */

void setup()
{
  Serial.begin(115200);

  delay(500);

  Serial.println();
  Serial.println(
    "============================================"
  );
  Serial.println(
    "       TERRALINK RECEIVER #2"
  );
  Serial.println(
    "        RESCUE COMMAND CENTER"
  );
  Serial.println(
    "============================================"
  );

  /* -------------------------------------------------------
                           UART
  ------------------------------------------------------- */

  rescueSerial.begin(
    UART_BAUD
  );

  Serial.println(
    "[OK] UART RX2 initialized"
  );

  /* -------------------------------------------------------
                          KEYPAD
  ------------------------------------------------------- */

  for (int r = 0; r < 4; r++)
  {
    pinMode(
      rowPins[r],
      OUTPUT
    );

    digitalWrite(
      rowPins[r],
      HIGH
    );
  }

  for (int c = 0; c < 4; c++)
  {
    pinMode(
      colPins[c],
      INPUT_PULLUP
    );
  }

  Serial.println(
    "[OK] Keypad initialized"
  );

  /* -------------------------------------------------------
                         DATABASE
  ------------------------------------------------------- */

  initDatabase();

  if (
    LittleFS.begin()
  )
  {
    Serial.println(
      "[OK] LittleFS mounted"
    );

    loadNodes();
    loadIncidents();
    loadMessages();

    Serial.print(
      "[DB] Nodes: "
    );

    Serial.println(
      getNodeCount()
    );
  }
  else
  {
    Serial.println(
      "[ERROR] LittleFS failed"
    );
  }

  /* -------------------------------------------------------
                        WIFI AP
  ------------------------------------------------------- */

  WiFi.mode(
    WIFI_AP
  );

  if (
    WiFi.softAP(
      AP_SSID,
      AP_PASSWORD
    )
  )
  {
    Serial.println(
      "[OK] WiFi AP started"
    );

    Serial.print(
      "SSID: "
    );

    Serial.println(
      AP_SSID
    );

    Serial.print(
      "IP: "
    );

    Serial.println(
      WiFi.softAPIP()
    );
  }

  /* -------------------------------------------------------
                       WEB SERVER
  ------------------------------------------------------- */

  setupWebServer();

  server.begin();

  Serial.println(
    "[OK] Web server started"
  );

  Serial.println();
  Serial.println(
    "============================================"
  );
  Serial.println(
    "       RESCUE COMMAND CENTER READY"
  );
  Serial.println(
    "============================================"
  );

  showHomeScreen();
}

/* =========================================================
                          LOOP
========================================================= */

void loop()
{
  processUART();

  char key =
    getKeypadKey();

  if (key != 0)
  {
    handleKey(key);
  }

  server.handleClient();

  /*
     RX1 timeout indication.
  */

  if (
    rx1Online &&
    millis() - rx1LastSeen > 10000
  )
  {
    rx1Online = false;
  }

  yield();
}