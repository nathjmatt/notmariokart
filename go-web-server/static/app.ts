interface WiiState {
  pitch: number;
  yaw: number;
  roll: number;
  a?: boolean;
  b?: boolean;
  '1'?: boolean;
  '2'?: boolean;
}

const canvas = document.getElementById('c') as HTMLCanvasElement;
const connectionStatus = document.getElementById('connectionStatus') as HTMLDivElement;
const info = document.getElementById('info') as HTMLDivElement;
const ui = document.getElementById('ui') as HTMLDivElement | null;
const btnA = document.getElementById('btnA') as HTMLDivElement;
const btnB = document.getElementById('btnB') as HTMLDivElement;
const btn1 = document.getElementById('btn1') as HTMLDivElement;
const btn2 = document.getElementById('btn2') as HTMLDivElement;
const ctx = canvas.getContext('2d') as CanvasRenderingContext2D;

function resize(): void {
  canvas.width = innerWidth;
  canvas.height = innerHeight;
}

addEventListener('resize', resize);
resize();

let state: WiiState = {
  pitch: 0,
  yaw: 0,
  roll: 0,
  a: false,
  b: false,
  '1': false,
  '2': false,
};

// clamp returns v limited to the range [min, max]
function clamp(v: number, min: number, max: number): number {
  return Math.max(min, Math.min(max, v));
}

function draw(): void {
  ctx.clearRect(0, 0, canvas.width, canvas.height);
  const cx = canvas.width / 2;
  const cy = canvas.height / 2;

  // Compute a centered containment rectangle (the dot will be constrained here)
  const edgePad = Math.min(96, Math.floor(Math.min(canvas.width, canvas.height) * 0.08));
  const rectW = Math.max(120, canvas.width - edgePad * 2);
  const rectH = Math.max(120, canvas.height - edgePad * 2);
  let rectX = (canvas.width - rectW) / 2;
  let rectY = (canvas.height - rectH) / 2;

  // If the UI occupies part of the screen, shift the rectangle so it doesn't overlap
  if (ui) {
    const uiRect = ui.getBoundingClientRect();
    const gap = 8; // pixels of breathing room between UI and rectangle
    const u = { left: uiRect.left - gap, top: uiRect.top - gap, right: uiRect.right + gap, bottom: uiRect.bottom + gap };
    const rect = { left: rectX, top: rectY, right: rectX + rectW, bottom: rectY + rectH };
    const intersects = !(rect.left >= u.right || rect.right <= u.left || rect.top >= u.bottom || rect.bottom <= u.top);
    if (intersects) {
      // compute overlaps
      const overlapX = Math.min(rect.right, u.right) - Math.max(rect.left, u.left);
      const overlapY = Math.min(rect.bottom, u.bottom) - Math.max(rect.top, u.top);
      // Prefer moving in the smaller-overlap direction
      if (overlapY < overlapX) {
        // move vertically away from UI
        if (rectY >= u.top) rectY = u.bottom + 4; else rectY = u.top - rectH - 4;
      } else {
        // move horizontally away from UI
        if (rectX >= u.left) rectX = u.right + 4; else rectX = u.left - rectW - 4;
      }
      // clamp into canvas
      rectX = clamp(rectX, 0, canvas.width - rectW);
      rectY = clamp(rectY, 0, canvas.height - rectH);
    }
  }

  // roll to rotation in radians, and size scale
  const rot = ((clamp(state.roll ?? 0, -180, 180) * Math.PI) / 180) || 0;
  const baseR = 30;
  const r = baseR + (Math.abs(clamp(state.roll ?? 0, -180, 180)) / 180) * 40;

  // Draw the containment rectangle so users can see the allowed area
  ctx.save();
  ctx.strokeStyle = 'rgba(150,200,255,0.14)';
  ctx.lineWidth = Math.max(2, Math.round(Math.min(6, canvas.width * 0.002)));
  ctx.setLineDash([6, 8]);
  ctx.strokeRect(rectX, rectY, rectW, rectH);
  ctx.setLineDash([]);
  ctx.restore();

  // Map yaw/pitch (-180..180) to normalized 0..1 inside the rectangle
  const nx = (clamp(state.yaw ?? 0, -180, 180) + 180) / 360;
  const ny = (clamp(state.pitch ?? 0, -180, 180) + 180) / 360;

  // Position inside rectangle and clamp so the dot stays fully inside
  let x = rectX + nx * rectW;
  let y = rectY + ny * rectH;
  x = clamp(x, rectX + r, rectX + rectW - r);
  y = clamp(y, rectY + r, rectY + rectH - r);

  ctx.save();
  ctx.translate(x, y);
  ctx.rotate(rot);
  ctx.beginPath(); ctx.fillStyle = 'red'; ctx.arc(0, 0, r, 0, Math.PI * 2); ctx.fill();
  ctx.restore();

  info.textContent = 'pitch:' + (state.pitch ?? 0).toFixed(2) +
    ' yaw:' + (state.yaw ?? 0).toFixed(2) +
    ' roll:' + (state.roll ?? 0).toFixed(2) +
    ' a:' + !!state.a + ' b:' + !!state.b + ' 1:' + !!state['1'] + ' 2:' + !!state['2'];

  // update buttons
  btnA.classList.toggle('on', !!state.a);
  btnB.classList.toggle('on', !!state.b);
  btn1.classList.toggle('on', !!state['1']);
  btn2.classList.toggle('on', !!state['2']);

  requestAnimationFrame(draw);
}
requestAnimationFrame(draw);

// WebSocket display connection first
let ws: WebSocket | undefined;

function connectWS(): void {
  ws = new WebSocket((location.protocol === 'https:' ? 'wss://' : 'ws://') + location.host + '/ws');

  ws.onopen = () => {
    connectionStatus.textContent = 'Connected via WebSocket';
    console.debug('WebSocket connection established');
  };

  ws.onmessage = (ev: MessageEvent) => {
    try {
      const s = JSON.parse(ev.data) as WiiState;
      state = s;
    } catch (e) {
      console.error('Error parsing WebSocket message:', e);
    }
  };

  ws.onclose = () => {
    connectionStatus.textContent = 'WebSocket closed, falling back to polling';
    console.debug('WebSocket connection closed, switching to polling.');
    setTimeout(connectPoll, 500);
  };

  ws.onerror = () => {
    ws?.close();
    console.error("WebSocket error occurred, closing connection.");
    connectionStatus.textContent = 'WebSocket error, falling back to polling';
  };
}

// Polling fallback
let pollTimer: number | undefined;
function connectPoll(): void {
  if (pollTimer) clearInterval(pollTimer);
  fetch('/state').then(r => r.json()).then((s: WiiState) => { state = s; info.textContent = 'Polling /state'; }).catch(() => { });
  pollTimer = setInterval(() => { fetch('/state').then(r => r.json()).then((s: WiiState) => state = s).catch(() => { }); }, 100) as unknown as number;
}

try {
  connectWS();
} catch (e) {
  connectPoll();
}
