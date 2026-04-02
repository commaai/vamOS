import { FlashManager, Step, ErrorCode, loadProgrammer } from "./utils/manager";
import { getManifest } from "./utils/manifest";

import portsThree from "./assets/qdl-ports-threex.svg";
import portsFour from "./assets/qdl-ports-four.svg";
import comma3X from "./assets/comma3X.webp";
import commaFour from "./assets/four_screen_on.webp";

// -- State --
let manager: FlashManager | null = null;
let selectedDevice: "comma3" | "comma4" | null = null;

const isLinux = navigator.platform.includes("Linux");
const isWindows = navigator.platform.includes("Win") ||
  (navigator as any).userAgentData?.platform === "Windows";

// -- Helpers --
function $(id: string) { return document.getElementById(id)!; }

function showStep(id: string) {
  document.querySelectorAll(".step").forEach(s => s.classList.remove("active"));
  $(id).classList.add("active");
}

function getStepLabels(): string[] {
  const steps = ["Device"];
  if (isWindows) steps.push("Driver");
  steps.push("Connect");
  if (isLinux && selectedDevice === "comma3") steps.push("Unbind");
  steps.push("Flash");
  return steps;
}

function updateStepper(current: number) {
  const labels = getStepLabels();
  for (const el of document.querySelectorAll("[data-stepper]")) {
    el.innerHTML = "";
    const stepper = document.createElement("div");
    stepper.className = "stepper";
    labels.forEach((label, i) => {
      if (i > 0) {
        const line = document.createElement("div");
        line.className = "stepper-line" + (i <= current ? " done" : "");
        stepper.appendChild(line);
      }
      const btn = document.createElement("button");
      btn.className = "stepper-btn" + (i === current ? " active" : i < current ? " done" : "");
      btn.textContent = i < current ? `\u2713 ${label}` : label;
      if (i < current) btn.onclick = () => navigateTo(label);
      else btn.disabled = true;
      stepper.appendChild(btn);
    });
    el.appendChild(stepper);
  }
}

function navigateTo(label: string) {
  const stepMap: Record<string, () => void> = {
    "Device": () => { showStep("step-device"); renderDevicePicker(); },
    "Driver": () => { showStep("step-zadig"); renderZadig(); },
    "Connect": () => { showStep("step-connect"); renderConnect(); },
    "Unbind": () => { showStep("step-unbind"); renderUnbind(); },
  };
  const handler = stepMap[label];
  if (handler) {
    handler();
    updateStepper(getStepLabels().indexOf(label));
  }
}

