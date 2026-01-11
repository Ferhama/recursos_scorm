#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <ArduinoJson.h>

#include "pages.h"

const char* SSID = "ESP32-Quiz";
const char* PASS = "12345678";
const char* ROOM_PIN = "1234";
const uint8_t MAX_PLAYERS = 50;

struct Player {
  bool used = false;
  uint16_t id = 0;
  String name;
  uint8_t icon = 0;
  int32_t score = 0;
  int32_t streak = 0;
  bool answered = false;
  int8_t answer = -1;
  bool correct = false;
  uint32_t answerTime = 0;
  bool joinedThisRound = false;
};

const uint32_t QUESTION_TIME_MS = 20000;

WebServer server(80);

struct Question {
  const char* q;
  const char* a[4];
  uint8_t correct;
};

const Question QUESTIONS[] = {
  {"¿Qué partícula tiene carga negativa?", {"Protón","Neutrón","Electrón","Núcleo"}, 2},
  {"¿Qué indica el número atómico Z?", {"Electrones + neutrones","Protones","Neutrones","Masa en gramos"}, 1},
  {"En un átomo neutro, ¿qué se cumple?", {"p=n","p=e","n=e","Z=A"}, 1},
  {"¿Qué es un isótopo?", {"Mismo Z, distinto A","Mismo A, distinto Z","Distinta carga","Misma masa siempre"}, 0},
  {"A = ...", {"p + e","p + n","n + e","Z + e"}, 1},
  {"¿Dónde se concentra casi toda la masa?", {"Corteza","Nube electrónica","Núcleo","Órbitas"}, 2},
  {"¿Qué modelo introduce niveles de energía cuantizados?", {"Dalton","Thomson","Rutherford","Bohr"}, 3},
  {"El experimento de Rutherford evidenció que...", {"El átomo es macizo","Hay un núcleo pequeño y denso","Los electrones están en el núcleo","No existen protones"}, 1},
  {"¿Qué es un ion?", {"Átomo con carga neta","Átomo con muchos neutrones","Molécula neutra","Protón libre"}, 0},
  {"Un catión es...", {"Cargado negativamente","Cargado positivamente","Sin carga","Un isótopo"}, 1},
  {"Si un átomo gana 2 electrones, se convierte en...", {"Catión 2+","Anión 2-","Neutrón","Isótopo"}, 1},
  {"¿Qué partícula define el elemento químico?", {"Neutrón","Electrón","Protón (Z)","Fotón"}, 2},
  {"En la tabla periódica, los elementos se ordenan por...", {"Masa atómica","Z creciente","N creciente","Densidad"}, 1},
  {"¿Qué carga tiene el neutrón?", {"Positiva","Negativa","Neutra","Variable"}, 2},
  {"¿Qué significa 'estado excitado'?", {"Menos energía que el fundamental","Más energía que el fundamental","Sin electrones","Sin núcleo"}, 1}
};
const uint8_t NQ = sizeof(QUESTIONS)/sizeof(QUESTIONS[0]);

enum Phase : uint8_t { LOBBY=0, QUESTION=2, REVEAL=3, LEADERBOARD=4 };
Phase phase = LOBBY;

uint8_t currentQ = 0;
uint32_t questionStartMs = 0;

const char* ICONS[] = {"⚛️","🔬","🧪","📡","🛰️","🧠","📘","🧷","🔧","🎯","⭐","🟦","🟩","🧡","🟥","🟪","🟨","🟫","⬛","⬜"};
const uint8_t NICON = sizeof(ICONS)/sizeof(ICONS[0]);

Player players[MAX_PLAYERS];
uint16_t nextPlayerId = 1;

Player* findPlayer(uint16_t pid) {
  for (auto &p : players) if (p.used && p.id == pid) return &p;
  return nullptr;
}

uint8_t playerCount() {
  uint8_t c=0; for (auto &p : players) if (p.used) c++;
  return c;
}

uint8_t playersAnswered() {
  uint8_t c=0; for (auto &p : players) if (p.used && p.answered) c++;
  return c;
}

void resetForNewQuestion() {
  for (auto &p : players) {
    if (!p.used) continue;
    p.answered = false;
    p.answer = -1;
    p.correct = false;
    p.answerTime = 0;
  }
}

