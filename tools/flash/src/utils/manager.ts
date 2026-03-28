import { qdlDevice } from "@commaai/qdl";
import { usbClass } from "@commaai/qdl/usblib";

import type { ManifestEntry } from "./manifest";
import { ImageManager } from "./image";
import { createSteps, withProgress } from "./progress";

const PROGRAMMER_URL =
  "https://raw.githubusercontent.com/commaai/flash/master/src/QDL/programmer.bin";

export const Step = {
  INITIALIZING: 0,
  READY: 1,
  CONNECTING: 2,
  REPAIR_PARTITION_TABLES: 3,
  ERASE_DEVICE: 4,
  FLASH_SYSTEM: 5,
  FINALIZING: 6,
  DONE: 7,
} as const;

export const ErrorCode = {
  NONE: 0,
  UNKNOWN: -1,
  REQUIREMENTS_NOT_MET: 1,
  STORAGE_SPACE: 2,
  LOST_CONNECTION: 3,
  REPAIR_FAILED: 4,
  ERASE_FAILED: 5,
  FLASH_FAILED: 6,
} as const;

export interface FlashCallbacks {
  onStepChange?: (step: number) => void;
  onMessageChange?: (message: string) => void;
  onProgressChange?: (progress: number) => void;
  onErrorChange?: (error: number) => void;
  onConnectionChange?: (connected: boolean) => void;
  onSerialChange?: (serial: string) => void;
}

export class FlashManager {
  private callbacks: FlashCallbacks;
  private device: qdlDevice;
  private imageManager: ImageManager;
  private manifest: ManifestEntry[] | null = null;
  step = Step.INITIALIZING;
  error = ErrorCode.NONE;

  constructor(programmer: ArrayBuffer, callbacks: FlashCallbacks = {}) {
    this.callbacks = callbacks;
    this.device = new qdlDevice(programmer);
    this.imageManager = new ImageManager();
  }

  private setStep(step: number) {
    this.step = step;
    this.callbacks.onStepChange?.(step);
  }

  private setMessage(message: string) {
    if (message) console.info("[Flash]", message);
    this.callbacks.onMessageChange?.(message);
  }

  private setProgress(progress: number) {
    this.callbacks.onProgressChange?.(progress);
  }

  private setError(error: number) {
    this.error = error;
    this.callbacks.onErrorChange?.(error);
    this.setProgress(-1);
  }

  async initialize(manifest: ManifestEntry[]) {
    this.setProgress(-1);
    this.setMessage("");

    if (typeof navigator.usb === "undefined") {
      this.setError(ErrorCode.REQUIREMENTS_NOT_MET);
      return;
    }

    try {
      await this.imageManager.init();
    } catch (err: any) {
      console.error("[Flash] Failed to initialize image manager:", err);
      if (err?.message?.startsWith("Not enough storage")) {
        this.setError(ErrorCode.STORAGE_SPACE);
        this.setMessage(err.message);
      } else {
        this.setError(ErrorCode.UNKNOWN);
      }
      return;
    }

    this.manifest = manifest;
    console.info("[Flash] Loaded manifest:", this.manifest.length, "entries");
    this.setStep(Step.READY);
  }

  private async connect() {
    this.setStep(Step.CONNECTING);
    this.setProgress(-1);

    try {
      await this.device.connect(new usbClass());
    } catch (err: any) {
      if (err.name === "NotFoundError") {
        this.setStep(Step.READY);
        return;
      }
      console.error("[Flash] Connection error:", err);
      this.setError(ErrorCode.LOST_CONNECTION);
      return;
    }

    console.info("[Flash] Connected");
    this.callbacks.onConnectionChange?.(true);

    try {
      const storageInfo = await this.device.getStorageInfo();
      const serial = Number(storageInfo.serial_num).toString(16).padStart(8, "0");
      this.callbacks.onSerialChange?.(serial);
      console.info("[Flash] Serial:", serial);
    } catch (err) {
      console.warn("[Flash] Could not read storage info:", err);
    }
  }