// -- Steps --
function renderLanding() {
  $("step-landing").innerHTML = `
    <div style="margin-bottom: 1rem; font-size: 4rem;">
      <span class="bounce" style="animation-delay: 0s">🌟</span>
      <span class="bounce" style="animation-delay: 0.2s">🗺️</span>
      <span class="bounce" style="animation-delay: 0.4s">🌟</span>
    </div>
    <h1>~*~ flashpack ~*~</h1>
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
    showStep("step-device");
    renderDevicePicker();
    updateStepper(0);
  };
}

function renderDevicePicker() {
  $("step-device").innerHTML = `
    <div data-stepper></div>
    <div style="font-size: 3rem; margin-bottom: 1rem;">🎒</div>
    <h2>which device are you flashing?</h2>
    <p class="subtitle">pick your comma device!</p>
    <div style="display: flex; gap: 1.5rem; justify-content: center; flex-wrap: wrap; margin-bottom: 2rem;">
      <button class="device-card" id="pick-comma3">
        <img src="${comma3X}" alt="comma 3X" style="height: 8rem; object-fit: contain;">
        <span>comma three<br>comma 3X</span>
      </button>
      <button class="device-card" id="pick-comma4">
        <img src="${commaFour}" alt="comma four" style="height: 8rem; object-fit: contain;">
        <span>comma four</span>
      </button>
    </div>
    <button class="btn btn-primary" id="btn-device-next" disabled>next!</button>
  `;

  function select(device: "comma3" | "comma4") {
    selectedDevice = device;
    document.querySelectorAll(".device-card").forEach(c => c.classList.remove("selected"));
    $(`pick-${device}`).classList.add("selected");
    ($("btn-device-next") as HTMLButtonElement).disabled = false;
  }

  $("pick-comma3").onclick = () => select("comma3");
  $("pick-comma4").onclick = () => select("comma4");
  if (selectedDevice) select(selectedDevice);
  $("btn-device-next").onclick = () => {
    if (isWindows) {
      showStep("step-zadig");
      renderZadig();
      updateStepper(getStepLabels().indexOf("Driver"));
    } else {
      showStep("step-connect");
      renderConnect();
      updateStepper(getStepLabels().indexOf("Connect"));
    }
  };
}

function renderZadig() {
  const vendorId = selectedDevice === "comma4" ? "3801" : "05C6";
  $("step-zadig").innerHTML = `
    <div data-stepper></div>
    <h2>install USB driver</h2>
    <p class="subtitle">Windows needs a driver to communicate with your device</p>
    <div class="instructions" style="max-width: 32rem;">
      <ol>
        <li><span class="step-num">1</span><span>Download and run <a href="https://zadig.akeo.ie/" target="_blank" style="color: var(--pink); font-weight: 700;">Zadig</a></span></li>
        <li><span class="step-num">2</span><span>Under <strong>Device</strong> in the menu bar, select <strong>Create New Device</strong></span></li>
        <li><span class="step-num">3</span><span>Fill in the form:<br>
          Name: <code style="background: rgba(255,255,255,0.1); padding: 0.125rem 0.5rem; border-radius: 0.25rem;">${selectedDevice === "comma4" ? "comma four" : "comma 3/3X"}</code><br>
          USB ID: <code style="background: rgba(255,255,255,0.1); padding: 0.125rem 0.5rem; border-radius: 0.25rem;">${vendorId}</code> and <code style="background: rgba(255,255,255,0.1); padding: 0.125rem 0.5rem; border-radius: 0.25rem;">9008</code></span></li>
        <li><span class="step-num">4</span><span>Click <strong>Install Driver</strong></span></li>
      </ol>
    </div>
    <button class="btn btn-primary" id="btn-zadig-done">done!</button>
  `;
  $("btn-zadig-done").onclick = () => {
    showStep("step-connect");
    renderConnect();
    updateStepper(getStepLabels().indexOf("Connect"));
  };
}

function renderConnect() {
  const isFour = selectedDevice === "comma4";
  const portsImg = isFour ? portsFour : portsThree;

  const steps = isFour
    ? `<li><span class="step-num">A</span><span>Unplug the device</span></li>
       <li><span class="step-num">B</span><span>Connect <strong>port 1</strong> to your computer</span></li>
       <li><span class="step-num">C</span><span>Connect <strong>port 2</strong> to your computer or a power brick</span></li>`
    : `<li><span class="step-num">A</span><span>Unplug the device</span></li>
       <li><span class="step-num">B</span><span>Wait for the light on the back to fully turn off</span></li>
       <li><span class="step-num">C</span><span>Connect <strong>port 1</strong> to your computer</span></li>
       <li><span class="step-num">D</span><span>Connect <strong>port 2</strong> to your computer or a power brick</span></li>`;

  $("step-connect").innerHTML = `
    <div data-stepper></div>
    <h2>connect ur device!</h2>
    <p class="subtitle">follow these steps to prepare your device for flashing</p>
    <div style="display: flex; gap: 2rem; align-items: center; justify-content: center; flex-wrap: wrap; margin-bottom: 1.5rem;">
      <img src="${portsImg}" alt="port diagram" style="height: 12rem;">
      <div class="instructions"><ol>${steps}</ol></div>
    </div>
    <p style="color: var(--cyan); margin-bottom: 1.5rem;">the device screen will be blank. that's totally normal!</p>
    <button class="btn btn-primary" id="btn-connect-next">next!</button>
  `;

  $("btn-connect-next").onclick = () => {
    if (isLinux && selectedDevice === "comma3") {
      showStep("step-unbind");
      renderUnbind();
      updateStepper(getStepLabels().indexOf("Unbind"));
    } else {
      showStep("step-webusb");
      renderWebUSB();
      updateStepper(getStepLabels().indexOf("Flash"));
    }
  };
}

function renderUnbind() {
  $("step-unbind").innerHTML = `
    <div data-stepper></div>
    <h2>unbind from qcserial</h2>
    <p class="subtitle">on Linux, devices in QDL mode are bound to the kernel's qcserial driver. run this command in a terminal to unbind it:</p>
    <div class="code-block"><button class="copy-btn" id="btn-copy-unbind">Copy</button>for d in /sys/bus/usb/drivers/qcserial/*-*; do [ -e "$d" ] && echo -n "$(basename $d)" | sudo tee /sys/bus/usb/drivers/qcserial/unbind > /dev/null; done</div>
    <button class="btn btn-primary" id="btn-unbind-done">done!</button>
  `;

  $("btn-copy-unbind").onclick = () => {
    const cmd = 'for d in /sys/bus/usb/drivers/qcserial/*-*; do [ -e "$d" ] && echo -n "$(basename $d)" | sudo tee /sys/bus/usb/drivers/qcserial/unbind > /dev/null; done';
    navigator.clipboard.writeText(cmd);
    $("btn-copy-unbind").textContent = "Copied!";
    setTimeout(() => { $("btn-copy-unbind").textContent = "Copy"; }, 2000);
  };

  $("btn-unbind-done").onclick = () => {
    showStep("step-webusb");
    renderWebUSB();
    updateStepper(getStepLabels().length - 1);
  };
}

function renderWebUSB() {
  $("step-webusb").innerHTML = `
    <div data-stepper></div>
    <div style="font-size: 3rem; margin-bottom: 1rem;">🔌</div>
    <h2>select your device</h2>
    <p class="subtitle">click the button below to open the device selector</p>
    <button class="btn btn-primary" id="btn-webusb-connect">connect! 🎒</button>
    <p style="margin-top: 0.75rem; color: rgba(255,255,255,0.5); font-size: 0.875rem;">
      pick <code style="background: var(--lime); padding: 0.125rem 0.5rem; border-radius: 0.25rem; font-weight: 700; color: black;">QUSB_BULK_CID</code> from the list!
    </p>
  `;
  $("btn-webusb-connect").onclick = () => startFlashing();
}

function renderFlash() {
  $("step-flash").innerHTML = `
    <div data-stepper></div>
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
    <div data-stepper></div>
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
  const flashIdx = getStepLabels().indexOf("Flash");
  updateStepper(flashIdx);
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
    updateStepper(getStepLabels().length);
    renderDone();
  }
}

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
    const [programmer, { version, manifest }] = await Promise.all([
      loadProgrammer(),
      getManifest(),
    ]);

    const sha = process.env.GIT_SHA || "master";
    const versionLink = document.getElementById("version-link") as HTMLAnchorElement;
    versionLink.href = `https://github.com/commaai/vamOS/tree/${sha}`;
    versionLink.textContent = sha.slice(0, 7);

    console.info("[flashpack] Manifest loaded:", manifest.length, "entries");
    manager = new FlashManager(programmer, {});
    await manager.initialize(manifest);

    if (manager.error !== ErrorCode.NONE) {
      throw new Error("Initialization failed");
    }

    $("init-status").textContent = "";
    ($("btn-start") as HTMLButtonElement).disabled = false;
  } catch (err: any) {
    console.error("[flashpack] Init failed:", err);
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
