import { qdlDevice } from "@commaai/qdl";
import { usbClass } from "@commaai/qdl/usblib";

const PROGRAMMER_URL = "https://raw.githubusercontent.com/commaai/flash/master/src/QDL/programmer.bin";

// -- State --
let programmer: ArrayBuffer | null = null;
let bootFile: File | null = null;
let systemFile: File | null = null;

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

const stepLabels = ["Images", "Connect", "Flash", "Done"];

function updateStepper(current: number) {
  for (const id of ["stepper-images", "stepper-connect", "stepper-flash", "stepper-done"]) {
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

// -- Render steps --
function renderLanding() {
  $("step-landing").innerHTML = `
    <div style="margin-bottom: 2rem;">
      <svg width="80" height="80" viewBox="2 3 42 42" fill="currentColor" style="color: var(--text);">
        <path fill-rule="evenodd" clip-rule="evenodd" d="M16.6964 40C16.6964 39.2596 16.6385 38.6393 16.7236 38.0415C16.7599 37.7865 17.0575 37.5135 17.3001 37.3595C18.4832 36.6087 19.7684 36.0092 20.8699 35.1481C24.4378 32.3587 26.5526 28.6866 26.6682 23.9166C26.7009 22.5622 26.203 22.2238 25.0654 22.7514C21.7817 24.2746 18.2505 23.3815 16.3659 20.5509C14.3107 17.4636 14.6001 13.3531 17.0626 10.6562C20.2079 7.21156 25.3833 7.10849 28.9522 10.3982C31.09 12.3688 32.1058 14.9132 32.3591 17.8074C33.2084 27.5032 28.3453 35.495 19.4941 39.0057C18.6181 39.353 17.7198 39.6382 16.6964 40Z"/>
      </svg>
    </div>
    <h1>~*~ vamOS Flash ~*~</h1>
    <p class="subtitle">flash vamOS onto ur comma device via WebUSB !!</p>
    <button class="btn btn-primary" id="btn-start">Let's Go!</button>
    <div id="no-webusb" style="display:none;" class="error-box">
      oh no!! this browser does not support WebUSB. please use <a href="https://www.google.com/chrome/" target="_blank" style="color:var(--pink);font-weight:700;">Google Chrome</a> or Microsoft Edge!!
    </div>
  `;
  $("btn-start").onclick = () => {
    if (!programmer) { alert("Programmer binary is still loading. Please wait."); return; }
    showStep("step-images");
    updateStepper(0);
  };
}

function renderImages() {
  $("step-images").innerHTML = `
    <div id="stepper-images"></div>
    <h2>Pick ur images!</h2>
    <p class="subtitle">grab the boot and system images to flash</p>
    <div class="download-links">
      <p>Download the latest release:</p>
      <a href="https://github.com/commaai/vamOS/releases/latest" target="_blank">vamOS Releases</a>
    </div>
    <div id="file-boot" class="file-row">
      <span class="label">boot.img</span>
      <span class="filename" id="boot-filename">No file selected</span>
      <label class="file-input-btn">Browse<input type="file" accept=".img" style="display:none" id="input-boot"></label>
    </div>
    <div id="file-system" class="file-row">
      <span class="label">system.img</span>
      <span class="filename" id="system-filename">No file selected</span>
      <label class="file-input-btn">Browse<input type="file" accept=".img" style="display:none" id="input-system"></label>
    </div>
    <div style="margin-top: 1.5rem;">
      <button class="btn btn-primary" id="btn-images-next" disabled>Next</button>
    </div>
  `;

  function onFileSelect(type: "boot" | "system", input: HTMLInputElement) {
    const file = input.files?.[0];
    if (!file) return;
    if (type === "boot") bootFile = file; else systemFile = file;
    const nameEl = $(`${type}-filename`);
    nameEl.textContent = `${file.name} (${formatSize(file.size)})`;
    nameEl.classList.add("set");
    $(`file-${type}`).classList.add("ready");
    if (bootFile && systemFile) ($("btn-images-next") as HTMLButtonElement).disabled = false;
  }

  $("input-boot").onchange = function() { onFileSelect("boot", this as HTMLInputElement); };
  $("input-system").onchange = function() { onFileSelect("system", this as HTMLInputElement); };
  $("btn-images-next").onclick = () => { showStep("step-connect"); updateStepper(1); };
}

function renderConnect() {
  const isLinux = navigator.platform.includes("Linux");
  $("step-connect").innerHTML = `
    <div id="stepper-connect"></div>
    <h2>Connect ur device!</h2>
    <p class="subtitle">put your device into EDL mode and plug it in</p>
    <div class="instructions">
      <ol>
        <li><span class="step-num">1</span><span>Unplug the device and wait for it to fully power off</span></li>
        <li><span class="step-num">2</span><span>Connect <strong>port 1</strong> (USB-C closest to edge) to your computer</span></li>
        <li><span class="step-num">3</span><span>Connect <strong>port 2</strong> to power (computer or power brick)</span></li>
      </ol>
    </div>
    <p style="color: var(--text-secondary); margin-bottom: 1.5rem;">The device screen will remain blank. This is normal.</p>
    ${isLinux ? `
      <p style="color: var(--warning); font-weight: 600; margin: 1rem 0;">Linux: unbind qcserial first</p>
      <div class="code-block"><button class="copy-btn" id="btn-copy">Copy</button>for d in /sys/bus/usb/drivers/qcserial/*-*; do [ -e "$d" ] && echo -n "$(basename $d)" | sudo tee /sys/bus/usb/drivers/qcserial/unbind > /dev/null; done</div>
    ` : ""}
    <button class="btn btn-primary" id="btn-connect">Connect Device</button>
    <p style="margin-top: 0.75rem; color: var(--text-secondary); font-size: 0.875rem;">
      Select <code style="background: var(--green); padding: 0.125rem 0.5rem; border-radius: 0.25rem; font-weight: 600;">QUSB_BULK_CID</code> from the browser dialog
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
      <svg width="64" height="64" viewBox="0 0 24 24" fill="black"><path d="M7 2v11h3v9l7-12h-4l4-8z"/></svg>
    </div>
    <h2 id="flash-title">Connecting...</h2>
    <p class="status-message" id="flash-status">Do not unplug your device</p>
    <div class="progress-container">
      <div class="progress-bar-bg"><div class="progress-bar-fill" id="progress-fill"></div></div>
      <div class="progress-text" id="progress-text"></div>
    </div>
    <div id="flash-serial" class="device-serial" style="display:none;"></div>
    <div id="flash-error" class="error-box" style="display:none;"></div>
    <button id="btn-retry" class="btn btn-primary" style="display:none; margin-top:1rem;">Retry</button>
  `;
  $("btn-retry").onclick = () => location.reload();
}

function renderDone() {
  $("step-done").innerHTML = `
    <div id="stepper-done"></div>
    <div class="flash-icon icon-green">
      <svg width="64" height="64" viewBox="0 0 24 24" fill="black"><path d="M9 16.17L4.83 12l-1.42 1.41L9 19 21 7l-1.41-1.41L9 16.17z"/></svg>
    </div>
    <h2>We did it!</h2>
    <p class="subtitle">your device is rebooting into vamOS!!</p>
    <button class="btn btn-secondary" id="btn-again">Flash another one!</button>
  `;
  $("btn-again").onclick = () => location.reload();
}

// -- Flash logic --
async function startFlashing() {
  showStep("step-flash");
  updateStepper(2);
  renderFlash();

  function setProgress(pct: number) {
    $("progress-fill").style.width = Math.min(pct, 100) + "%";
    $("progress-text").textContent = Math.min(pct, 100).toFixed(0) + "%";
  }

  function setStatus(title: string, message?: string) {
    $("flash-title").textContent = title;
    $("flash-status").textContent = message ?? "";
  }

  function setError(message: string) {
    $("flash-icon").className = "flash-icon icon-red";
    $("flash-title").textContent = "Error";
    $("flash-status").textContent = "";
    $("flash-error").style.display = "block";
    $("flash-error").textContent = message;
    $("btn-retry").style.display = "inline-block";
  }

  try {
    setStatus("Waiting for device...", "Select your device in the browser dialog");
    const usb = new usbClass();
    const qdl = new qdlDevice(programmer!);

    try {
      await qdl.connect(usb);
    } catch (err: any) {
      if (err.name === "NotFoundError") {
        showStep("step-connect");
        updateStepper(1);
        return;
      }
      throw err;
    }

    console.info("[vamOS Flash] Connected");
    setStatus("Connected", "Reading device info...");

    try {
      const storageInfo = await qdl.getStorageInfo();
      const serial = Number(storageInfo.serial_num).toString(16).padStart(8, "0");
      $("flash-serial").textContent = "Device serial: " + serial;
      $("flash-serial").style.display = "block";
    } catch (err) {
      console.warn("[vamOS Flash] Could not read storage info:", err);
    }

    const bootSize = bootFile!.size;
    const systemSize = systemFile!.size;

    // Flash boot_a (10%)
    setStatus("Flashing boot_a...", "Do not unplug your device");
    setProgress(0);
    await qdl.flashBlob("boot_a", bootFile!, (written: number) => {
      setProgress((written / bootSize) * 10);
    });
    console.info("[vamOS Flash] boot_a done");

    // Flash boot_b (10%)
    setStatus("Flashing boot_b...", "Do not unplug your device");
    await qdl.flashBlob("boot_b", bootFile!, (written: number) => {
      setProgress(10 + (written / bootSize) * 10);
    });
    console.info("[vamOS Flash] boot_b done");

    // Flash system_a (75%)
    setStatus("Flashing system_a...", "This may take several minutes. Do not unplug your device.");
    await qdl.flashBlob("system_a", systemFile!, (written: number) => {
      setProgress(20 + (written / systemSize) * 75);
    }, false);
    console.info("[vamOS Flash] system_a done");

    // Finalize
    setStatus("Finalizing...", "Setting active slot");
    setProgress(95);
    await qdl.setActiveSlot("a");

    setStatus("Rebooting...", "");
    setProgress(100);
    await qdl.reset();

    showStep("step-done");
    updateStepper(3);
    renderDone();

  } catch (err: any) {
    console.error("[vamOS Flash] Flash failed:", err);
    setError(err.message || "An unknown error occurred. Try a different cable, USB port, or computer.");
  }
}

// -- Init --
async function init() {
  renderLanding();
  renderImages();
  renderConnect();

  if (typeof navigator.usb === "undefined") {
    $("no-webusb").style.display = "block";
    $("btn-start").style.display = "none";
    return;
  }

  try {
    const res = await fetch(PROGRAMMER_URL);
    if (!res.ok) throw new Error(`Failed to fetch programmer: ${res.status}`);
    programmer = await res.arrayBuffer();
    console.info("[vamOS Flash] Programmer loaded:", programmer.byteLength, "bytes");
  } catch (err) {
    console.error("[vamOS Flash] Failed to load programmer:", err);
  }
}

window.addEventListener("beforeunload", (e) => {
  if ($("step-flash").classList.contains("active") && $("btn-retry").style.display !== "inline-block") {
    e.preventDefault();
    return (e.returnValue = "Flash in progress. Are you sure you want to leave?");
  }
});

init();
