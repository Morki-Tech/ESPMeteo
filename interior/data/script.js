// ==========================================
// Estació Meteorològica — Dashboard JS
// Polling + Chart.js gràfics en temps real
// ==========================================

const UPDATE_INTERVAL = 10000; // 10s polling
let currentHours = 6;
let charts = {};

// --- Elements DOM ---
const els = {
    clock:     document.getElementById('clock'),
    statusDot: document.getElementById('status-dot'),
    tempIn:    document.getElementById('temp-in'),
    tempOut:   document.getElementById('temp-out'),
    humIn:     document.getElementById('hum-in'),
    humOut:    document.getElementById('hum-out'),
    eco2:      document.getElementById('eco2'),
    tvoc:      document.getElementById('tvoc'),
    wind:      document.getElementById('wind'),
    rain:      document.getElementById('rain'),
    battery:   document.getElementById('battery'),
    co2Bar:    document.getElementById('co2-bar'),
    uptime:    document.getElementById('uptime'),
    heap:      document.getElementById('heap'),
};

// --- Utilitats ---
function updateValue(el, newVal) {
    if (el.textContent !== String(newVal)) {
        el.textContent = newVal;
        el.classList.remove('value-updated');
        void el.offsetWidth; // force reflow
        el.classList.add('value-updated');
    }
}

function getCO2Color(ppm) {
    if (ppm < 600) return '#69f0ae';
    if (ppm < 1000) return '#ffd740';
    if (ppm < 1500) return '#ff9100';
    return '#ff5252';
}

function formatUptime(seconds) {
    const h = Math.floor(seconds / 3600);
    const m = Math.floor((seconds % 3600) / 60);
    return `${h}h ${m}m`;
}

// --- Fetch dades actuals ---
async function fetchCurrent() {
    try {
        const res = await fetch('/api/current');
        const d = await res.json();

        // Actualitzar cards
        updateValue(els.tempIn, d.indoor.temp.toFixed(1));
        updateValue(els.tempOut, d.outdoor.temp.toFixed(1));
        updateValue(els.humIn, d.indoor.hum.toFixed(0));
        updateValue(els.humOut, d.outdoor.hum.toFixed(0));
        updateValue(els.eco2, d.outdoor.eco2);
        updateValue(els.tvoc, d.outdoor.tvoc);
        updateValue(els.wind, d.outdoor.wind.toFixed(1));
        updateValue(els.rain, d.outdoor.rain.toFixed(2));
        updateValue(els.battery, d.outdoor.battery);
        els.clock.textContent = d.time;

        // CO2 bar
        const co2Pct = Math.min((d.outdoor.eco2 / 3000) * 100, 100);
        els.co2Bar.style.width = co2Pct + '%';
        els.co2Bar.style.background = getCO2Color(d.outdoor.eco2);

        // Stale cards
        const outCards = ['card-temp-out','card-hum-out','card-co2','card-tvoc','card-wind','card-rain'];
        outCards.forEach(id => {
            document.getElementById(id).classList.toggle('stale', d.outdoor.stale);
        });

        // Status
        els.statusDot.className = 'status-dot online';
    } catch (e) {
        els.statusDot.className = 'status-dot offline';
    }
}

// --- Fetch estat ---
async function fetchStatus() {
    try {
        const res = await fetch('/api/status');
        const d = await res.json();
        els.uptime.textContent = 'Uptime: ' + formatUptime(d.uptime);
        els.heap.textContent = 'Heap: ' + (d.heap / 1024).toFixed(0) + 'KB';
    } catch(e) {}
}

// --- Chart.js configuració ---
const chartDefaults = {
    responsive: true,
    maintainAspectRatio: true,
    animation: { duration: 500 },
    plugins: {
        legend: { labels: { color: '#9aa0a8', font: { family: 'Inter', size: 11 } } },
        tooltip: {
            backgroundColor: 'rgba(26,31,46,0.95)',
            titleColor: '#e8eaed',
            bodyColor: '#9aa0a8',
            borderColor: 'rgba(255,255,255,0.06)',
            borderWidth: 1,
            cornerRadius: 8,
        }
    },
    scales: {
        x: {
            type: 'time', time: { tooltipFormat: 'HH:mm', displayFormats: { hour: 'HH:mm' } },
            grid: { color: 'rgba(255,255,255,0.04)' },
            ticks: { color: '#5f6368', font: { size: 10 } }
        },
        y: {
            grid: { color: 'rgba(255,255,255,0.04)' },
            ticks: { color: '#5f6368', font: { size: 10 } }
        }
    }
};

