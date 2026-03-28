import { FlashManager, Step, ErrorCode, loadProgrammer } from "./utils/manager";
import { getLatestManifestUrl } from "./utils/manifest";

// -- State --
let manager: FlashManager | null = null;

// -- Helpers --
function $(id: string) { return document.getElementById(id)!; }

function formatSize(bytes: number): string {
  if (bytes < 1024) return bytes + " B";
  if (bytes < 1024 * 1024) return (bytes / 1024).toFixed(1) + " KB";
  if (bytes < 1024 * 1024 * 1024) return (bytes / (1024 * 1024)).toFixed(1) + " MB";
  return (bytes / (1024 * 1024 * 1024)).toFixed(2) + " GB";
}

function showStep(id: string) {
  document.querySelectorAll(".step").forEach(s => s.classList.remove("active"));
  $(id).classList.add("active");
}

const stepLabels = ["Connect", "Flash", "Done"];

function updateStepper(current: number) {
  for (const id of ["stepper-connect", "stepper-flash", "stepper-done"]) {
    const el = document.getElementById(id);
    if (!el) continue;
    el.innerHTML = "";
    const stepper = document.createElement("div");
    stepper.className = "stepper";
    stepLabels.forEach((_, i) => {
      if (i > 0) {
        const line = document.createElement("div");
        line.className = "stepper-line" + (i <= current ? " done" : "");
        stepper.appendChild(line);
      }
      const dot = document.createElement("div");
      dot.className = "stepper-dot" + (i === current ? " active" : i < current ? " done" : "");
      dot.textContent = i < current ? "\u2713" : String(i + 1);
      stepper.appendChild(dot);
    });
    el.appendChild(stepper);
  }
}

// -- Render --
function renderLanding() {
  $("step-landing").innerHTML = `
    <div style="margin-bottom: 1rem; font-size: 4rem;">
      <span class="bounce" style="animation-delay: 0s">🌟</span>
      <span class="bounce" style="animation-delay: 0.2s">🗺️</span>
      <span class="bounce" style="animation-delay: 0.4s">🌟</span>
    </div>
    <h1>~*~ vamOS Flash ~*~</h1>
    <p class="subtitle">can YOU help me flash vamOS onto my comma device??</p>
    <div id="init-status" style="color: var(--cyan); margin-bottom: 1.5rem; font-size: 0.875rem;"></div>
    <button class="btn btn-primary" id="btn-start" disabled>vamonos!!</button>
    <div id="no-webusb" style="display:none;" class="error-box">
      oh no!! swiper swiped ur WebUSB!! please use <a href="https://www.google.com/chrome/" target="_blank" style="color:var(--pink);font-weight:700;">Google Chrome</a>!
    </div>
    <div id="init-error" style="display:none;" class="error-box"></div>
  `;
  $("btn-start").onclick = () => {
    if (!manager || manager.step !== Step.READY) return;
    showStep("step-connect");
    updateStepper(0);
    renderConnect();
  };
}

