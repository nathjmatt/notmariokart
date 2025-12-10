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
const info = document.getElementById('info') as HTMLDivElement;
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
  const margin = 96;
  const maxX = cx - margin;
  const maxY = cy - margin;

  // Map yaw [-180,180] to [-maxX, maxX]
  const x = cx + (clamp(state.yaw ?? 0, -180, 180) / 180) * maxX;
  const y = cy + (clamp(state.pitch ?? 0, -180, 180) / 180) * maxY;

  // roll to rotation in radians, and size scale
  const rot = ((clamp(state.roll ?? 0, -180, 180) * Math.PI) / 180) || 0;
  const baseR = 30;
  const r = baseR + (Math.abs(clamp(state.roll ?? 0, -180, 180)) / 180) * 40;

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
        info.textContent = 'Connected via WebSocket';
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
        info.textContent = 'WebSocket closed, falling back to polling';
        console.debug('WebSocket connection closed, switching to polling.');
        setTimeout(connectPoll, 500);
    };

  ws.onerror = () => { 
        ws?.close(); 
        console.error("WebSocket error occurred, closing connection.");
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
