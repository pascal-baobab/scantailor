# Phone Simulator

A visual mobile device simulator for testing web apps — supports iPhone and Samsung/Android devices. Built with React + TypeScript + Vite, runs standalone or embeddable as a VS Code webview.

## Installation

```bash
npm install
npm run dev
```

Open `http://localhost:5173` in your browser.

## Features

- **13 devices** — iPhone SE through iPhone 16 Pro Max, Galaxy S23/S24/A54/Z Fold/Z Flip
- **Photorealistic CSS frames** — metallic chassis, notch/Dynamic Island/punch-hole, side buttons, glass reflection
- **iOS & Android status bars** — time, signal, WiFi, battery with realistic styling
- **URL loading** — type any URL and see it rendered inside the device frame via iframe
- **Portrait/Landscape** — smooth rotation animation with correct viewport swap
- **Zoom controls** — 50%–150% presets plus fit-to-window mode
- **Side-by-side comparison** — place 2–3 devices next to each other loading the same URL
- **Screenshot capture** — screen-only or full device mockup, saved as PNG
- **Network throttling** — simulate Fast 4G, Slow 3G, 2G, or Offline conditions
- **DevTools panel** — viewport info, console log capture, and network tab
- **Dark/Light theme** — VS Code-style dark theme by default, with light mode toggle
- **Device quick switcher** — searchable sidebar with brand filtering
- **Keyboard accessible** — all controls have aria-labels and keyboard navigation

## Supported Devices

### Apple
| Device | Viewport | DPR |
|--------|----------|-----|
| iPhone SE (3rd) | 375 × 667 | 2 |
| iPhone 14 | 390 × 844 | 3 |
| iPhone 14 Pro | 393 × 852 | 3 |
| iPhone 15 | 393 × 852 | 3 |
| iPhone 15 Pro Max | 430 × 932 | 3 |
| iPhone 16 Pro | 402 × 874 | 3 |
| iPhone 16 Pro Max | 440 × 956 | 3 |

### Samsung
| Device | Viewport | DPR |
|--------|----------|-----|
| Galaxy S23 | 360 × 780 | 3 |
| Galaxy S24 Ultra | 384 × 824 | 3 |
| Galaxy A54 | 360 × 800 | 2 |
| Galaxy Z Fold 5 (Cover) | 374 × 832 | 3 |
| Galaxy Z Fold 5 (Inner) | 882 × 1104 | 2 |
| Galaxy Z Flip 5 | 360 × 748 | 3 |

## Tech Stack

- **Vite + React 18 + TypeScript** (strict mode)
- **Zero UI framework dependencies** — all custom CSS
- **html2canvas** for screenshot capture
- Pure CSS device frames — no images or external SVGs

## Project Structure

```
src/
├── components/     # UI components (Simulator, DeviceFrame, Toolbar, etc.)
├── devices/        # Device configuration registry (Apple, Samsung)
├── hooks/          # Custom React hooks (useDevice, useZoom, etc.)
├── styles/         # Global CSS, device styles, animations
└── types/          # TypeScript type definitions
```

## Future Roadmap

- VS Code extension packaging (webview panel)
- Touch gesture simulation (swipe, pinch-to-zoom)
- Device screen recording (video export)
- Custom device definitions
- Auto-reload on file changes (HMR proxy)
- Responsive breakpoint ruler overlay
- Performance metrics overlay (FPS, memory)
