// PixiJS backdrop: catalog fartScene, full-viewport, no CDN.
const C = { f4rtGreen: 0x02f53a, f4rtYellow: 0xe4f20f };
const reduceMotion = matchMedia('(prefers-reduced-motion: reduce)').matches;
const rand = (lo, hi) => lo + Math.random() * (hi - lo);

// ?fx / ?fx=true keeps only the canvas. ?fx=false / 0 / off stays a normal page.
const fxRaw = new URLSearchParams(location.search).get('fx');
const fxOnly = fxRaw !== null && !/^(0|false|off)$/i.test(fxRaw);
if (fxOnly) document.body.classList.add('fx-only');

const pointer = { x: 0.5, y: 0.5 };
if (!fxOnly) {
  addEventListener('pointermove', (e) => {
    pointer.x = e.clientX / innerWidth;
    pointer.y = e.clientY / innerHeight;
  }, { passive: true });
}

start().catch((err) => {
  console.info('fx: animated backdrop disabled:', err?.message || err);
  document.getElementById('fx')?.remove();
});

async function start() {
  const PIXI = await import('./vendor/pixi.min.mjs');
  const app = new PIXI.Application();
  await app.init({
    canvas: document.getElementById('fx'),
    resizeTo: window,
    backgroundAlpha: 0,
    antialias: true,
    autoDensity: true,
    resolution: Math.min(devicePixelRatio || 1, 2),
    preference: 'webgl',
  });

  const glow = glowTexture(PIXI);
  const { width: w0, height: h0 } = app.screen;
  const scene = fartScene(PIXI, glow, w0, h0);
  app.stage.addChild(scene.root);
  bootTicker(app, scene);
}

function bootTicker(app, scene) {
  if (reduceMotion) {
    app.ticker.stop();
    const still = () => {
      const { width: w, height: h } = app.screen;
      scene.update(8, w, h, 0);
      app.render();
    };
    addEventListener('resize', () => requestAnimationFrame(still));
    still();
    return;
  }

  let t = 0;
  app.ticker.add((ticker) => {
    const dt = Math.min(ticker.deltaMS / 1000, 0.05);
    t += dt;
    const { width: w, height: h } = app.screen;
    // Preview mode is full-bleed, so there is no page to parallax against.
    if (!fxOnly) {
      const ease = Math.min(1, dt * 2.5);
      scene.root.x += ((0.5 - pointer.x) * 40 - scene.root.x) * ease;
      scene.root.y += ((0.5 - pointer.y) * 24 - scene.root.y) * ease;
    }
    scene.update(t, w, h, dt);
  });

  document.addEventListener('visibilitychange', () => {
    if (document.hidden) app.ticker.stop(); else app.ticker.start();
  });
}

function glowTexture(PIXI, size = 64) {
  const c = document.createElement('canvas');
  c.width = c.height = size;
  const g = c.getContext('2d');
  const r = size / 2;
  const grd = g.createRadialGradient(r, r, 0, r, r, r);
  grd.addColorStop(0, 'rgba(255,255,255,1)');
  grd.addColorStop(0.25, 'rgba(255,255,255,0.5)');
  grd.addColorStop(1, 'rgba(255,255,255,0)');
  g.fillStyle = grd;
  g.fillRect(0, 0, size, size);
  return PIXI.Texture.from(c);
}

function glowSprite(PIXI, tex, tint) {
  const s = new PIXI.Sprite(tex);
  s.anchor.set(0.5);
  s.tint = tint;
  s.blendMode = 'add';
  return s;
}

const GLOW_PASSES = [[22, 0.05], [10, 0.14], [2.5, 1]];
function glowLine(g, pts, color, scale = 1, passes = GLOW_PASSES) {
  for (const [width, alpha] of passes) {
    g.moveTo(pts[0], pts[1]);
    for (let i = 2; i < pts.length; i += 2) g.lineTo(pts[i], pts[i + 1]);
    g.stroke({ width: width * scale, color, alpha, cap: 'round', join: 'round' });
  }
}

function fartScene(PIXI, glow, w0, h0) {
  const root = new PIXI.Container();
  const wave = new PIXI.Graphics();
  wave.blendMode = 'add';
  root.addChild(wave);

  const blobs = Array.from({ length: 28 }, (_, i) => {
    const s = glowSprite(PIXI, glow, i % 3 ? C.f4rtGreen : C.f4rtYellow);
    s.x = rand(0, w0);
    s.y = rand(0, h0);
    s.scale.set(rand(2.4, 7));
    s.alpha = rand(0.08, 0.2);
    s.vy = rand(-70, -22);
    s.ph = rand(0, 6.28);
    root.addChild(s);
    return s;
  });

  return {
    root,
    update(t, w, h, dt) {
      for (const s of blobs) {
        s.y += s.vy * dt;
        s.x += Math.sin(t * 0.6 + s.ph) * 18 * dt;
        s.alpha = 0.08 + 0.12 * (0.5 + 0.5 * Math.sin(t * 1.4 + s.ph));
        if (s.y < -80) {
          s.y = h + rand(40, 160);
          s.x = rand(0, w);
        }
      }

      wave.clear();
      // Fixed wavelength so the coil cycle cannot reverse travel.
      const k = 0.5 - 0.5 * Math.cos(t * 0.55);
      const amp = h * (0.015 + 0.22 * k);
      const cy = h * 0.58;
      const main = [], echo = [];
      for (let x = -10; x <= w + 10; x += 5) {
        const u = (x - t * 140) / 90;
        const spring = Math.sin(u) + k * 0.45 * Math.sin(u * 3) + k * k * 0.35 * Math.sin(u * 7);
        const y = cy + amp * spring;
        main.push(x, y);
        echo.push(x, cy + (y - cy) * 0.55);
      }
      glowLine(wave, echo, C.f4rtYellow, 0.7);
      glowLine(wave, main, C.f4rtGreen, 1 + 0.35 * k);
    },
  };
}