function renderConnect() {
  const isLinux = navigator.platform.includes("Linux");
  $("step-connect").innerHTML = `
    <div id="stepper-connect"></div>
    <div style="font-size: 3rem; margin-bottom: 1rem;">🎒🗺️</div>
    <h2>connect ur device!</h2>
    <p class="subtitle">we need YOUR help! put ur device into EDL mode!</p>
    <div class="instructions">
      <ol>
        <li><span class="step-num">1</span><span>Unplug the device and wait for it to fully power off</span></li>
        <li><span class="step-num">2</span><span>Connect <strong>port 1</strong> (USB-C closest to edge) to your computer</span></li>
        <li><span class="step-num">3</span><span>Connect <strong>port 2</strong> to power (computer or power brick)</span></li>
      </ol>
    </div>
    <p style="color: var(--cyan); margin-bottom: 1.5rem;">the device screen will be blank. that's totally normal!</p>
    ${isLinux ? `
      <p style="color: var(--warning); font-weight: 700; margin: 1rem 0;">🐧 Linux: unbind qcserial first!</p>
      <div class="code-block"><button class="copy-btn" id="btn-copy">Copy</button>for d in /sys/bus/usb/drivers/qcserial/*-*; do [ -e "$d" ] && echo -n "$(basename $d)" | sudo tee /sys/bus/usb/drivers/qcserial/unbind > /dev/null; done</div>
    ` : ""}
    <button class="btn btn-primary" id="btn-connect">say "connect"!! 🔌</button>
    <p style="margin-top: 0.75rem; color: rgba(255,255,255,0.5); font-size: 0.875rem;">
      pick <code style="background: var(--lime); padding: 0.125rem 0.5rem; border-radius: 0.25rem; font-weight: 700; color: black;">QUSB_BULK_CID</code> from the list!
    </p>
  `;
  if (isLinux) {
    $("btn-copy").onclick = () => {
      const cmd = 'for d in /sys/bus/usb/drivers/qcserial/*-*; do [ -e "$d" ] && echo -n "$(basename $d)" | sudo tee /sys/bus/usb/drivers/qcserial/unbind > /dev/null; done';
      navigator.clipboard.writeText(cmd);
      $("btn-copy").textContent = "Copied!";
      setTimeout(() => { $("btn-copy").textContent = "Copy"; }, 2000);
    };
  }
  $("btn-connect").onclick = () => startFlashing();
}

function renderFlash() {
  $("step-flash").innerHTML = `
    <div id="stepper-flash"></div>
    <div class="flash-icon icon-green animate-pulse" id="flash-icon">
      <span style="font-size: 3.5rem;">⚡</span>
    </div>
    <h2 id="flash-title">connecting...</h2>
    <p class="status-message" id="flash-status">do NOT unplug ur device!!</p>
    <div class="progress-container">
      <div class="progress-bar-bg"><div class="progress-bar-fill" id="progress-fill"></div></div>
      <div class="progress-text" id="progress-text"></div>
    </div>
    <div id="flash-serial" class="device-serial" style="display:none;"></div>
    <div id="flash-error" class="error-box" style="display:none;"></div>
    <button id="btn-retry" class="btn btn-secondary" style="display:none; margin-top:1rem;">try again!</button>
  `;
  $("btn-retry").onclick = () => location.reload();
}

function renderDone() {
  $("step-done").innerHTML = `
    <div id="stepper-done"></div>
    <div style="font-size: 4rem; margin-bottom: 1rem;">
      <span class="bounce" style="animation-delay: 0s">🎉</span>
      <span class="bounce" style="animation-delay: 0.15s">⭐</span>
      <span class="bounce" style="animation-delay: 0.3s">🎊</span>
    </div>
    <h2>we did it!! we did it!!</h2>
    <p class="subtitle">your device is rebooting into vamOS!! lo hicimos!!</p>
    <button class="btn btn-secondary" id="btn-again">flash another one!</button>
  `;
  $("btn-again").onclick = () => location.reload();
}

// -- Flash --
async function startFlashing() {
  showStep("step-flash");
  updateStepper(1);
  renderFlash();

  function setProgress(pct: number) {
    if (pct < 0) { $("progress-text").textContent = ""; return; }
    $("progress-fill").style.width = Math.min(pct * 100, 100) + "%";
    $("progress-text").textContent = Math.min(pct * 100, 100).toFixed(0) + "%";
  }

  manager!.callbacks.onStepChange = (step: number) => {
    const titles: Record<number, string> = {
      [Step.CONNECTING]: "connecting...",
      [Step.REPAIR_PARTITION_TABLES]: "repairing partition tables...",
      [Step.ERASE_DEVICE]: "erasing device...",
      [Step.FLASH_SYSTEM]: "flashing!! go go go!!",
      [Step.FINALIZING]: "almost done...",
    };
    if (titles[step]) $("flash-title").textContent = titles[step];
  };
  manager!.callbacks.onMessageChange = (msg: string) => {
    if (msg) $("flash-status").textContent = msg;
  };
  manager!.callbacks.onProgressChange = setProgress;
  manager!.callbacks.onSerialChange = (serial: string) => {
    $("flash-serial").textContent = "device serial: " + serial;
    $("flash-serial").style.display = "block";
  };
  manager!.callbacks.onErrorChange = (error: number) => {
    if (error === ErrorCode.NONE) return;
    $("flash-icon").className = "flash-icon icon-red";
    $("flash-icon").innerHTML = '<span style="font-size: 3.5rem;">😢</span>';
    $("flash-title").textContent = "oh no!! swiper no swiping!!";
    $("flash-status").textContent = "";
    $("flash-error").style.display = "block";
    $("flash-error").textContent = "something went wrong! try a different cable, USB port, or computer.";
    $("btn-retry").style.display = "inline-block";
  };

  await manager!.start();

  if (manager!.step === Step.DONE) {
    showStep("step-done");
    updateStepper(2);
    renderDone();
  }
}

// Expose callbacks for manager to use
(FlashManager.prototype as any).callbacks = {};

// -- Init --
async function init() {
  renderLanding();

  if (typeof navigator.usb === "undefined") {
    $("no-webusb").style.display = "block";
    $("btn-start").style.display = "none";
    return;
  }

  $("init-status").textContent = "loading programmer + manifest...";

  try {
    const [programmer, { manifestUrl }] = await Promise.all([
      loadProgrammer(),
      getLatestManifestUrl(),
    ]);

    manager = new FlashManager(programmer, {});
    await manager.initialize(manifestUrl);

    if (manager.error !== ErrorCode.NONE) {
      throw new Error("Initialization failed");
    }

    $("init-status").textContent = `ready! (${manager.step === Step.READY ? "manifest loaded" : "..."})`;
    ($("btn-start") as HTMLButtonElement).disabled = false;
  } catch (err: any) {
    console.error("[vamOS Flash] Init failed:", err);
    $("init-status").textContent = "";
    const el = $("init-error");
    el.style.display = "block";
    el.textContent = "failed to load: " + (err.message || err);
  }
}

window.addEventListener("beforeunload", (e) => {
  if ($("step-flash")?.classList.contains("active") && $("btn-retry")?.style.display !== "inline-block") {
    e.preventDefault();
    return (e.returnValue = "Flash in progress!!");
  }
});

init();
