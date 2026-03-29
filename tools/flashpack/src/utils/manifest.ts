const MANIFEST_URL = "https://raw.githubusercontent.com/commaai/vamOS/release-images/manifest.json";

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

interface Manifest {
  version: string;
  images: ManifestEntry[];
}

export async function getManifest(): Promise<Manifest> {
  const res = await fetch(MANIFEST_URL);
  if (!res.ok) throw new Error(`Failed to fetch manifest: ${res.status}`);
  return res.json();
}
