#include <WiFi.h>
#include <WebServer.h>
#include <ArduinoJson.h>

const char* SSID = "ESP32-Quiz";
const char* PASS = "12345678";
const char* ROOM_PIN = "1234";
const uint8_t MAX_PLAYERS = 50;
const uint32_t QUESTION_TIME_MS = 20000;
const uint32_t JOIN_TIME_MS = 10000;

WebServer server(80);

// ------------------ Banco de preguntas (15) ------------------
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

// ------------------ Estado del juego ------------------
enum Phase : uint8_t { LOBBY=0, JOINING=1, QUESTION=2, REVEAL=3, LEADERBOARD=4 };
Phase phase = LOBBY;

uint8_t currentQ = 0;
uint32_t phaseStartMs = 0;
uint32_t questionStartMs = 0;
bool questionVisible = false;

const char* ICONS[] = {"⚛️","🔬","🧪","📡","🛰️","🧠","📘","🧷","🔧","🎯","⭐","🟦","🟩","🟧","🟥","🟪","🟨","🟫","⬛","⬜"};
const uint8_t NICON = sizeof(ICONS)/sizeof(ICONS[0]);

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
  questionVisible = false;
}

void resetForNewRound() {
  for (auto &p : players) {
    if (!p.used) continue;
    p.score = 0;
    p.streak = 0;
    p.joinedThisRound = false;
  }
}

uint32_t timeLeftMs() {
  if (phase == QUESTION) {
    uint32_t now = millis();
    if (now <= questionStartMs) return QUESTION_TIME_MS;
    uint32_t elapsed = now - questionStartMs;
    if (elapsed >= QUESTION_TIME_MS) return 0;
    return QUESTION_TIME_MS - elapsed;
  }
  else if (phase == JOINING) {
    uint32_t now = millis();
    if (now <= phaseStartMs) return JOIN_TIME_MS;
    uint32_t elapsed = now - phaseStartMs;
    if (elapsed >= JOIN_TIME_MS) return 0;
    return JOIN_TIME_MS - elapsed;
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
    phaseStartMs = millis();
  }
  else if (phase == JOINING && timeLeftMs() == 0) {
    phase = LOBBY;
  }
}

