import { fetchStream } from "./stream";
import type { ManifestEntry } from "./manifest";

type ProgressCallback = (progress: number) => void;

const MIN_QUOTA_GB = 3;

export class ImageManager {
  root: FileSystemDirectoryHandle | null = null;

  async init() {
    if (!this.root) {
      this.root = await navigator.storage.getDirectory();
      try {
        await (this.root as any).remove({ recursive: true });
      } catch (_) {}
      this.root = await navigator.storage.getDirectory();
      console.info("[ImageManager] Initialized");
    }

    const estimate = await navigator.storage.estimate();
    const quotaGB = (estimate.quota || 0) / 1024 ** 3;
    if (quotaGB < MIN_QUOTA_GB) {
      throw new Error(
        `Not enough storage: ${quotaGB.toFixed(1)}GB free, need ${MIN_QUOTA_GB.toFixed(1)}GB`,
      );
    }
  }

  async downloadImage(image: ManifestEntry, onProgress?: ProgressCallback) {
    const fileName = `${image.name}-${image.hash_raw}.img`;
    const fileHandle = await this.root!.getFileHandle(fileName, { create: true });
    const writable = await fileHandle.createWritable();

    try {
      if (image.chunks && image.chunks.length > 0) {
        let bytesDownloaded = 0;
        for (const chunk of image.chunks) {
          console.debug(`[ImageManager] Downloading chunk ${chunk.url}`);
          const stream = await fetchStream(chunk.url, { mode: "cors" }, {
            onProgress: (chunkProgress) => {
              onProgress?.((bytesDownloaded + chunkProgress * chunk.size) / image.size);
            },
          });
          const reader = stream.getReader();
          while (true) {
            const { done, value } = await reader.read();
            if (done) break;
            await writable.write(value);
          }
          bytesDownloaded += chunk.size;
        }
        await writable.close();
      } else {
        console.debug(`[ImageManager] Downloading ${image.name} from ${image.url}`);
        const stream = await fetchStream(image.url, { mode: "cors" }, { onProgress });
        await stream.pipeTo(writable);
      }
      onProgress?.(1);
    } catch (e) {
      throw new Error(`Error downloading ${image.name}: ${e}`, { cause: e as Error });
    }
  }

  async getImage(image: ManifestEntry): Promise<Blob> {
    const fileName = `${image.name}-${image.hash_raw}.img`;
    const fileHandle = await this.root!.getFileHandle(fileName, { create: false });
    return fileHandle.getFile();
  }
}
