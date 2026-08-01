/**
 * PROTOTYPE — throwaway.
 * Three variants of Tramp app chrome / player UI, switchable via ?variant=
 * Question: What should v1 scalable UI look like (dense, playlist-centric, frameless)?
 */

const TRACKS = [
  { n: "01", title: "Night Bus", artist: "Kettles", dur: "3:41" },
  { n: "02", title: "Static Hymn", artist: "Wire Garden", dur: "4:12", active: true },
  { n: "03", title: "Copper Vein", artist: "Lumen Dirt", dur: "2:58" },
  { n: "04", title: "Afterimage", artist: "Soft Clamp", dur: "5:03" },
  { n: "05", title: "Dockside", artist: "Marlow", dur: "3:27" },
  { n: "06", title: "Low Orbit", artist: "Parcel", dur: "4:44" },
  { n: "07", title: "Redline", artist: "Ash Motor", dur: "3:09" },
  { n: "08", title: "Quiet Gear", artist: "Tannoy", dur: "6:01" },
];

const VARIANTS = [
  { key: "W", name: "Winner — A layout + B language" },
  { key: "A", name: "Transport stack" },
  { key: "B", name: "Playlist-first column" },
  { key: "C", name: "Playlist surface + HUD" },
];

function playlistHtml(extraClass = "") {
  return `
    <ul class="playlist ${extraClass}">
      ${TRACKS.map(
        (t) => `
        <li class="${t.active ? "active" : ""}">
          <span class="n">${t.n}</span>
          <span><span class="meta-title">${t.title}</span> <span class="meta-sub">— ${t.artist}</span></span>
          <span class="dur">${t.dur}</span>
        </li>`
      ).join("")}
    </ul>`;
}

function winControls() {
  return `
    <div class="win-controls" aria-label="Window controls">
      <button type="button" class="min" title="Minimize" aria-label="Minimize"></button>
      <button type="button" class="close" title="Close" aria-label="Close"></button>
    </div>`;
}

function VariantW() {
  return `
    <div class="desktop">
      <div class="window w-window" role="application" aria-label="Tramp locked direction">
        <div class="resize-hint"></div>
        <header class="w-top drag-bar">
          <div class="w-brand">TRAMP<span>.</span></div>
          ${winControls()}
        </header>
        <section class="w-transport">
          <div class="w-now">
            <div>
              <div class="meta-title">Static Hymn</div>
              <div class="meta-sub">Wire Garden · album cut</div>
            </div>
            <div class="meta-sub">1:48 / 4:12</div>
          </div>
          <input class="seek" type="range" min="0" max="100" value="42" aria-label="Seek" />
          <div class="w-controls">
            <button class="btn" type="button">Prev</button>
            <button class="btn primary" type="button">Pause</button>
            <button class="btn" type="button">Next</button>
            <button class="btn" type="button">Shuffle</button>
            <button class="btn" type="button">Repeat</button>
          </div>
          <div class="w-vol-row">
            <span>Vol</span>
            <input class="vol" type="range" min="0" max="100" value="70" aria-label="Volume" />
            <span>Mute</span>
          </div>
        </section>
        ${playlistHtml("w-playlist")}
      </div>
    </div>`;
}

function VariantA() {
  return `
    <div class="desktop">
      <div class="window a-window" role="application" aria-label="Tramp variant A">
        <div class="resize-hint"></div>
        <header class="a-top drag-bar">
          <div class="brand a-brand">Tramp<span>.</span></div>
          ${winControls()}
        </header>
        <section class="a-transport">
          <div class="a-now">
            <div>
              <div class="meta-title">Static Hymn</div>
              <div class="meta-sub">Wire Garden · album cut</div>
            </div>
            <div class="meta-sub">1:48 / 4:12</div>
          </div>
          <input class="seek" type="range" min="0" max="100" value="42" aria-label="Seek" />
          <div class="a-controls">
            <button class="btn" type="button">Prev</button>
            <button class="btn primary" type="button">Pause</button>
            <button class="btn" type="button">Next</button>
            <button class="btn" type="button">Shuffle</button>
            <button class="btn" type="button">Repeat</button>
          </div>
          <div class="a-vol-row">
            <span>Vol</span>
            <input class="vol" type="range" min="0" max="100" value="70" aria-label="Volume" />
            <span>Mute</span>
          </div>
        </section>
        ${playlistHtml("a-playlist")}
      </div>
    </div>`;
}