// ------------------ HTML Host mejorado ------------------
static const char HOST_HTML[] PROGMEM = R"HTML(
<!doctype html><html lang="es"><head>
<meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1">
<title>ESP32 Quiz - Host</title>
<style>
:root {
  --primary: #FF6B6B;
  --secondary: #4ECDC4;
  --accent: #FFE66D;
  --dark: #2C3E50;
  --light: #F7FFF7;
  --correct: #2ECC71;
  --incorrect: #E74C3C;
}
* { box-sizing: border-box; }
body {
  font-family: 'Segoe UI', system-ui, -apple-system, sans-serif;
  margin: 0;
  padding: 20px;
  background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
  min-height: 100vh;
}
.card {
  max-width: 1000px;
  margin: auto;
  background: white;
  border-radius: 20px;
  padding: 24px;
  box-shadow: 0 20px 40px rgba(0,0,0,0.15);
}
.header {
  text-align: center;
  margin-bottom: 24px;
}
.header h1 {
  color: var(--dark);
  margin: 0;
  font-size: 2.5rem;
}
.header .subtitle {
  color: #666;
  font-size: 1.1rem;
}
.stats {
  display: grid;
  grid-template-columns: repeat(auto-fit, minmax(150px, 1fr));
  gap: 12px;
  margin-bottom: 24px;
}
.stat-box {
  background: linear-gradient(135deg, var(--primary), #FF8E8E);
  color: white;
  padding: 16px;
  border-radius: 12px;
  text-align: center;
  box-shadow: 0 4px 12px rgba(255,107,107,0.3);
}
.stat-box:nth-child(2) { background: linear-gradient(135deg, var(--secondary), #6ED5CD); box-shadow: 0 4px 12px rgba(78,205,196,0.3); }
.stat-box:nth-child(3) { background: linear-gradient(135deg, var(--accent), #FFF0A0); color: var(--dark); box-shadow: 0 4px 12px rgba(255,230,109,0.3); }
.stat-box:nth-child(4) { background: linear-gradient(135deg, #9B59B6, #C39BD3); box-shadow: 0 4px 12px rgba(155,89,182,0.3); }
.stat-box h3 { margin: 0; font-size: 0.9rem; opacity: 0.9; }
.stat-box p { margin: 8px 0 0 0; font-size: 1.5rem; font-weight: bold; }
.question-area {
  background: var(--light);
  border-radius: 16px;
  padding: 20px;
  margin-bottom: 20px;
  border-left: 6px solid var(--primary);
}
.question-area h2 {
  margin: 0 0 16px 0;
  color: var(--dark);
  font-size: 1.3rem;
}
.question-text {
  font-size: 1.4rem;
  font-weight: 600;
  color: var(--dark);
  margin-bottom: 16px;
  line-height: 1.4;
}
.options {
  display: grid;
  grid-template-columns: repeat(auto-fit, minmax(200px, 1fr));
  gap: 12px;
}
.option {
  background: white;
  border: 2px solid #e0e0e0;
  border-radius: 12px;
  padding: 14px 18px;
  font-size: 1.1rem;
  cursor: pointer;
  transition: all 0.2s;
  text-align: left;
}
.option:hover { border-color: var(--primary); transform: translateY(-2px); }
.option.correct { background: var(--correct); color: white; border-color: var(--correct); }
.option.incorrect { background: var(--incorrect); color: white; border-color: var(--incorrect); }
.controls {
  display: flex;
  gap: 12px;
  flex-wrap: wrap;
  margin-bottom: 24px;
}
.btn {
  padding: 14px 24px;
  border: none;
  border-radius: 12px;
  font-size: 1rem;
  font-weight: 600;
  cursor: pointer;
  transition: all 0.2s;
  color: white;
}
.btn:hover { transform: translateY(-2px); box-shadow: 0 6px 16px rgba(0,0,0,0.2); }
.btn:active { transform: translateY(0); }
.btn-primary { background: linear-gradient(135deg, var(--primary), #FF8E8E); }
.btn-secondary { background: linear-gradient(135deg, var(--secondary), #6ED5CD); }
.btn-accent { background: linear-gradient(135deg, var(--accent), #FFF0A0); color: var(--dark); }
.btn-purple { background: linear-gradient(135deg, #9B59B6, #C39BD3); }
.leaderboard {
  background: white;
  border-radius: 16px;
  overflow: hidden;
  box-shadow: 0 4px 12px rgba(0,0,0,0.1);
}
.leaderboard h3 {
  margin: 0;
  padding: 16px 20px;
  background: var(--dark);
  color: white;
  font-size: 1.2rem;
}
.leaderboard-content {
  padding: 0;
  max-height: 300px;
  overflow-y: auto;
}
.player-row {
  display: flex;
  align-items: center;
  padding: 12px 20px;
  border-bottom: 1px solid #f0f0f0;
  transition: background 0.2s;
}
.player-row:hover { background: #f8f9fa; }
.player-rank {
  font-size: 1.2rem;
  font-weight: bold;
  width: 40px;
  color: var(--primary);
}
.player-icon {
  font-size: 1.5rem;
  margin-right: 12px;
}
.player-name {
  flex: 1;
  font-weight: 500;
}
.player-score {
  font-weight: bold;
  color: var(--primary);
  font-size: 1.1rem;
}
.progress-bar {
  width: 100%;
  height: 8px;
  background: #e0e0e0;
  border-radius: 4px;
  overflow: hidden;
  margin-top: 8px;
}
.progress-fill {
  height: 100%;
  background: linear-gradient(90deg, var(--primary), var(--secondary));
  border-radius: 4px;
  transition: width 0.3s ease;
}
.empty-state {
  text-align: center;
  padding: 40px;
  color: #999;
}
.timer-large {
  font-size: 3rem;
  font-weight: bold;
  color: var(--primary);
  text-align: center;
  margin: 20px 0;
}
.phase-indicator {
  display: inline-block;
  padding: 6px 14px;
  border-radius: 20px;
  font-size: 0.9rem;
  font-weight: 600;
  text-transform: uppercase;
}
.phase-lobby { background: #E8F5E9; color: #2E7D32; }
.phase-joining { background: #FFF3E0; color: #F57C00; }
.phase-question { background: #E3F2FD; color: #1976D2; }
.phase-reveal { background: #F3E5F5; color: #7B1FA2; }
.phase-leaderboard { background: #FCE4EC; color: #C2185B; }
</style></head><body>
<div class="card">
  <div class="header">
    <h1>🧪 Quiz del Átomo</h1>
    <p class="subtitle">Panel de Control - Host</p>
  </div>

  <div class="stats">
    <div class="stat-box">
      <h3>PIN de Sala</h3>
      <p id="pin">1234</p>
    </div>
    <div class="stat-box">
      <h3>Jugadores</h3>
      <p id="pc">0</p>
    </div>
    <div class="stat-box">
      <h3>Fase</h3>
      <p id="ph">LOBBY</p>
    </div>
    <div class="stat-box">
      <h3>Tiempo</h3>
      <p id="tl">0s</p>
    </div>
  </div>

  <div class="question-area">
    <h2>Pregunta <span id="q-num">1</span> de <span id="q-total">15</span></h2>
    <div class="question-text" id="q-text">Presiona "Siguiente Pregunta" para comenzar</div>
    <div class="options" id="opts"></div>
    <div class="progress-bar">
      <div class="progress-fill" id="progress" style="width: 0%"></div>
    </div>
  </div>

  <div class="controls">
    <button class="btn btn-primary" onclick="host('next')">🔄 Siguiente Pregunta</button>
    <button class="btn btn-secondary" onclick="host('start')">▶️ Iniciar Ronda</button>
    <button class="btn btn-accent" onclick="host('reveal')">👁️ Revelar Respuestas</button>
    <button class="btn btn-purple" onclick="host('reset')">🔄 Resetear Todo</button>
  </div>

  <div class="leaderboard">
    <h3>🏆 Tabla de Clasificación</h3>
    <div class="leaderboard-content" id="lb">
      <div class="empty-state">Esperando jugadores...</div>
    </div>
  </div>
</div>

<script>
const pinEl=document.getElementById('pin'), pcEl=document.getElementById('pc');
const phEl=document.getElementById('ph'), tlEl=document.getElementById('tl');
const qNumEl=document.getElementById('q-num'), qTotalEl=document.getElementById('q-total');
const qTextEl=document.getElementById('q-text'), optsEl=document.getElementById('opts');
const lbEl=document.getElementById('lb'), progressEl=document.getElementById('progress');

async function host(cmd){
  await fetch('/api/host/'+cmd, {cache:'no-store'});
  await tick();
}

function phaseName(p){
  const names = ['LOBBY','UNIENDOSE','PREGUNTA','REVELAR','CLASIFICACION'];
  return names[p] || p;
}

function phaseClass(p){
  const classes = ['phase-lobby','phase-joining','phase-question','phase-reveal','phase-leaderboard'];
  return classes[p] || '';
}

async function tick(){
  const r=await fetch('/api/state', {cache:'no-store'});
  const s=await r.json();

  pinEl.textContent = s.pin;
  pcEl.textContent = s.players + (s.players_answered ? ` (${s.players_answered} respondieron)` : '');
  phEl.textContent = phaseName(s.phase);
  phEl.className = 'phase-indicator ' + phaseClass(s.phase);
  tlEl.textContent = Math.ceil(s.time_left_ms/1000) + 's';

  qNumEl.textContent = s.q_index + 1;
  qTotalEl.textContent = s.q_total;
  qTextEl.textContent = s.q_visible ? s.q_text : '⏳ Esperando a que inicie la ronda...';

  optsEl.innerHTML = '';
  if (s.q_visible) {
    for(let i=0;i<4;i++){
      const d=document.createElement('div');
      d.className = 'option';
      if (s.phase === 3 && i === s.correct) d.classList.add('correct');
      const mark = (s.phase === 3 && i === s.correct) ? ' ✅' : '';
      d.textContent = String.fromCharCode(65+i) + ') ' + s.q_opts[i] + mark;
      optsEl.appendChild(d);
    }
  }

  const answered = s.players_answered || 0;
  const total = s.players || 1;
  const progress = (answered / total) * 100;
  progressEl.style.width = progress + '%';

  lbEl.innerHTML = '';
  if (s.leaderboard && s.leaderboard.length > 0) {
    s.leaderboard.forEach((p, idx) => {
      const row = document.createElement('div');
      row.className = 'player-row';
      row.innerHTML = `
        <div class="player-rank">#${idx + 1}</div>
        <div class="player-icon">${p.icon}</div>
        <div class="player-name">${p.name}</div>
        <div class="player-score">${p.score} pts</div>
      `;
      lbEl.appendChild(row);
    });
  } else {
    lbEl.innerHTML = '<div class="empty-state">Esperando jugadores...</div>';
  }
}

setInterval(tick, 500);
tick();
</script>
</body></html>
)HTML";

// ------------------ HTML Player mejorado ------------------
static const char PLAY_HTML[] PROGMEM = R"HTML(
<!doctype html><html lang="es"><head>
<meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1">
<title>ESP32 Quiz - Jugador</title>
<style>
:root {
  --primary: #FF6B6B;
  --secondary: #4ECDC4;
  --accent: #FFE66D;
  --dark: #2C3E50;
  --light: #F7FFF7;
  --correct: #2ECC71;
  --incorrect: #E74C3C;
  --waiting: #F39C12;
}
* { box-sizing: border-box; }
body {
  font-family: 'Segoe UI', system-ui, -apple-system, sans-serif;
  margin: 0;
  padding: 20px;
  background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
  min-height: 100vh;
  display: flex;
  align-items: center;
  justify-content: center;
}
.card {
  max-width: 500px;
  width: 100%;
  background: white;
  border-radius: 24px;
  padding: 32px;
  box-shadow: 0 20px 40px rgba(0,0,0,0.15);
  text-align: center;
}
.logo {
  font-size: 4rem;
  margin-bottom: 16px;
}
.title {
  font-size: 2rem;
  color: var(--dark);
  margin: 0 0 8px 0;
  font-weight: 700;
}
.subtitle {
  color: #666;
  margin-bottom: 24px;
  font-size: 1.1rem;
}
.input-group {
  margin-bottom: 16px;
}
input {
  width: 100%;
  padding: 16px;
  border: 2px solid #e0e0e0;
  border-radius: 12px;
  font-size: 1.1rem;
  transition: border-color 0.2s;
}
input:focus {
  outline: none;
  border-color: var(--primary);
}
.btn {
  width: 100%;
  padding: 16px;
  border: none;
  border-radius: 12px;
  font-size: 1.1rem;
  font-weight: 600;
  cursor: pointer;
  transition: all 0.2s;
  color: white;
  background: linear-gradient(135deg, var(--primary), #FF8E8E);
}
.btn:hover { transform: translateY(-2px); box-shadow: 0 6px 16px rgba(255,107,107,0.3); }
.btn:disabled { opacity: 0.6; cursor: not-allowed; transform: none; }
.btn-secondary { background: linear-gradient(135deg, var(--secondary), #6ED5CD); }
.btn-accent { background: linear-gradient(135deg, var(--accent), #FFF0A0); color: var(--dark); }
.message {
  padding: 12px;
  border-radius: 8px;
  margin-top: 16px;
  font-weight: 500;
}
.message.error { background: #FADBD8; color: var(--incorrect); }
.message.success { background: #D5F4E6; color: var(--correct); }
.message.info { background: #E8F4FD; color: #1976D2; }
.stats {
  display: grid;
  grid-template-columns: repeat(3, 1fr);
  gap: 12px;
  margin-bottom: 24px;
}
.stat {
  background: #f8f9fa;
  padding: 12px;
  border-radius: 12px;
}
.stat-value {
  font-size: 1.5rem;
  font-weight: bold;
  color: var(--primary);
}
.stat-label {
  font-size: 0.9rem;
  color: #666;
}
.question-area {
  background: linear-gradient(135deg, #f8f9fa, #e9ecef);
  border-radius: 16px;
  padding: 24px;
  margin-bottom: 24px;
}
.question-number {
  font-size: 1rem;
  color: #666;
  margin-bottom: 8px;
}
.question-text {
  font-size: 1.3rem;
  font-weight: 600;
  color: var(--dark);
  line-height: 1.4;
  margin-bottom: 20px;
}
.answer-grid {
  display: grid;
  grid-template-columns: 1fr;
  gap: 12px;
  margin-bottom: 20px;
}
.answer-btn {
  padding: 16px 20px;
  border: 2px solid #e0e0e0;
  border-radius: 12px;
  background: white;
  font-size: 1.1rem;
  cursor: pointer;
  transition: all 0.2s;
  text-align: left;
}
.answer-btn:hover:not(:disabled) { border-color: var(--primary); transform: translateY(-2px); }
.answer-btn:disabled { cursor: not-allowed; opacity: 0.7; }
.answer-btn.correct { background: var(--correct); color: white; border-color: var(--correct); }
.answer-btn.incorrect { background: var(--incorrect); color: white; border-color: var(--incorrect); }
.answer-btn.selected { background: var(--primary); color: white; border-color: var(--primary); }
.status {
  padding: 16px;
  border-radius: 12px;
  font-weight: 600;
  font-size: 1.1rem;
  margin-bottom: 16px;
}
.status.waiting { background: #FFF3E0; color: var(--waiting); }
.status.correct { background: #D5F4E6; color: var(--correct); }
.status.incorrect { background: #FADBD8; color: var(--incorrect); }
.status.answered { background: #E8F4FD; color: #1976D2; }
.timer-circle {
  width: 80px;
  height: 80px;
  border-radius: 50%;
  background: conic-gradient(var(--primary) 0deg, #e0e0e0 0deg);
  display: flex;
  align-items: center;
  justify-content: center;
  margin: 0 auto 20px;
  position: relative;
}
.timer-circle::before {
  content: '';
  width: 60px;
  height: 60px;
  border-radius: 50%;
  background: white;
  position: absolute;
}
.timer-text {
  font-size: 1.5rem;
  font-weight: bold;
  color: var(--dark);
  position: relative;
  z-index: 1;
}
.join-info {
  background: #f8f9fa;
  border-radius: 12px;
  padding: 16px;
  margin-bottom: 20px;
}
.join-info h3 {
  margin: 0 0 8px 0;
  color: var(--dark);
}
.join-info p {
  margin: 0;
  color: #666;
}
</style></head><body>
<div class="card">
  <!-- Pantalla de unión -->
  <div id="joinScreen">
    <div class="logo">🧪</div>
    <h1 class="title">Quiz del Átomo</h1>
    <p class="subtitle">¡Únete y demuestra tu conocimiento!</p>
    
    <div class="input-group">
      <input id="pin" placeholder="PIN de la sala (ej: 1234)" maxlength="8">
    </div>
    <div class="input-group">
      <input id="name" placeholder="Tu nombre" maxlength="16">
    </div>
    <button class="btn" onclick="join()">🚀 Unirse al Juego</button>
    
    <div id="joinMessage"></div>
  </div>

  <!-- Pantalla de juego -->
  <div id="gameScreen" style="display:none;">
    <div class="stats">
      <div class="stat">
        <div class="stat-value" id="playerIcon">⚛️</div>
        <div class="stat-label" id="playerName">Tú</div>
      </div>
      <div class="stat">
        <div class="stat-value" id="score">0</div>
        <div class="stat-label">Puntos</div>
      </div>
      <div class="stat">
        <div class="stat-value" id="streak">0</div>
        <div class="stat-label">Racha</div>
      </div>
    </div>

    <div class="question-area">
      <div class="question-number" id="questionNum">Pregunta 1 de 15</div>
      <div class="question-text" id="questionText">Esperando a que inicie el juego...</div>
      
      <div id="timerContainer" style="display:none;">
        <div class="timer-circle" id="timerCircle">
          <div class="timer-text" id="timerText">20</div>
        </div>
      </div>
      
      <div class="answer-grid" id="answerGrid"></div>
    </div>

    <div id="statusArea"></div>
  </div>
</div>

<script>
let playerId = 0;
let playerData = null;
let selectedAnswer = -1;

const joinScreen = document.getElementById('joinScreen');
const gameScreen = document.getElementById('gameScreen');
const joinMessage = document.getElementById('joinMessage');

// Elementos del juego
const playerIcon = document.getElementById('playerIcon');
const playerName = document.getElementById('playerName');
const scoreEl = document.getElementById('score');
const streakEl = document.getElementById('streak');
const questionNum = document.getElementById('questionNum');
const questionText = document.getElementById('questionText');
const answerGrid = document.getElementById('answerGrid');
const statusArea = document.getElementById('statusArea');
const timerContainer = document.getElementById('timerContainer');
const timerText = document.getElementById('timerText');
const timerCircle = document.getElementById('timerCircle');

function showMessage(text, type = 'info') {
  joinMessage.innerHTML = `<div class="message ${type}">${text}</div>`;
}

async function join() {
  const pin = document.getElementById('pin').value.trim();
  const name = document.getElementById('name').value.trim();
  
  if (!pin || !name) {
    showMessage('Por favor ingresa el PIN y tu nombre', 'error');
    return;
  }
  
  try {
    const r = await fetch(`/api/join?pin=${encodeURIComponent(pin)}&name=${encodeURIComponent(name)}`);
    const data = await r.json();
    
    if (data.ok) {
      playerId = data.pid;
      playerData = data;
      
      playerIcon.textContent = data.icon;
      playerName.textContent = data.name;
      
      joinScreen.style.display = 'none';
      gameScreen.style.display = 'block';
      
      startGameLoop();
    } else {
      showMessage(data.err || 'Error al unirse', 'error');
    }
  } catch (e) {
    showMessage('Error de conexión', 'error');
  }
}

function phaseName(p) {
  const names = ['ESPERANDO', 'UNIÉNDOSE', 'PREGUNTA', 'REVELANDO', 'CLASIFICACIÓN'];
  return names[p] || p;
}

function updateTimer(seconds) {
  const total = 20;
  const progress = ((total - seconds) / total) * 360;
  timerCircle.style.background = `conic-gradient(var(--primary) ${progress}deg, #e0e0e0 ${progress}deg)`;
  timerText.textContent = seconds;
}

function createAnswerButtons(options, phase, answered, correct) {
  answerGrid.innerHTML = '';
  
  options.forEach((opt, i) => {
    const btn = document.createElement('button');
    btn.className = 'answer-btn';
    btn.textContent = String.fromCharCode(65 + i) + ') ' + opt;
    
    if (phase === 3) { // Revelando
      if (i === correct) btn.classList.add('correct');
      if (answered && i === selectedAnswer && i !== correct) btn.classList.add('incorrect');
      btn.disabled = true;
    } else if (phase === 2 && !answered) { // Pregunta activa
      btn.onclick = () => selectAnswer(i);
    } else if (answered && i === selectedAnswer) {
      btn.classList.add('selected');
      btn.disabled = true;
    } else {
      btn.disabled = true;
    }
    
    answerGrid.appendChild(btn);
  });
}

function selectAnswer(option) {
  selectedAnswer = option;
  fetch(`/api/answer?pid=${playerId}&opt=${option}`);
  
  // Actualizar UI
  const buttons = answerGrid.querySelectorAll('.answer-btn');
  buttons.forEach((btn, i) => {
    btn.disabled = true;
    if (i === option) btn.classList.add('selected');
  });
  
  statusArea.innerHTML = '<div class="status answered">✅ Respuesta enviada</div>';
}

function updateStatus(phase, answered, correct, timeLeft) {
  if (phase === 0) { // Lobby
    statusArea.innerHTML = '<div class="status waiting">⏳ Esperando a que el host inicie el juego...</div>';
    timerContainer.style.display = 'none';
  } else if (phase === 1) { // Uniéndose
    statusArea.innerHTML = '<div class="status waiting">🎯 ¡Prepárate! La ronda comienza pronto...</div>';
    timerContainer.style.display = 'none';
  } else if (phase === 2) { // Pregunta
    if (answered) {
      statusArea.innerHTML = '<div class="status answered">✅ Respuesta enviada, esperando a los demás...</div>';
    } else {
      statusArea.innerHTML = '<div class="status waiting">🤔 Selecciona tu respuesta antes de que se acabe el tiempo</div>';
    }
    timerContainer.style.display = 'block';
    updateTimer(Math.ceil(timeLeft / 1000));
  } else if (phase === 3) { // Revelar
    if (answered) {
      if (correct) {
        statusArea.innerHTML = '<div class="status correct">🎉 ¡Correcto! ¡Bien hecho!</div>';
      } else {
        statusArea.innerHTML = '<div class="status incorrect">❌ Incorrecto, pero ¡sigue intentando!</div>';
      }
    } else {
      statusArea.innerHTML = '<div class="status incorrect">⏰ Se acabó el tiempo</div>';
    }
    timerContainer.style.display = 'none';
  }
}

async function startGameLoop() {
  while (true) {
    try {
      const r = await fetch(`/api/state?pid=${playerId}`);
      const s = await r.json();
      
      // Actualizar stats
      scoreEl.textContent = s.me_score || 0;
      streakEl.textContent = s.me_streak || 0;
      
      // Actualizar pregunta
      questionNum.textContent = `Pregunta ${s.q_index + 1} de ${s.q_total}`;
      questionText.textContent = s.q_visible ? s.q_text : '⏳ Esperando la siguiente pregunta...';
      
      // Actualizar botones de respuesta
      if (s.q_visible) {
        createAnswerButtons(s.q_opts, s.phase, s.me_answered, s.correct);
      } else {
        answerGrid.innerHTML = '';
      }
      
      // Actualizar estado
      updateStatus(s.phase, s.me_answered, s.me_correct, s.time_left_ms);
      
    } catch (e) {
      console.error('Error:', e);
    }
    
    await new Promise(resolve => setTimeout(resolve, 500));
  }
}
</script>
</body></html>
)HTML";

// ------------------ Estado y API ------------------
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

  players[slot].used = true;
  players[slot].id = nextPlayerId++;
  players[slot].name = name.substring(0,16);
  players[slot].icon = players[slot].id % NICON;
  players[slot].score = 0;
  players[slot].streak = 0;
  players[slot].answered = false;
  players[slot].answer = -1;
  players[slot].correct = false;
  players[slot].answerTime = 0;
  players[slot].joinedThisRound = true;

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
  p->answerTime = millis();

  // Puntuación con sistema de racha
  uint32_t tl = timeLeftMs();
  uint32_t gained = 0;
  
  if (p->correct) {
    p->streak++;
    gained = 100 + (tl * 400UL) / QUESTION_TIME_MS; // 100..500 base
    gained += p->streak * 50; // Bonus por racha
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
           String(ICONS[players[best].icon]) + "\",\"score\":" + String(players[best].score) + ",\"streak\":" + String(players[best].streak) + "}";
  }
  out += "]";
  return out;
}

void apiState() {
  autoAdvance();

  uint16_t pid = server.hasArg("pid") ? (uint16_t)server.arg("pid").toInt() : 0;
  Player* me = pid ? findPlayer(pid) : nullptr;

  uint32_t tl = timeLeftMs();
  // Las preguntas deben ser visibles en fase QUESTION y REVEAL siempre
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
  body += "\"q_opts\":["
       "\"" + String(QUESTIONS[currentQ].a[0]) + "\","
       "\"" + String(QUESTIONS[currentQ].a[1]) + "\","
       "\"" + String(QUESTIONS[currentQ].a[2]) + "\","
       "\"" + String(QUESTIONS[currentQ].a[3]) + "\"],";
  body += "\"correct\":" + String(QUESTIONS[currentQ].correct) + ",";
  body += "\"leaderboard\":" + leaderboardJson(10);

  if (me) {
    String st;
    if (phase == LOBBY) st = "Esperando a que el host inicie...";
    else if (phase == JOINING) st = "¡Prepárate! La ronda comienza pronto...";
    else if (phase == QUESTION) st = me->answered ? "Respuesta enviada ✓" : "Responde antes de que se acabe el tiempo!";
    else if (phase == REVEAL) st = me->correct ? "¡Correcto! 🎉" : (me->answered ? "Incorrecto 😅" : "Se acabó el tiempo ⏰");

    body += ",\"me_score\":" + String(me->score);
    body += ",\"me_streak\":" + String(me->streak);
    body += ",\"me_answered\":" + String(me->answered ? "true":"false");
    body += ",\"me_correct\":" + String(me->correct ? "true":"false");
    body += ",\"me_status\":\"" + jsonEscape(st) + "\"";
  }

  body += "}";
  sendJson(body);
}

void hostNext() {
  currentQ = (currentQ + 1) % NQ;
  phase = QUESTION;
  questionStartMs = millis();
  phaseStartMs = millis();
  questionVisible = true;
  resetForNewQuestion();
  sendJson("{\"ok\":true}");
}

void hostStart() {
  phase = QUESTION;
  questionStartMs = millis();
  phaseStartMs = millis();
  questionVisible = true;
  resetForNewQuestion();
  sendJson("{\"ok\":true}");
}

void hostReveal() {
  phase = REVEAL;
  phaseStartMs = millis();
  questionVisible = true;
  sendJson("{\"ok\":true}");
}

void hostReset() {
  for (auto &p : players) p = Player();
  nextPlayerId = 1;
  currentQ = 0;
  phase = LOBBY;
  resetForNewQuestion();
  resetForNewRound();
  sendJson("{\"ok\":true}");
}

// ------------------ Rutas ------------------
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

  Serial.println("=================================");
  Serial.println("  🧪 ESP32 Quiz Mejorado Iniciado");
  Serial.println("=================================");
  Serial.print("IP AP: ");
  Serial.println(WiFi.softAPIP());
  Serial.println("Host:  http://192.168.4.1/host");
  Serial.println("Play:  http://192.168.4.1/play");
  Serial.print("PIN:   ");
  Serial.println(ROOM_PIN);
  Serial.println("=================================");
}

void loop() {
  server.handleClient();
}