#pragma once
#include <Arduino.h>

// HTML del Host (panel de control)
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

  if (s.phase === 4) {
    qNumEl.textContent = s.q_total;
    qTotalEl.textContent = s.q_total;
    qTextEl.textContent = '🏁 Fin de la partida — revisa los ganadores abajo';
    optsEl.innerHTML = '';
    progressEl.style.width = '100%';
  } else {
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
  }

  const answered = s.players_answered || 0;
  const total = s.players || 1;
  const progress = (answered / total) * 100;
  progressEl.style.width = progress + '%';
  if (s.phase === 4) progressEl.style.width = '100%';

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

// HTML del Jugador
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
.final-row {
  display: flex;
  align-items: center;
  justify-content: space-between;
  padding: 10px 12px;
  border-radius: 12px;
  background: #f8f9fa;
  margin: 8px 0;
  font-weight: 600;
}
.final-left {
  display: flex;
  align-items: center;
  gap: 10px;
}
.final-rank {
  width: 26px;
  text-align: center;
}
.final-score {
  color: var(--primary);
}
</style></head><body>
<div class="card">
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
let selectedAnswer = -1;

const joinScreen = document.getElementById('joinScreen');
const gameScreen = document.getElementById('gameScreen');
const joinMessage = document.getElementById('joinMessage');

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
  if (!pin || !name) { showMessage('Por favor ingresa el PIN y tu nombre', 'error'); return; }

  try {
    const r = await fetch(`/api/join?pin=${encodeURIComponent(pin)}&name=${encodeURIComponent(name)}`);
    const data = await r.json();
    if (data.ok) {
      playerId = data.pid;
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

    if (phase === 3) {
      if (i === correct) btn.classList.add('correct');
      if (answered && i === selectedAnswer && i !== correct) btn.classList.add('incorrect');
      btn.disabled = true;
    } else if (phase === 2 && !answered) {
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

function renderFinalLeaderboard(lb) {
  answerGrid.innerHTML = '';
  if (!lb || lb.length === 0) {
    answerGrid.innerHTML = '<div class="status waiting">Sin jugadores conectados</div>';
    return;
  }
  const medals = ['🥇','🥈','🥉'];
  lb.slice(0, 5).forEach((p, idx) => {
    const row = document.createElement('div');
    row.className = 'final-row';
    const medal = medals[idx] || '🏅';
    row.innerHTML = `
      <div class="final-left">
        <div class="final-rank">${medal}</div>
        <div>${p.icon} ${p.name}</div>
      </div>
      <div class="final-score">${p.score} pts</div>
    `;
    answerGrid.appendChild(row);
  });
}

function selectAnswer(option) {
  selectedAnswer = option;
  fetch(`/api/answer?pid=${playerId}&opt=${option}`);
  const buttons = answerGrid.querySelectorAll('.answer-btn');
  buttons.forEach((btn, i) => {
    btn.disabled = true;
    if (i === option) btn.classList.add('selected');
  });
  statusArea.innerHTML = '<div class="status answered">✅ Respuesta enviada</div>';
}

function updateStatus(phase, answered, correct, timeLeft) {
  if (phase === 0) {
    statusArea.innerHTML = '<div class="status waiting">⏳ Esperando a que el host inicie el juego...</div>';
    timerContainer.style.display = 'none';
  } else if (phase === 2) {
    statusArea.innerHTML = answered
      ? '<div class="status answered">✅ Respuesta enviada, esperando a los demás...</div>'
      : '<div class="status waiting">🤔 Selecciona tu respuesta antes de que se acabe el tiempo</div>';
    timerContainer.style.display = 'block';
    updateTimer(Math.ceil(timeLeft / 1000));
  } else if (phase === 3) {
    if (answered) {
      statusArea.innerHTML = correct
        ? '<div class="status correct">🎉 ¡Correcto! ¡Bien hecho!</div>'
        : '<div class="status incorrect">❌ Incorrecto, pero ¡sigue intentando!</div>';
    } else {
      statusArea.innerHTML = '<div class="status incorrect">⏰ Se acabó el tiempo</div>';
    }
    timerContainer.style.display = 'none';
  } else if (phase === 4) {
    statusArea.innerHTML = '<div class="status correct">🏁 Partida finalizada. ¡Mira la clasificación!</div>';
    timerContainer.style.display = 'none';
  } else {
    statusArea.innerHTML = '<div class="status waiting">🎯 Preparando...</div>';
    timerContainer.style.display = 'none';
  }
}

async function startGameLoop() {
  while (true) {
    try {
      const r = await fetch(`/api/state?pid=${playerId}`);
      const s = await r.json();

      scoreEl.textContent = s.me_score || 0;
      streakEl.textContent = s.me_streak || 0;

      if (s.phase === 4) {
        questionNum.textContent = '🏁 Final';
        questionText.textContent = '🏆 Clasificación final';
        renderFinalLeaderboard(s.leaderboard);
      } else {
        questionNum.textContent = `Pregunta ${s.q_index + 1} de ${s.q_total}`;
        questionText.textContent = s.q_visible ? s.q_text : '⏳ Esperando la siguiente pregunta...';
        if (s.q_visible) createAnswerButtons(s.q_opts, s.phase, s.me_answered, s.correct);
        else answerGrid.innerHTML = '';
      }

      updateStatus(s.phase, s.me_answered, s.me_correct, s.time_left_ms);
    } catch (e) {}
    await new Promise(resolve => setTimeout(resolve, 500));
  }
}
</script>
</body></html>
)HTML";
