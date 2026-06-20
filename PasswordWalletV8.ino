#include <LiquidCrystal_I2C.h>
#include <Wire.h>
#include <BleKeyboard.h>
#include <ArduinoJson.h>
#include <LittleFS.h>
#include <FS.h>
#include <WiFi.h>
#include <WebServer.h>
#include <DNSServer.h>

#define SEND 18
#define SWITCH 16

LiquidCrystal_I2C lcd(0x27, 16, 2);


bool send_laststate = HIGH;
bool switch_laststate = HIGH;


struct Account {

  String platform;
  String username;
  String password;

 };

const int MAX_ACCOUNT_NUMBER = 20;
const int MAX_PLATFORM_NUMBER = 10;

Account wallet[MAX_ACCOUNT_NUMBER];
int accountsNumber = 0;


String platforms[MAX_PLATFORM_NUMBER];
int platformsNumber = 0;

const int actionsNumber = 2;

int currentPlatformIndex = 0;
int currentAccountIndex = 0;
int currentActionIndex = 0;

const char *jsonFilePath = "/wallet.json";

bool configMode = false;

unsigned long sendClickTime = 0;
bool isHoldingSend = false;

unsigned long switchClickTime = 0;
bool isHoldingSwitch = false;

const byte DNS_PORT = 53;
IPAddress apIP(192,168,4,1);
DNSServer dnsServer;
WebServer server(80);

// Password Global Variabls

bool dash = 1;
bool dot = 0;
const int passLength = 8;
bool morseString[passLength];
bool passwordString[passLength] = {0,1,1,1,0,0,1,1};
bool touchLastState = false;
int counter = 0;
bool isHoldingTouch = false;
bool actionTakenTouch = false;
unsigned long prevTouchTime;
bool unlocked = 0;
const char index_html[] PROGMEM = R"rawliteral(