function VariantB() {
  return `
    <div class="desktop">
      <div class="window b-window" role="application" aria-label="Tramp variant B">
        <div class="resize-hint"></div>
        <div class="b-left">
          <header class="b-head drag-bar">
            <div class="b-brand">TRAMP</div>
            ${winControls()}
          </header>
          ${playlistHtml("b-playlist")}
        </div>
        <aside class="b-right">
          <div class="b-art">Static<br/>Hymn</div>
          <div class="b-info">
            <div class="meta-title">Static Hymn</div>
            <div class="meta-sub">Wire Garden</div>
          </div>
          <div class="b-transport">
            <input class="seek" type="range" min="0" max="100" value="42" aria-label="Seek" />
            <div class="b-controls">
              <button class="btn" type="button">Prev</button>
              <button class="btn primary" type="button">Pause</button>
              <button class="btn" type="button">Next</button>
              <button class="btn" type="button">Shuffle</button>
              <button class="btn" type="button">Repeat</button>
            </div>
            <input class="vol" type="range" min="0" max="100" value="70" aria-label="Volume" />
          </div>
        </aside>
      </div>
    </div>`;
}

function VariantC() {
  return `
    <div class="desktop">
      <div class="window c-window" role="application" aria-label="Tramp variant C">
        <div class="resize-hint"></div>
        <header class="c-top drag-bar">
          <div class="brand c-brand">TRAMP</div>
          ${winControls()}
        </header>
        ${playlistHtml("c-playlist")}
        <footer class="c-hud">
          <div class="c-transport">
            <button class="btn" type="button">Prev</button>
            <button class="btn primary" type="button">Pause</button>
            <button class="btn" type="button">Next</button>
          </div>
          <div class="now">
            <div class="meta-title">Static Hymn</div>
            <div class="meta-sub">Wire Garden · 1:48 / 4:12</div>
          </div>
          <div class="c-transport">
            <button class="btn" type="button">Shuffle</button>
            <button class="btn" type="button">Repeat</button>
          </div>
          <div class="c-seek-wrap">
            <input class="seek" type="range" min="0" max="100" value="42" aria-label="Seek" />
          </div>
        </footer>
      </div>
    </div>`;
}

const renderers = { W: VariantW, A: VariantA, B: VariantB, C: VariantC };

function currentKey() {
  const v = new URLSearchParams(location.search).get("variant") || "W";
  return VARIANTS.some((x) => x.key === v) ? v : "W";
}

function setVariant(key) {
  const url = new URL(location.href);
  url.searchParams.set("variant", key);
  history.replaceState(null, "", url);
  paint();
}

function paint() {
  const key = currentKey();
  const meta = VARIANTS.find((v) => v.key === key);
  document.getElementById("app").innerHTML = renderers[key]();
  document.getElementById("label").textContent = `${key} — ${meta.name}`;
  console.log("[prototype state]", { variant: key, name: meta.name, tracks: TRACKS.length });
}

function cycle(delta) {
  const i = VARIANTS.findIndex((v) => v.key === currentKey());
  const next = VARIANTS[(i + delta + VARIANTS.length) % VARIANTS.length];
  setVariant(next.key);
}

document.getElementById("prev").addEventListener("click", () => cycle(-1));
document.getElementById("next").addEventListener("click", () => cycle(1));

document.addEventListener("keydown", (e) => {
  const t = e.target;
  if (t && (t.tagName === "INPUT" || t.tagName === "TEXTAREA" || t.isContentEditable)) return;
  if (e.key === "ArrowLeft") cycle(-1);
  if (e.key === "ArrowRight") cycle(1);
});

paint();
