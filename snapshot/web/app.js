// Gong Performance UI

const API = '';  // Same origin

let currentPreset = '';
let currentIR = '';

// DOM elements
const presetList = document.getElementById('preset-list');
const irList = document.getElementById('ir-list');
const currentPresetEl = document.getElementById('current-preset');
const currentIREl = document.getElementById('current-ir');
const tabs = document.querySelectorAll('.tab');
const panels = document.querySelectorAll('.panel');

// Tab switching
tabs.forEach(tab => {
    tab.addEventListener('click', () => {
        const target = tab.dataset.panel;
        tabs.forEach(t => t.classList.remove('active'));
        panels.forEach(p => p.classList.remove('active'));
        tab.classList.add('active');
        document.getElementById(target + '-panel').classList.add('active');
    });
});

// Swipe support for panel switching
let touchStartX = 0;
let touchStartY = 0;
const panelsEl = document.getElementById('panels');

panelsEl.addEventListener('touchstart', e => {
    touchStartX = e.touches[0].clientX;
    touchStartY = e.touches[0].clientY;
}, { passive: true });

panelsEl.addEventListener('touchend', e => {
    const dx = e.changedTouches[0].clientX - touchStartX;
    const dy = e.changedTouches[0].clientY - touchStartY;

    // Only swipe if horizontal movement > vertical and > threshold
    if (Math.abs(dx) > 80 && Math.abs(dx) > Math.abs(dy) * 1.5) {
        const tabNames = ['presets', 'irs'];
        const activeTab = document.querySelector('.tab.active');
        const currentIndex = tabNames.indexOf(activeTab.dataset.panel);

        let newIndex;
        if (dx < 0) newIndex = Math.min(currentIndex + 1, tabNames.length - 1);
        else newIndex = Math.max(currentIndex - 1, 0);

        if (newIndex !== currentIndex) {
            tabs.forEach(t => t.classList.remove('active'));
            panels.forEach(p => p.classList.remove('active'));
            tabs[newIndex].classList.add('active');
            document.getElementById(tabNames[newIndex] + '-panel').classList.add('active');
        }
    }
}, { passive: true });

function renderList(container, items, activeItem, type) {
    if (items.length === 0) {
        container.innerHTML = '<div class="empty-message">No ' + type + ' found</div>';
        return;
    }

    container.innerHTML = items.map(name => {
        const isActive = name === activeItem;
        return '<div class="list-item' + (isActive ? ' active' : '') + '" data-name="' +
            name.replace(/"/g, '&quot;') + '" data-type="' + type + '">' +
            name + '</div>';
    }).join('');

    // Add click handlers
    container.querySelectorAll('.list-item').forEach(item => {
        item.addEventListener('click', () => activate(item));
    });
}

async function activate(item) {
    const name = item.dataset.name;
    const type = item.dataset.type;

    item.classList.add('loading');

    try {
        const endpoint = type === 'presets'
            ? `/api/presets/${encodeURIComponent(name)}/activate`
            : `/api/irs/${encodeURIComponent(name)}/activate`;

        const res = await fetch(API + endpoint, { method: 'POST' });

        if (res.ok) {
            if (type === 'presets') {
                currentPreset = name;
                currentPresetEl.textContent = name;
                renderList(presetList, getItemNames(presetList), currentPreset, 'presets');
            } else {
                currentIR = name;
                currentIREl.textContent = name;
                renderList(irList, getItemNames(irList), currentIR, 'irs');
            }
        }
    } catch (err) {
        console.error('Activation failed:', err);
    }

    item.classList.remove('loading');
}

function getItemNames(container) {
    return Array.from(container.querySelectorAll('.list-item')).map(el => el.dataset.name);
}

async function fetchPresets() {
    try {
        const res = await fetch(API + '/api/presets');
        const names = await res.json();
        renderList(presetList, names, currentPreset, 'presets');
    } catch (err) {
        console.error('Failed to fetch presets:', err);
    }
}

async function fetchIRs() {
    try {
        const res = await fetch(API + '/api/irs');
        const names = await res.json();
        renderList(irList, names, currentIR, 'irs');
    } catch (err) {
        console.error('Failed to fetch IRs:', err);
    }
}

async function fetchStatus() {
    try {
        const res = await fetch(API + '/api/status');
        const status = await res.json();

        if (status.preset !== currentPreset) {
            currentPreset = status.preset;
            currentPresetEl.textContent = currentPreset || '--';
            // Re-render to update active state
            const presetNames = getItemNames(presetList);
            if (presetNames.length > 0) {
                renderList(presetList, presetNames, currentPreset, 'presets');
            }
        }

        if (status.ir !== currentIR) {
            currentIR = status.ir;
            currentIREl.textContent = currentIR || '--';
            const irNames = getItemNames(irList);
            if (irNames.length > 0) {
                renderList(irList, irNames, currentIR, 'irs');
            }
        }
    } catch (err) {
        // Silently retry on next poll
    }
}

// Initial load
fetchPresets();
fetchIRs();
fetchStatus();

// Poll for status changes every 2 seconds
setInterval(fetchStatus, 2000);
