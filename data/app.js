/**
 * ESP32 DeskOS — Dashboard Web Interface
 * Lightweight client script for device control and telemetry preview.
 */

document.addEventListener('DOMContentLoaded', () => {
    // DOM Elements
    const brightnessSlider = document.getElementById('brightness-slider');
    const brightnessVal = document.getElementById('brightness-val');
    const eyeMoodSelect = document.getElementById('eye-mood');
    const eyeElements = document.querySelectorAll('.eye-graphic');
    const btnPreview = document.getElementById('btn-preview');
    const btnSend = document.getElementById('btn-send');
    const customTextInput = document.getElementById('custom-text');
    const displayModeSelect = document.getElementById('display-mode');

    // Brightness Slider Live Update
    if (brightnessSlider && brightnessVal) {
        brightnessSlider.addEventListener('input', (e) => {
            const percent = Math.round((e.target.value / 255) * 100);
            brightnessVal.textContent = `${percent}%`;
        });
    }

    // Robot Eyes Mood Preview Animation
    if (eyeMoodSelect && eyeElements.length >= 2) {
        const leftEye = eyeElements[0];
        const rightEye = eyeElements[1];

        eyeMoodSelect.addEventListener('change', (e) => {
            const mood = e.target.value;
            applyEyeMood(leftEye, rightEye, mood);
        });
    }

    function applyEyeMood(left, right, mood) {
        // Reset transforms
        left.style.transform = 'none';
        right.style.transform = 'none';
        left.style.borderRadius = '12px';
        right.style.borderRadius = '12px';
        left.style.height = '48px';
        right.style.height = '48px';

        switch (mood) {
            case 'happy':
                left.style.borderRadius = '20px 20px 4px 4px';
                right.style.borderRadius = '20px 20px 4px 4px';
                left.style.transform = 'scaleY(0.7)';
                right.style.transform = 'scaleY(0.7)';
                break;
            case 'angry':
                left.style.borderRadius = '4px 20px 12px 12px';
                right.style.borderRadius = '20px 4px 12px 12px';
                left.style.transform = 'rotate(-12deg)';
                right.style.transform = 'rotate(12deg)';
                break;
            case 'skeptical':
                left.style.height = '48px';
                right.style.height = '24px';
                break;
            case 'sleepy':
                left.style.height = '14px';
                right.style.height = '14px';
                break;
            case 'glitch':
                left.style.transform = 'skew(15deg, 5deg)';
                right.style.transform = 'skew(-15deg, -5deg)';
                break;
            default: // Neutral
                break;
        }
    }

    // Custom Message & QR Buttons (Placeholder verification handlers)
    if (btnPreview) {
        btnPreview.addEventListener('click', () => {
            const text = customTextInput ? customTextInput.value : '';
            console.log(`[DeskOS UI] Preview clicked. Payload: "${text}"`);
            alert(`[DeskOS Preview]\nDisplay Mode: ${displayModeSelect.value}\nMessage: "${text || '(empty)'}"\n(Firmware link: Standby / v0.1 verification mode)`);
        });
    }

    if (btnSend) {
        btnSend.addEventListener('click', () => {
            console.log('[DeskOS UI] Push to device requested.');
            alert('[DeskOS Action]\nPush request queued. Flash DeskOS v0.1 firmware to enable hardware REST/WebSocket listener.');
        });
    }

    // Periodic simulation of agent telemetry (until agent bridge connects)
    let simulatedCpu = 18;
    let simulatedRam = 42;
    setInterval(() => {
        simulatedCpu = Math.max(5, Math.min(95, Math.round(simulatedCpu + (Math.random() * 10 - 5))));
        const cpuStat = document.getElementById('cpu-stat');
        const cpuBar = document.getElementById('cpu-bar');
        if (cpuStat && cpuBar) {
            cpuStat.innerHTML = `${simulatedCpu}<small>%</small>`;
            cpuBar.style.width = `${simulatedCpu}%`;
        }

        const ramStat = document.getElementById('ram-stat');
        const ramBar = document.getElementById('ram-bar');
        if (ramStat && ramBar) {
            ramStat.innerHTML = `${simulatedRam}<small>%</small>`;
            ramBar.style.width = `${simulatedRam}%`;
        }
    }, 3000);
});
