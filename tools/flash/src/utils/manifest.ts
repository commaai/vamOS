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
  gpt?: {
    lun: number;
    start_sector: number;
    num_sectors: number;
  };
}

export async function getManifest(url: string): Promise<ManifestEntry[]> {
  const response = await fetch(url);
  if (!response.ok) throw new Error(`Failed to fetch manifest: ${response.status}`);
  return response.json();
}

/**
 * Get the manifest URL for the latest vamOS release.
 * Uses the GitHub API (CORS-safe) to find the latest release tag,
 * then returns the manifest.json asset URL.
 */
export async function getLatestManifestUrl(): Promise<{ tag: string; manifestUrl: string }> {
  const res = await fetch("https://api.github.com/repos/commaai/vamOS/releases/latest");
  if (!res.ok) throw new Error(`No releases found: ${res.status}`);
  const { tag_name, assets } = await res.json();
  const manifestAsset = assets.find((a: any) => a.name === "manifest.json");
  if (!manifestAsset) throw new Error("manifest.json not found in release assets");
  return { tag: tag_name, manifestUrl: manifestAsset.browser_download_url };
}