uint32_t timeLeftMs() {
  if (phase == QUESTION) {
    uint32_t now = millis();
    uint32_t elapsed = now - questionStartMs;
    if (elapsed >= QUESTION_TIME_MS) return 0;
    return QUESTION_TIME_MS - elapsed;
  }
  return 0;
}

bool shouldAdvanceFromQuestion() {
  if (phase != QUESTION) return false;
  if (timeLeftMs() == 0) return true;

  uint8_t total = 0, answered = 0;
  for (auto &p : players) {
    if (p.used) {
      total++;
      if (p.answered) answered++;
    }
  }
  return (total > 0 && answered == total);
}

void autoAdvance() {
  if (shouldAdvanceFromQuestion()) {
    phase = REVEAL;
  }
}

void sendJson(const String& body) {
  server.send(200, "application/json; charset=utf-8", body);
}

String jsonEscape(const String& s) {
  String o; o.reserve(s.length()+8);
  for (size_t i=0;i<s.length();i++){
    char c=s[i];
    if (c=='"') o += "\\\"";
    else if (c=='\\') o += "\\\\";
    else if (c=='\n') o += "\\n";
    else o += c;
  }
  return o;
}

void apiJoin() {
  String pin = server.hasArg("pin") ? server.arg("pin") : "";
  String name = server.hasArg("name") ? server.arg("name") : "";
  name.trim(); pin.trim();

  if (pin != ROOM_PIN) { sendJson("{\"ok\":false,\"err\":\"PIN incorrecto\"}"); return; }
  if (name.length() < 1) { sendJson("{\"ok\":false,\"err\":\"Nombre vacío\"}"); return; }

  int slot = -1;
  for (int i=0;i<MAX_PLAYERS;i++) if (!players[i].used) { slot=i; break; }
  if (slot < 0) { sendJson("{\"ok\":false,\"err\":\"Sala llena\"}"); return; }

  players[slot] = Player();
  players[slot].used = true;
  players[slot].id = nextPlayerId++;
  players[slot].name = name.substring(0,16);
  players[slot].icon = players[slot].id % NICON;

  String body = "{\"ok\":true,\"pid\":" + String(players[slot].id) +
                ",\"name\":\"" + jsonEscape(players[slot].name) + "\"" +
                ",\"icon\":\"" + String(ICONS[players[slot].icon]) + "\"}";
  sendJson(body);
}

void apiAnswer() {
  if (!server.hasArg("pid") || !server.hasArg("opt")) { sendJson("{\"ok\":false}"); return; }
  uint16_t pid = (uint16_t) server.arg("pid").toInt();
  int opt = server.arg("opt").toInt();

  Player* p = findPlayer(pid);
  if (!p) { sendJson("{\"ok\":false,\"err\":\"Jugador no existe\"}"); return; }
  if (phase != QUESTION) { sendJson("{\"ok\":false,\"err\":\"No es momento de responder\"}"); return; }
  if (opt < 0 || opt > 3) { sendJson("{\"ok\":false,\"err\":\"Opción inválida\"}"); return; }
  if (p->answered) { sendJson("{\"ok\":true}"); return; }

  p->answered = true;
  p->answer = (int8_t)opt;
  p->correct = (opt == QUESTIONS[currentQ].correct);

  uint32_t tl = timeLeftMs();
  uint32_t gained = 0;
  if (p->correct) {
    p->streak++;
    gained = 100 + (tl * 400UL) / QUESTION_TIME_MS;
    gained += (uint32_t)p->streak * 50U;
  } else {
    p->streak = 0;
  }
  p->score += (int32_t)gained;

  sendJson("{\"ok\":true}");
}

String leaderboardJson(uint8_t maxItems) {
  String out = "[";
  bool first = true;

  bool usedIdx[MAX_PLAYERS];
  for (int i=0;i<MAX_PLAYERS;i++) usedIdx[i] = players[i].used;

  for (uint8_t k=0;k<maxItems;k++) {
    int best = -1;
    for (int i=0;i<MAX_PLAYERS;i++){
      if (!usedIdx[i]) continue;
      if (best<0 || players[i].score > players[best].score) best=i;
    }
    if (best<0) break;
    usedIdx[best] = false;

    if (!first) out += ",";
    first = false;
    out += "{\"name\":\"" + jsonEscape(players[best].name) + "\",\"icon\":\"" +
           String(ICONS[players[best].icon]) + "\",\"score\":" + String(players[best].score) + "}";
  }
  out += "]";
  return out;
}