  private async repairPartitionTables() {
    this.setStep(Step.REPAIR_PARTITION_TABLES);
    this.setProgress(0);

    const gptImages = this.manifest!.filter((e) => !!e.gpt);
    if (gptImages.length === 0) {
      console.error("[Flash] No GPT images found");
      this.setError(ErrorCode.REPAIR_FAILED);
      return;
    }

    try {
      for (const [image, onProgress] of withProgress(gptImages, this.setProgress.bind(this))) {
        const [onDownload, onRepair] = createSteps([2, 1], onProgress);
        this.setMessage(`Downloading ${image.name}`);
        await this.imageManager.downloadImage(image, onDownload);
        const blob = await this.imageManager.getImage(image);
        this.setMessage(`Repairing GPT LUN ${image.gpt!.lun}`);
        if (!(await this.device.repairGpt(image.gpt!.lun, blob))) {
          throw new Error(`Repairing LUN ${image.gpt!.lun} failed`);
        }
        onRepair(1.0);
      }
    } catch (err) {
      console.error("[Flash] Partition table repair failed:", err);
      this.setError(ErrorCode.REPAIR_FAILED);
    }
  }

  private async eraseDevice() {
    this.setStep(Step.ERASE_DEVICE);
    this.setProgress(-1);

    const luns = Array.from({ length: 6 }, (_, i) => i);

    const [found, persistLun, partition] = await this.device.detectPartition("persist");
    if (!found || luns.indexOf(persistLun) < 0) {
      console.error("[Flash] Could not find persist partition");
      this.setError(ErrorCode.ERASE_FAILED);
      return;
    }

    try {
      const critical = ["mbr", "gpt"];
      for (const lun of luns) {
        const preserve = [...critical];
        if (lun === persistLun) preserve.push("persist");
        this.setMessage(`Erasing LUN ${lun}`);
        if (!(await this.device.eraseLun(lun, preserve))) {
          throw new Error(`Erasing LUN ${lun} failed`);
        }
      }
    } catch (err) {
      console.error("[Flash] Erase failed:", err);
      this.setError(ErrorCode.ERASE_FAILED);
    }
  }

  private async flashSystem() {
    this.setStep(Step.FLASH_SYSTEM);
    this.setProgress(0);

    // Flash everything except GPTs and persist
    const systemImages = this.manifest!.filter((e) => !e.gpt && e.name !== "persist");

    try {
      for await (const [image, onImageProgress] of withProgress(
        systemImages,
        this.setProgress.bind(this),
        (img) => img.size,
      )) {
        const [onDownload, onFlash] = createSteps(
          [1, image.has_ab ? 2 : 1],
          onImageProgress,
        );

        this.setMessage(`Downloading ${image.name}`);
        await this.imageManager.downloadImage(image, onDownload);
        const blob = await this.imageManager.getImage(image);
        onDownload(1.0);

        const slots = image.has_ab ? ["_a", "_b"] : [""];
        for (const [slot, onSlotProgress] of withProgress(slots, onFlash)) {
          const partitionName = `${image.name}${slot}`;
          this.setMessage(`Flashing ${partitionName}`);
          if (
            !(await this.device.flashBlob(
              partitionName,
              blob,
              (progress: number) => onSlotProgress(progress / image.size),
              false,
            ))
          ) {
            throw new Error(`Flashing ${partitionName} failed`);
          }
          onSlotProgress(1.0);
        }
      }
    } catch (err) {
      console.error("[Flash] Flash failed:", err);
      this.setError(ErrorCode.FLASH_FAILED);
    }
  }

  private async finalize() {
    this.setStep(Step.FINALIZING);
    this.setProgress(-1);
    this.setMessage("Setting active slot");

    if (!(await this.device.setActiveSlot("a"))) {
      this.setError(ErrorCode.UNKNOWN);
      return;
    }

    this.setMessage("Rebooting");
    await this.device.reset();
    this.callbacks.onConnectionChange?.(false);
    this.setStep(Step.DONE);
  }

  async start() {
    if (this.step !== Step.READY) return;

    await this.connect();
    if (this.step === Step.READY || this.error !== ErrorCode.NONE) return;

    await this.repairPartitionTables();
    if (this.error !== ErrorCode.NONE) return;

    await this.eraseDevice();
    if (this.error !== ErrorCode.NONE) return;

    await this.flashSystem();
    if (this.error !== ErrorCode.NONE) return;

    await this.finalize();
  }
}

export async function loadProgrammer(): Promise<ArrayBuffer> {
  const res = await fetch(PROGRAMMER_URL);
  if (!res.ok) throw new Error(`Failed to fetch programmer: ${res.status}`);
  return res.arrayBuffer();
}