<!DOCTYPE html>
<html>
<head>
    <title>Seif Password Wallet</title>
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <style>
        body { font-family: Arial; text-align: center; margin-top: 50px; background-color: #f4f4f4; }
        form { display: inline-block; background: white; padding: 20px; border-radius: 10px; box-shadow: 0px 0px 10px rgba(0,0,0,0.1); }
        input { display: block; margin: 10px auto; padding: 10px; width: 80%; border: 1px solid #ccc; border-radius: 5px; }
        input[type="submit"] { background-color: #007BFF; color: white; cursor: pointer; border: none; font-weight: bold; }
    </style>
</head>
<body>
    <h2>Add New Account </h2>
    <form action="/save" method="POST">
        <input type="text" name="platform" placeholder="Platform (e.g. Gmail)" required>
        <input type="text" name="username" placeholder="Username / Email" required>
        <input type="password" name="password" placeholder="Password" required>
        <input type="submit" value="Save Account">
        <br>
            <button onclick="window.location.href='/accounts'" style="margin-top: 15px; padding: 10px 20px; background-color: #28a745; color: white; border: none; border-radius: 5px; cursor: pointer; font-weight: bold;">
                Show Current Accounts
            </button>
    </form>
</body>
</html>
)rawliteral";

const char index_accounts[] PROGMEM = R"rawliteral(
    <style>
    * { box-sizing: border-box; margin: 0; padding: 0; }
    body { font-family: Arial, sans-serif; background: #f4f4f4; min-height: 100vh; padding: 24px 16px; }
    h2 { text-align: center; font-size: 20px; font-weight: 700; color: #222; margin-bottom: 20px; }
    .card { background: white; border-radius: 10px; box-shadow: 0 0 10px rgba(0,0,0,0.1); max-width: 480px; margin: 0 auto; overflow: hidden; }
    .account-item { display: flex; align-items: center; padding: 14px 18px; border-bottom: 1px solid #f0f0f0; gap: 12px; }
    .account-item:last-child { border-bottom: none; }
    .platform-icon { width: 38px; height: 38px; border-radius: 8px; background: #e8f0fe; display: flex; align-items: center; justify-content: center; font-weight: 700; font-size: 15px; color: #3a6ed8; flex-shrink: 0; }
    .account-info { flex: 1; min-width: 0; }
    .platform-name { font-size: 14px; font-weight: 700; color: #222; margin-bottom: 2px; }
    .username { font-size: 12px; color: #888; white-space: nowrap; overflow: hidden; text-overflow: ellipsis; }
    .actions { display: flex; gap: 6px; flex-shrink: 0; }
    .btn-icon { width: 32px; height: 32px; border: none; border-radius: 7px; cursor: pointer; display: flex; align-items: center; justify-content: center; transition: background 0.15s; }
    .btn-edit { background: #e8f4ff; color: #1a6fc4; }
    .btn-edit:hover { background: #cce3f8; }
    .btn-delete { background: #fff0f0; color: #c0392b; }
    .btn-delete:hover { background: #fdd; }
    svg { width: 16px; height: 16px; fill: none; stroke: currentColor; stroke-width: 2; stroke-linecap: round; stroke-linejoin: round; }
    .empty { text-align: center; padding: 40px 20px; color: #aaa; font-size: 14px; }

    .modal-overlay { display: none; position: fixed; inset: 0; background: rgba(0,0,0,0.45); z-index: 100; align-items: center; justify-content: center; }
    .modal-overlay.active { display: flex; }
    .modal { background: white; border-radius: 12px; padding: 22px; width: 90%; max-width: 340px; box-shadow: 0 8px 30px rgba(0,0,0,0.15); }
    .modal h3 { font-size: 16px; font-weight: 700; margin-bottom: 16px; color: #222; }
    .modal input { display: block; width: 100%; margin: 8px 0; padding: 9px 12px; border: 1px solid #ccc; border-radius: 6px; font-size: 14px; outline: none; }
    .modal input:focus { border-color: #007BFF; }
    .modal-actions { display: flex; gap: 8px; margin-top: 16px; justify-content: flex-end; }
    .btn-save { background: #007BFF; color: white; border: none; padding: 8px 18px; border-radius: 6px; cursor: pointer; font-size: 14px; font-weight: 600; }
    .btn-cancel { background: #f0f0f0; color: #555; border: none; padding: 8px 14px; border-radius: 6px; cursor: pointer; font-size: 14px; }

    .confirm-overlay { display: none; position: fixed; inset: 0; background: rgba(0,0,0,0.45); z-index: 100; align-items: center; justify-content: center; }
    .confirm-overlay.active { display: flex; }
    .confirm-box { background: white; border-radius: 12px; padding: 24px; width: 90%; max-width: 300px; text-align: center; }
    .confirm-box p { font-size: 14px; color: #444; margin-bottom: 18px; line-height: 1.5; }
    .btn-confirm-del { background: #e74c3c; color: white; border: none; padding: 8px 18px; border-radius: 6px; cursor: pointer; font-size: 14px; font-weight: 600; }
    </style>

    <h2>&#128273; Seif Password Wallet</h2>

    <div class="card" id="accountList"></div>

    <div class="modal-overlay" id="editModal">
      <div class="modal">
        <h3>Edit Account</h3>
        <input type="text" id="editPlatform" placeholder="Platform (e.g. Gmail)">
        <input type="text" id="editUsername" placeholder="Username / Email">
        <input type="password" id="editPassword" placeholder="New Password">
        <div class="modal-actions">
          <button class="btn-cancel" onclick="closeEdit()">Cancel</button>
          <button class="btn-save" onclick="saveEdit()">Save</button>
        </div>
      </div>
    </div>

    <div class="confirm-overlay" id="confirmModal">
      <div class="confirm-box">
        <p id="confirmText">Delete this account?</p>
        <div style="display:flex;gap:8px;justify-content:center;">
          <button class="btn-cancel" onclick="closeConfirm()">Cancel</button>
          <button class="btn-confirm-del" onclick="confirmDelete()">Delete</button>
        </div>
      </div>
    </div>

    <script>
    var accounts = [];

    function loadAccounts() {
      fetch('/accounts/show')
        .then(response => response.json())
        .then(data => {
          accounts = data.map((item, index) => {
            return {
              id: index + 1,
              platform: item.platform,
              username: item.user,
              password: item.pass
            };
          });
          renderAccounts();
        })
        .catch(error => {
          console.error('Error fetching data:', error);
          document.getElementById('accountList').innerHTML = '<div class="empty">Error loading accounts from ESP32</div>';
        });
    }

    var editingId = null;
    var deletingId = null;

    function getInitials(name) {
      return name.substring(0, 2).toUpperCase();
    }

    function getColor(name) {
      var colors = [
        {bg:'#e8f0fe',fg:'#3a6ed8'},
        {bg:'#fce8e6',fg:'#c0392b'},
        {bg:'#e6f4ea',fg:'#1e7e34'},
        {bg:'#fff3e0',fg:'#e65100'},
        {bg:'#f3e5f5',fg:'#7b1fa2'},
        {bg:'#e0f7fa',fg:'#00695c'}
      ];
      var idx = name.charCodeAt(0) % colors.length;
      return colors[idx];
    }

    function renderAccounts() {
      var list = document.getElementById('accountList');
      if (accounts.length === 0) {
        list.innerHTML = '<div class="empty">No accounts saved yet.</div>';
        return;
      }
      list.innerHTML = accounts.map(function(acc) {
        var c = getColor(acc.platform);
        return '<div class="account-item" id="item-'+acc.id+'">' +
          '<div class="platform-icon" style="background:'+c.bg+';color:'+c.fg+'">'+getInitials(acc.platform)+'</div>' +
          '<div class="account-info">' +
            '<div class="platform-name">'+esc(acc.platform)+'</div>' +
            '<div class="username">'+esc(acc.username)+'</div>' +
          '</div>' +
          '<div class="actions">' +
            '<button class="btn-icon btn-edit" onclick="openEdit('+acc.id+')" title="Edit">' +
              '<svg viewBox="0 0 24 24"><path d="M11 4H4a2 2 0 0 0-2 2v14a2 2 0 0 0 2 2h14a2 2 0 0 0 2-2v-7"/><path d="M18.5 2.5a2.121 2.121 0 0 1 3 3L12 15l-4 1 1-4 9.5-9.5z"/></svg>' +
            '</button>' +
            '<button class="btn-icon btn-delete" onclick="askDelete('+acc.id+')" title="Delete">' +
              '<svg viewBox="0 0 24 24"><polyline points="3 6 5 6 21 6"/><path d="M19 6l-1 14a2 2 0 0 1-2 2H8a2 2 0 0 1-2-2L5 6"/><path d="M10 11v6"/><path d="M14 11v6"/><path d="M9 6V4a1 1 0 0 1 1-1h4a1 1 0 0 1 1 1v2"/></svg>' +
            '</button>' +
          '</div>' +
        '</div>';
      }).join('');
    }

    function esc(str) {
      return str.replace(/&/g,'&amp;').replace(/</g,'&lt;').replace(/>/g,'&gt;').replace(/"/g,'&quot;');
    }

    function openEdit(id) {
      var acc = accounts.find(function(a){ return a.id === id; });
      if (!acc) return;
      editingId = id;
      document.getElementById('editPlatform').value = acc.platform;
      document.getElementById('editUsername').value = acc.username;
      document.getElementById('editPassword').value = '';
      document.getElementById('editModal').classList.add('active');
    }

    function closeEdit() {
      document.getElementById('editModal').classList.remove('active');
      editingId = null;
    }

    function saveEdit() {
      if (!editingId) return;
      var p = document.getElementById('editPlatform').value.trim();
      var u = document.getElementById('editUsername').value.trim();
      var pw = document.getElementById('editPassword').value;
      if (!p || !u) return;

      var payload = {
        id: editingId,
        platform: p,
        username: u,
        password: pw
      };

      fetch('/accounts/edit', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify(payload)
      }).then(response => {
        if (response.ok) {
          closeEdit();
          loadAccounts();
        } else {
          alert("Error updating account");
        }
      });
    }

    function askDelete(id) {
      var acc = accounts.find(function(a){ return a.id === id; });
      deletingId = id;
      document.getElementById('confirmText').textContent = 'Delete "' + acc.platform + '" account? This cannot be undone.';
      document.getElementById('confirmModal').classList.add('active');
    }

    function closeConfirm() {
      document.getElementById('confirmModal').classList.remove('active');
      deletingId = null;
    }

    function confirmDelete() {
      var payload = { id: deletingId };

      fetch('/accounts/delete', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify(payload)
      }).then(response => {
        if (response.ok) {
          closeConfirm();
          loadAccounts();
        } else {
          alert("Error deleting account");
        }
      });
    }

    loadAccounts();
    </script>
)rawliteral";

enum UI_STATE {
  SELECT_PLATFORM,
  SELECT_ACCOUNT,
  SELECT_ACTION,
  LOCKED,
  UNLOCKED
};

UI_STATE currentState = SELECT_PLATFORM;

BleKeyboard blekeyboard("ESP32 Keyboard", "esp32", 100);

int getAccountsCountForPlatform(String platformName) {
  int count = 0;
  for (int i = 0; i < accountsNumber; i++) {
    if (wallet[i].platform == platformName) {
      count++;
    }
  }
  return count;
}

void lcd_in() {
  lcd.init();
  lcd.backlight();
  lcd.setCursor(0,0);
  lcd.print("Welcome To Seif");
  lcd.setCursor(0,1);
  lcd.print("Password Wallet!");
  delay(1000);
}

void keyboard_init() {
  Serial.println("Starting the Keyboard!");
  blekeyboard.begin();
}

void loadWalletFromFlash() {
  File file = LittleFS.open(jsonFilePath, FILE_READ);
  if (!file) {
    Serial.println("Failed to open file for reading!");
    return;
  }

  JsonDocument doc;
  DeserializationError error = deserializeJson(doc, file);
  file.close();

  if (error) {
    Serial.print("Failed to read JSON: ");
    Serial.println(error.c_str());
    wallet[0] = {"Gmail", "account1@gmail.com", "veryStrongPassword"};
    wallet[1] = {"Gmail", "account2@gmail.com", "unbreakablePassword"};
    accountsNumber = 2;
    platforms[0] = "Gmail";
    platformsNumber = 1;
    return;
  }

  JsonArray arr = doc.as<JsonArray>();
  accountsNumber = 0;
  platformsNumber = 0;

  for (JsonObject obj : arr) {
    if (accountsNumber >= MAX_ACCOUNT_NUMBER) break;

    wallet[accountsNumber].platform = obj["platform"].as<String>();
    wallet[accountsNumber].username = obj["user"].as<String>();
    wallet[accountsNumber].password = obj["pass"].as<String>();

    bool exists = false;
    for (int i = 0; i < platformsNumber; i++) {
      if (platforms[i] == wallet[accountsNumber].platform) {
        exists = true;
        break;
      }
    }
    if (!exists && platformsNumber < MAX_PLATFORM_NUMBER) {
      platforms[platformsNumber] = wallet[accountsNumber].platform;
      platformsNumber++;
    }
    accountsNumber++;
  }
}


void appendAccountToFlash(String platform, String username, String password) {
  JsonDocument doc;

  if (LittleFS.exists(jsonFilePath)) {
    File file = LittleFS.open(jsonFilePath, FILE_READ);
    deserializeJson(doc, file);
    file.close();
  }

  JsonArray array = doc.as<JsonArray>();
  if (array.isNull()) {
    array = doc.to<JsonArray>();
  }

  JsonObject newAcc = array.add<JsonObject>();
  newAcc["platform"] = platform;
  newAcc["user"] = username;
  newAcc["pass"] = password;

  File file = LittleFS.open(jsonFilePath, FILE_WRITE);
  if (file) {
    serializeJson(doc, file);
    file.close();
    Serial.println("Account successfully appended to Flash file!");
  } else {
    Serial.println("Failed to open file for writing during append!");
  }
}
void handleroot() {
  server.send(200, "text/html", index_html);
}

void handlesave() {
  if (server.hasArg("username") && server.hasArg("password") && server.hasArg("platform")) {
    String platform = server.arg("platform");
    String username = server.arg("username");
    String password = server.arg("password");

    if (platform == "EXIT" || platform == "exit") {
      server.send(200, "text/html", "<h1>Exiting Config Mode... ESP32 Restarting!</h1>");
      delay(1000);
      ESP.restart();
    } else {
      appendAccountToFlash(platform, username, password);
      String responseHtml = "<h1>Data Received!</h1><a href='/'>Go Back</a>";
      server.send(200, "text/html", responseHtml);
    }
  }
}

void handleShowAccounts() {
  server.send(200, "text/html", index_accounts);
}

void handleShowAccountsJson() {
    File file = LittleFS.open(jsonFilePath, "r");
    if (!file) {
        server.send(500, "text/plain", "Failed to Open Wallet File!");
        return;
    }
    server.streamFile(file, "application/json");
    file.close();
}

void handleEditAccount() {
  if (server.hasArg("plain")) {
    String body = server.arg("plain");
    JsonDocument requestDoc;
    deserializeJson(requestDoc, body);

    int id = requestDoc["id"];
    int index = id - 1;

    File file = LittleFS.open(jsonFilePath, "r");
    JsonDocument fileDoc;
    deserializeJson(fileDoc, file);
    file.close();

    JsonArray arr = fileDoc.as<JsonArray>();
    JsonObject obj = arr[index];
    obj["platform"] = requestDoc["platform"];
    obj["user"] = requestDoc["username"];

    String newPass = requestDoc["password"];
    if (newPass.length() > 0) {
      obj["pass"] = newPass;
    }

    file = LittleFS.open(jsonFilePath, "w");
    serializeJson(fileDoc, file);
    file.close();

    loadWalletFromFlash();

    server.send(200, "text/plain", "Account Updated");
  } else {
    server.send(400, "text/plain", "Bad Request");
  }
}

void handleDeleteAccount() {
  if (server.hasArg("plain")) {
    String body = server.arg("plain");
    JsonDocument requestDoc;
    deserializeJson(requestDoc, body);

    int id = requestDoc["id"];
    int index = id - 1;

    File file = LittleFS.open(jsonFilePath, "r");
    JsonDocument fileDoc;
    deserializeJson(fileDoc, file);
    file.close();

    JsonArray arr = fileDoc.as<JsonArray>();
    arr.remove(index);

    file = LittleFS.open(jsonFilePath, "w");
    serializeJson(fileDoc, file);
    file.close();

    loadWalletFromFlash();

    server.send(200, "text/plain", "Account Deleted");
  } else {
    server.send(400, "text/plain", "Bad Request");
  }
}

void updateLCD() {
  lcd.clear();


  switch (currentState) {

    case SELECT_ACTION:
    if (currentActionIndex == 0) {
      lcd.setCursor(0,0);
      lcd.print("> Send UserName");
      lcd.setCursor(0,1);
      lcd.print("Send Password");
    } else {
      lcd.setCursor(0,0);
      lcd.print("Send UserName");
      lcd.setCursor(0,1);
      lcd.print("> Send Password");
    }
    break;

    case SELECT_PLATFORM:
      lcd.setCursor(0,0);
      lcd.print("> ");
      lcd.print(platforms[currentPlatformIndex]);
      lcd.setCursor(5,1);
      lcd.print(currentPlatformIndex + 1);
      lcd.print(" / ");
      lcd.print(platformsNumber);

    break;

    case SELECT_ACCOUNT: {
      String selectedPlatform = platforms[currentPlatformIndex];
      int matchCount = 0;
      int AccPerPlat = getAccountsCountForPlatform(selectedPlatform);

      for (int i = 0; i < accountsNumber; i++) {
        if (wallet[i].platform == selectedPlatform) {
          if (matchCount == currentAccountIndex) {
            lcd.setCursor(0,0);
            lcd.print("> ");
            lcd.print(wallet[i].username);
            lcd.setCursor(5,1);
            lcd.print(currentAccountIndex + 1);
            lcd.print(" / ");
            lcd.print(AccPerPlat);
          }
          matchCount++;
        }
      }
    }
    break;

    case LOCKED:
    lcd.setCursor(2,0);
    lcd.print("Locked!");
    lcd.setCursor(0,1);
    lcd.print("Enter Password.");
    break;

    case UNLOCKED:
    lcd.setCursor(4,0);
    lcd.print("Unlocked!");
    delay(1000);
    break;
  } 

}

void setup() {
  Serial.begin(115200);
  pinMode(SEND, INPUT_PULLUP);
  pinMode(SWITCH, INPUT_PULLUP);
  lcd_in();
  currentState = LOCKED;

  if (!LittleFS.begin(true)) {
    Serial.println("LittleFS Mount Failed!");
  }

  if (!LittleFS.exists(jsonFilePath)) {
    Serial.println("Initializing clean default wallet array in Flash...");
    JsonDocument defaultDoc;
    JsonArray array = defaultDoc.to<JsonArray>();

    JsonObject acc1 = array.add<JsonObject>();
    acc1["platform"] = "Gmail";
    acc1["user"] = "GmailAccount1";
    acc1["pass"] = "veryStrongPassword";

    JsonObject acc2 = array.add<JsonObject>();
    acc2["platform"] = "Gmail";
    acc2["user"] = "hello@gmail.com";
    acc2["pass"] = "unbreakablePassword";

    File file = LittleFS.open(jsonFilePath, FILE_WRITE);
    if (file) {
      serializeJson(defaultDoc, file);
      file.close();
    }
  }

  keyboard_init();
  loadWalletFromFlash();
  updateLCD();
}

void sendData(String text) {
  if (!blekeyboard.isConnected()) return;
    if (currentState == SELECT_ACTION) {
      lcd.clear();
      lcd.setCursor(0,0);
      lcd.print("Sending Data..");
      lcd.setCursor(6,1);
      lcd.print("0.0   %");
      float step = 100 / text.length();
      float percent = 0;

      for (unsigned long i = 0; i < text.length(); i++) {
        blekeyboard.write(text[i]);
        lcd.setCursor(6,1);
        percent += step;
        lcd.print(percent);
        delay(12);
      }
      lcd.setCursor(6,1);
      lcd.print("100.0 %");
  } else {
      for (unsigned long i = 0; i < text.length(); i++) {
        blekeyboard.write(text[i]);
        delay(12);
      }
  }
  blekeyboard.releaseAll();
}

void printPassword() {
  lcd.clear();
  lcd.setCursor(3,0);
  String tempString;

  for (int i = 0; i < counter; i++) {
    tempString += "*";  
  }
  lcd.print(tempString);

}
void loop() {
    if (!unlocked) {
        int touchval = touchRead(4);
        bool touchcurrentstate = (touchval < 30);

        if (touchcurrentstate && !touchLastState) {
          prevTouchTime = millis();
          isHoldingTouch = true;
          actionTakenTouch = false; 
        }

        if (isHoldingTouch && !actionTakenTouch) {
          if ((millis() - prevTouchTime) > 300) {
            if (counter < passLength) {
              morseString[counter] = dash;
              counter++;
              printPassword();
            }
            actionTakenTouch = true; 
          }
        }

        if (!touchcurrentstate && touchLastState) {
          if (isHoldingTouch && !actionTakenTouch) {
            if (counter < passLength) {
              morseString[counter] = dot;
              counter++;
              printPassword();
            }
          }
          isHoldingTouch = false;
          actionTakenTouch = true;
        }

        if (counter == passLength) {
          for (int i = 0; i < passLength; i++) {
            if (morseString[i] != passwordString[i]) {
              lcd.clear();
              lcd.setCursor(2,0);
              lcd.print("Wrong!");
              lcd.setCursor(2,1);
              lcd.print("Try Again!");
              delay(500);
              updateLCD();
              counter = 0;
              return;
            }
          }
          counter = 0;
          currentState = UNLOCKED;
          updateLCD();
          if (digitalRead(SWITCH) == LOW) {
            configMode = true;
            lcd.clear();
            lcd.setCursor(0,0);
            lcd.print("Config Mode");
            lcd.setCursor(0,1);
            lcd.print("Ready For WiFi.....");

            WiFi.mode(WIFI_AP);
            WiFi.softAPConfig(apIP, apIP, IPAddress(255,255,255,0));
            WiFi.softAP("ESP32 PasswordWallet", "seif2005");

            dnsServer.start(DNS_PORT, "*", apIP);

            delay(300);


            server.on("/", handleroot);
            server.on("/save", HTTP_POST, handlesave);
            server.on("/accounts", HTTP_GET, handleShowAccounts);
            server.on("/accounts/show", HTTP_GET, handleShowAccountsJson);
            server.on("/accounts/edit", HTTP_POST, handleEditAccount);
            server.on("/accounts/delete", HTTP_POST, handleDeleteAccount);
            server.begin();
  


            server.onNotFound([]() {
            server.send(200, "text/html", index_html);
            });
          } else {
            currentState = SELECT_PLATFORM;
            updateLCD();
          }
          unlocked = true;
      }
    touchLastState = touchcurrentstate;
    delay(10);
    }

    if (configMode) {
      server.handleClient();
      dnsServer.processNextRequest();
      delay(2);
      return;
    }

    bool send_currentstate = digitalRead(SEND);
    bool switch_currentstate = digitalRead(SWITCH);

    // send button logic
    if (!send_currentstate && send_laststate) {
      delay(50); // Debounce
      if (!digitalRead(SEND)) {
        sendClickTime = millis();
        isHoldingSend = true;
      }
    }

    if (!send_currentstate && isHoldingSend) {
      if (currentState == SELECT_ACCOUNT && (millis() - sendClickTime >= 2000)) {
        if (blekeyboard.isConnected()) {
          String selectedPlatform = platforms[currentPlatformIndex];
          int matchCount = 0;

          for (int i = 0; i < accountsNumber; i++) {
            if (wallet[i].platform == selectedPlatform) {
              if (matchCount == currentAccountIndex) {
                sendData(wallet[i].username);
                delay(100);
                blekeyboard.write(KEY_TAB);
                delay(100);
                sendData(wallet[i].password);
                delay(100);
                blekeyboard.write(KEY_RETURN);

                lcd.clear();
                lcd.setCursor(0,0);
                lcd.print("Auto Login...");
                delay(800);
                break;
              }
              matchCount++;
            }
          }
        }

        currentState = SELECT_PLATFORM;
        currentPlatformIndex = 0;
        currentAccountIndex = 0;
        currentActionIndex = 0;
        updateLCD();
        isHoldingSend = false;
      }
    }

    if (send_currentstate && !send_laststate) {
      delay(50); // Debounce
      if (digitalRead(SEND)) {
        if (isHoldingSend) {
          isHoldingSend = false;

          switch (currentState) {
            case SELECT_PLATFORM:
              currentState = SELECT_ACCOUNT;
              currentAccountIndex = 0;
              break;

            case SELECT_ACCOUNT:
              currentState = SELECT_ACTION;
              currentActionIndex = 0;
              break;

            case SELECT_ACTION:
              if (blekeyboard.isConnected()) {
                String selectedPlatform = platforms[currentPlatformIndex];
                int matchCount = 0;
                for (int i = 0; i < accountsNumber; i++) {
                  if (wallet[i].platform == selectedPlatform) {
                    if (matchCount == currentAccountIndex) {
                      if (currentActionIndex == 0) {
                        sendData(wallet[i].username);
                        delay(100);
                        blekeyboard.write(KEY_RETURN);
                      } else {
                        sendData(wallet[i].password);
                        delay(100);
                        blekeyboard.write(KEY_RETURN);
                      }
                      break;
                    }
                    matchCount++;
                  }
                }
              }
              delay(500);
              currentState = SELECT_PLATFORM;
              currentPlatformIndex = 0;
              currentAccountIndex = 0;
              currentActionIndex = 0;
              break;

            default:
              break;
          }
          updateLCD();
        }
      }
    }

    // switch button logic

    if (!switch_currentstate && switch_laststate) {
      delay(50);
      if (!digitalRead(SWITCH)) {
        switchClickTime = millis();
        isHoldingSwitch = true;
      }
    }

    if (!switch_currentstate && isHoldingSwitch) {
      if (millis() - switchClickTime >= 1000) {
        currentState = SELECT_PLATFORM;
        currentPlatformIndex = 0;
        currentAccountIndex = 0;
        currentActionIndex = 0;
        switchClickTime = 0;
        isHoldingSwitch = false;
        updateLCD();
      }
    }

    if (switch_currentstate && !switch_laststate) {
      delay(50);
      if (digitalRead(SWITCH)) {
        if (isHoldingSwitch) {
          isHoldingSwitch = false;
          switch (currentState) {

            case SELECT_PLATFORM:
              currentPlatformIndex = (currentPlatformIndex + 1) % platformsNumber;
              break;

            case SELECT_ACCOUNT: {
                String selectedPlatform = platforms[currentPlatformIndex];
                int AccPerPlat = getAccountsCountForPlatform(selectedPlatform);
                currentAccountIndex = (currentAccountIndex + 1) % AccPerPlat;
                break;
               }

            case SELECT_ACTION:
               currentActionIndex = (currentActionIndex + 1) % actionsNumber;
               break;

            default:
             break;
          }
          updateLCD();
        }
      }

    }


    send_laststate = send_currentstate;
    switch_laststate = switch_currentstate;

    delay(10);
  }
