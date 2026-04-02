const REPO = "commaai/vamOS";
const IMAGES_REPO = "commaai/vamos-images";
const VERSION_URL = `https://raw.githubusercontent.com/${REPO}/master/userspace/root/VERSION`;

export interface ChunkInfo {
  url: string;
  size: number;
}

export interface ManifestEntry {
  name: string;
  url: string;
  hash: string;
  hash_raw: string;
  size: number;
  sparse: boolean;
  full_check: boolean;
  has_ab: boolean;
  ondevice_hash: string;
  chunks?: ChunkInfo[];
  gpt?: {
    lun: number;
    start_sector: number;
    num_sectors: number;
  };
}

export async function getManifest(): Promise<ManifestEntry[]> {
  const versionRes = await fetch(VERSION_URL);
  if (!versionRes.ok) throw new Error(`Failed to fetch version: ${versionRes.status}`);
  const version = (await versionRes.text()).trim();

  const manifestUrl = `https://raw.githubusercontent.com/${IMAGES_REPO}/v${version}/manifest.json`;
  const res = await fetch(manifestUrl);
  if (!res.ok) throw new Error(`Failed to fetch manifest: ${res.status}`);
  return res.json();
}
