const canvas = document.getElementById('c');
const info = document.getElementById('info');
const btnA = document.getElementById('btnA');
const btnB = document.getElementById('btnB');
const btn1 = document.getElementById('btn1');
const btn2 = document.getElementById('btn2');
const ctx = canvas.getContext('2d');
function resize() {
    canvas.width = innerWidth;
    canvas.height = innerHeight;
}
addEventListener('resize', resize);
resize();
let state = {
    pitch: 0,
    yaw: 0,
    roll: 0,
    a: false,
    b: false,
    '1': false,
    '2': false,
};
// clamp returns v limited to the range [min, max]
function clamp(v, min, max) {
    return Math.max(min, Math.min(max, v));
}
function draw() {
    var _a, _b, _c, _d, _e, _f, _g;
    ctx.clearRect(0, 0, canvas.width, canvas.height);
    const cx = canvas.width / 2;
    const cy = canvas.height / 2;
    const margin = 96;
    const maxX = cx - margin;
    const maxY = cy - margin;
    // Map yaw [-180,180] to [-maxX, maxX]
    const x = cx + (clamp((_a = state.yaw) !== null && _a !== void 0 ? _a : 0, -180, 180) / 180) * maxX;
    const y = cy + (clamp((_b = state.pitch) !== null && _b !== void 0 ? _b : 0, -180, 180) / 180) * maxY;
    // roll to rotation in radians, and size scale
    const rot = ((clamp((_c = state.roll) !== null && _c !== void 0 ? _c : 0, -180, 180) * Math.PI) / 180) || 0;
    const baseR = 30;
    const r = baseR + (Math.abs(clamp((_d = state.roll) !== null && _d !== void 0 ? _d : 0, -180, 180)) / 180) * 40;
    ctx.save();
    ctx.translate(x, y);
    ctx.rotate(rot);
    ctx.beginPath();
    ctx.fillStyle = 'red';
    ctx.arc(0, 0, r, 0, Math.PI * 2);
    ctx.fill();
    ctx.restore();
    info.textContent = 'pitch:' + ((_e = state.pitch) !== null && _e !== void 0 ? _e : 0).toFixed(2) +
        ' yaw:' + ((_f = state.yaw) !== null && _f !== void 0 ? _f : 0).toFixed(2) +
        ' roll:' + ((_g = state.roll) !== null && _g !== void 0 ? _g : 0).toFixed(2) +
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
let ws;
function connectWS() {
    ws = new WebSocket((location.protocol === 'https:' ? 'wss://' : 'ws://') + location.host + '/ws');
    ws.onopen = () => {
        info.textContent = 'Connected via WebSocket';
        console.debug('WebSocket connection established');
    };
    ws.onmessage = (ev) => {
        try {
            const s = JSON.parse(ev.data);
            state = s;
        }
        catch (e) {
            console.error('Error parsing WebSocket message:', e);
        }
    };
    ws.onclose = () => {
        info.textContent = 'WebSocket closed, falling back to polling';
        console.debug('WebSocket connection closed, switching to polling.');
        setTimeout(connectPoll, 500);
    };
    ws.onerror = () => {
        ws === null || ws === void 0 ? void 0 : ws.close();
        console.error("WebSocket error occurred, closing connection.");
    };
}
// Polling fallback
let pollTimer;
function connectPoll() {
    if (pollTimer)
        clearInterval(pollTimer);
    fetch('/state').then(r => r.json()).then((s) => { state = s; info.textContent = 'Polling /state'; }).catch(() => { });
    pollTimer = setInterval(() => { fetch('/state').then(r => r.json()).then((s) => state = s).catch(() => { }); }, 100);
}
try {
    connectWS();
}
catch (e) {
    connectPoll();
}
//# sourceMappingURL=app.js.map