function makeDataset(label, color, data) {
    return {
        label, data,
        borderColor: color,
        backgroundColor: color + '20',
        borderWidth: 2, pointRadius: 0, tension: 0.3, fill: true,
    };
}

function initCharts() {
    charts.temp = new Chart(document.getElementById('chart-temp'), {
        type: 'line',
        data: { datasets: [
            makeDataset('Interior', '#ff9100', []),
            makeDataset('Exterior', '#ff5252', [])
        ]},
        options: { ...chartDefaults, scales: { ...chartDefaults.scales, y: { ...chartDefaults.scales.y, title: { display: true, text: '°C', color: '#5f6368' } } } }
    });

    charts.hum = new Chart(document.getElementById('chart-hum'), {
        type: 'line',
        data: { datasets: [
            makeDataset('Interior', '#4fc3f7', []),
            makeDataset('Exterior', '#00e5ff', [])
        ]},
        options: { ...chartDefaults, scales: { ...chartDefaults.scales, y: { ...chartDefaults.scales.y, title: { display: true, text: '%RH', color: '#5f6368' } } } }
    });

    charts.air = new Chart(document.getElementById('chart-air'), {
        type: 'line',
        data: { datasets: [
            makeDataset('eCO2 (ppm)', '#69f0ae', []),
            makeDataset('TVOC (ppb)', '#b388ff', [])
        ]},
        options: chartDefaults
    });

    charts.wind = new Chart(document.getElementById('chart-wind'), {
        type: 'line',
        data: { datasets: [
            makeDataset('Vent (m/s)', '#e8eaed', []),
            makeDataset('Pluja (mm)', '#4fc3f7', [])
        ]},
        options: chartDefaults
    });
}

// --- Fetch historial i actualitzar gràfics ---
async function fetchHistory() {
    try {
        const res = await fetch(`/api/history?hours=${currentHours}`);
        const d = await res.json();
        const records = d.data || [];

        const toPoint = (rec, field) => ({ x: new Date(rec.t * 1000), y: rec[field] });

        charts.temp.data.datasets[0].data = records.map(r => toPoint(r, 'ti'));
        charts.temp.data.datasets[1].data = records.map(r => toPoint(r, 'to'));
        charts.temp.update('none');

        charts.hum.data.datasets[0].data = records.map(r => toPoint(r, 'hi'));
        charts.hum.data.datasets[1].data = records.map(r => toPoint(r, 'ho'));
        charts.hum.update('none');

        charts.air.data.datasets[0].data = records.map(r => toPoint(r, 'co2'));
        charts.air.data.datasets[1].data = records.map(r => toPoint(r, 'voc'));
        charts.air.update('none');

        charts.wind.data.datasets[0].data = records.map(r => toPoint(r, 'w'));
        charts.wind.data.datasets[1].data = records.map(r => toPoint(r, 'r'));
        charts.wind.update('none');
    } catch(e) {
        console.error('History fetch error:', e);
    }
}

// --- Botons de rang temporal ---
document.querySelectorAll('.chart-controls .btn').forEach(btn => {
    btn.addEventListener('click', () => {
        document.querySelectorAll('.chart-controls .btn').forEach(b => b.classList.remove('active'));
        btn.classList.add('active');
        currentHours = parseInt(btn.dataset.hours);
        fetchHistory();
    });
});

// --- Inicialització ---
document.addEventListener('DOMContentLoaded', () => {
    initCharts();
    fetchCurrent();
    fetchStatus();
    fetchHistory();

    setInterval(fetchCurrent, UPDATE_INTERVAL);
    setInterval(fetchStatus, 30000);
    setInterval(fetchHistory, 60000);
});
