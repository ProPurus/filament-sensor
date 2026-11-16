/*
 * The UI reads its values from this small data adapter. When the ESP32 API is
 * ready, replace loadDemoData()/saveFilament() with fetch() calls to the device.
 * Keeping the interface independent of the transport makes the static page
 * usable during development and easy to serve from ESP32 flash later.
 */
(function () {
    'use strict';

    var spools = [
        { id: 1, name: 'Spool 01', material: 'PLA', colorName: 'White', color: '#b8b2d6', remaining: 720, capacity: 1000, selected: true },
        { id: 2, name: 'Spool 02', material: 'PETG', colorName: 'Ocean blue', color: '#668bd2', remaining: 680, capacity: 1000, selected: false },
        { id: 3, name: 'Spool 03', material: 'PLA', colorName: 'Coral', color: '#df8d7e', remaining: 420, capacity: 1000, selected: false },
        { id: 4, name: 'Spool 04', material: 'ABS', colorName: 'Graphite', color: '#626572', remaining: 250, capacity: 500, selected: false },
        { id: 5, name: 'Spool 05', material: 'PLA', colorName: 'Mint', color: '#69b79d', remaining: 500, capacity: 1000, selected: false },
        { id: 6, name: 'Spool 06', material: 'TPU', colorName: 'Yellow', color: '#d7b84e', remaining: 85, capacity: 500, selected: false }
    ];

    var views = {
        overview: document.getElementById('overview-view'),
        filaments: document.getElementById('filaments-view')
    };

    function escapeHtml(value) {
        return String(value).replace(/[&<>"']/g, function (character) {
            return { '&': '&amp;', '<': '&lt;', '>': '&gt;', '"': '&quot;', "'": '&#39;' }[character];
        });
    }

    function percentage(spool) {
        return Math.max(0, Math.min(100, Math.round(spool.remaining / spool.capacity * 100)));
    }

    function spoolCard(spool, compact) {
        var pct = percentage(spool);
        return '<article class="spool-card' + (spool.selected ? ' selected' : '') + '" data-spool-id="' + spool.id + '" tabindex="0" role="button" aria-pressed="' + spool.selected + '">' +
            '<div class="spool-title"><span>' + escapeHtml(spool.name) + '</span>' + (spool.selected ? '<span class="spool-selected">ACTIVE</span>' : '') + '</div>' +
            '<div class="spool-art" style="--spool-color:' + escapeHtml(spool.color) + '"><span class="spool-reel"></span><span class="spool-side"></span></div>' +
            '<div class="spool-value">' + spool.remaining + '<small> g</small></div><div class="spool-percent">' + pct + '% remaining</div>' +
            (compact ? '' : '') + '</article>';
    }

    function render() {
        var overviewSpools = document.getElementById('overview-spools');
        var filamentSpools = document.getElementById('filament-spools');
        var inventory = document.getElementById('inventory-list');
        var active = spools.filter(function (spool) { return spool.selected; })[0] || spools[0];

        overviewSpools.innerHTML = spools.slice(0, 4).map(function (spool) { return spoolCard(spool, true); }).join('');
        filamentSpools.innerHTML = spools.slice(0, 4).map(function (spool) { return spoolCard(spool, false); }).join('');
        inventory.innerHTML = spools.map(function (spool) {
            var pct = percentage(spool);
            return '<div class="inventory-row"><div class="material-name"><span class="material-swatch" style="--swatch:' + escapeHtml(spool.color) + '"></span>' + escapeHtml(spool.name) + ' <span class="muted">· ' + escapeHtml(spool.colorName) + '</span></div>' +
                '<span>' + escapeHtml(spool.material) + '</span><span class="inventory-amount">' + spool.remaining + ' <small>g · ' + pct + '%</small></span>' +
                '<span class="state-pill' + (pct < 20 ? ' low' : '') + '">' + (pct < 20 ? 'Low' : 'Ready') + '</span></div>';
        }).join('');

        document.querySelectorAll('.summary-card').forEach(function (card) {
            var label = card.querySelector('.card-label');
            if (label && label.textContent === 'ACTIVE SPOOL') {
                card.querySelector('.metric').innerHTML = escapeHtml(active.name);
                card.querySelector('.card-foot').innerHTML = '<span class="small-dot violet-dot"></span> ' + escapeHtml(active.material + ' · ' + active.colorName);
            } else if (label && label.textContent === 'REMAINING') {
                card.querySelector('.metric').innerHTML = active.remaining + ' <small>g</small>';
                card.querySelector('.progress-track span').style.width = percentage(active) + '%';
                card.querySelector('.card-foot').textContent = percentage(active) + '% of ' + active.capacity.toLocaleString('en-US') + ' g';
            }
        });
    }

    function selectView(name) {
        Object.keys(views).forEach(function (key) {
            var selected = key === name;
            views[key].hidden = !selected;
            views[key].classList.toggle('active', selected);
            var button = document.querySelector('[data-view="' + key + '"]');
            button.classList.toggle('active', selected);
            if (selected) button.setAttribute('aria-current', 'page');
            else button.removeAttribute('aria-current');
        });
        document.getElementById('page-title').textContent = name === 'overview' ? 'Overview' : 'Filaments';
    }

    function selectSpool(id) {
        spools.forEach(function (spool) { spool.selected = spool.id === id; });
        render();
    }

    function updateClock() {
        var now = new Date();
        document.getElementById('clock').textContent = now.toLocaleTimeString([], { hour: '2-digit', minute: '2-digit' });
        document.getElementById('date').textContent = now.toLocaleDateString([], { weekday: 'short', day: 'numeric', month: 'short', year: 'numeric' });
    }

    document.querySelectorAll('.nav-item').forEach(function (button) {
        button.addEventListener('click', function () { selectView(button.getAttribute('data-view')); });
    });
    document.addEventListener('click', function (event) {
        var card = event.target.closest('.spool-card');
        if (card) selectSpool(Number(card.getAttribute('data-spool-id')));
    });
    document.addEventListener('keydown', function (event) {
        var card = event.target.closest('.spool-card');
        if (card && (event.key === 'Enter' || event.key === ' ')) {
            event.preventDefault();
            selectSpool(Number(card.getAttribute('data-spool-id')));
        }
    });
    document.getElementById('add-spool').addEventListener('click', function () {
        window.alert('Filament editing can be connected to the ESP32 API here.');
    });

    render();
    updateClock();
    window.setInterval(updateClock, 30000);
}());