void apiState() {
  autoAdvance();

  uint16_t pid = server.hasArg("pid") ? (uint16_t)server.arg("pid").toInt() : 0;
  Player* me = pid ? findPlayer(pid) : nullptr;

  uint32_t tl = timeLeftMs();
  bool qVisible = (phase == QUESTION || phase == REVEAL);

  String body = "{";
  body += "\"pin\":\"" + String(ROOM_PIN) + "\",";
  body += "\"players\":" + String(playerCount()) + ",";
  body += "\"players_answered\":" + String(playersAnswered()) + ",";
  body += "\"phase\":" + String((uint8_t)phase) + ",";
  body += "\"time_left_ms\":" + String(tl) + ",";
  body += "\"q_index\":" + String(currentQ) + ",";
  body += "\"q_total\":" + String(NQ) + ",";
  body += "\"q_visible\":" + String(qVisible ? "true" : "false") + ",";
  body += "\"q_text\":\"" + String(QUESTIONS[currentQ].q) + "\",";
  body += "\"q_opts\":[\"" + String(QUESTIONS[currentQ].a[0]) + "\",\"" +
         String(QUESTIONS[currentQ].a[1]) + "\",\"" + String(QUESTIONS[currentQ].a[2]) +
         "\",\"" + String(QUESTIONS[currentQ].a[3]) + "\"],";
  body += "\"correct\":" + String(QUESTIONS[currentQ].correct) + ",";
  body += "\"leaderboard\":" + leaderboardJson(10);

  if (me) {
    body += ",\"me_score\":" + String(me->score);
    body += ",\"me_streak\":" + String(me->streak);
    body += ",\"me_answered\":" + String(me->answered ? "true":"false");
    body += ",\"me_correct\":" + String(me->correct ? "true":"false");
  }

  body += "}";
  sendJson(body);
}

void hostNext() {
  if (phase == LEADERBOARD) {
    currentQ = 0;
  } else if ((uint8_t)(currentQ + 1) >= NQ) {
    phase = LEADERBOARD;
    sendJson("{\"ok\":true,\"finished\":true}");
    return;
  } else {
    currentQ++;
  }

  phase = QUESTION;
  questionStartMs = millis();
  resetForNewQuestion();
  sendJson("{\"ok\":true}");
}

void hostStart() {
  currentQ = 0;
  for (auto &p : players) { if (p.used) { p.score = 0; p.streak = 0; } }
  phase = QUESTION;
  questionStartMs = millis();
  resetForNewQuestion();
  sendJson("{\"ok\":true}");
}

void hostReveal() {
  phase = REVEAL;
  sendJson("{\"ok\":true}");
}

void hostReset() {
  for (auto &p : players) p = Player();
  nextPlayerId = 1;
  currentQ = 0;
  phase = LOBBY;
  resetForNewQuestion();
  sendJson("{\"ok\":true}");
}

void setup() {
  Serial.begin(115200);
  WiFi.mode(WIFI_AP);
  WiFi.softAP(SSID, PASS);

  server.on("/", [](){ server.send(200, "text/html; charset=utf-8",
    "<div style='font-family:sans-serif;text-align:center;margin-top:50px;'>"
    "<h1>🧪 ESP32 Quiz</h1>"
    "<p><a href='/host' style='display:inline-block;padding:12px 24px;margin:8px;background:#FF6B6B;color:white;text-decoration:none;border-radius:8px;'>Panel del Host</a></p>"
    "<p><a href='/play' style='display:inline-block;padding:12px 24px;margin:8px;background:#4ECDC4;color:white;text-decoration:none;border-radius:8px;'>Unirse como Jugador</a></p>"
    "</div>"); });

  server.on("/host", [](){ server.send(200, "text/html; charset=utf-8", FPSTR(HOST_HTML)); });
  server.on("/play", [](){ server.send(200, "text/html; charset=utf-8", FPSTR(PLAY_HTML)); });

  server.on("/api/join", apiJoin);
  server.on("/api/answer", apiAnswer);
  server.on("/api/state", apiState);

  server.on("/api/host/next", hostNext);
  server.on("/api/host/start", hostStart);
  server.on("/api/host/reveal", hostReveal);
  server.on("/api/host/reset", hostReset);

  server.begin();
}

void loop() {
  server.handleClient();
}
