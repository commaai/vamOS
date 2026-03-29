// Swap this to Azure CDN URL for production
export const PROXY_BASE = "https://vamos-release-proxy.vamos-release-proxy.workers.dev";

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

/**
 * Fetch the latest vamOS release tag via the GitHub API (CORS-safe),
 * then fetch the manifest through the CORS proxy and rewrite all
 * image URLs to go through the proxy as well.
 */
export async function getManifest(): Promise<{ tag: string; manifest: ManifestEntry[] }> {
  // GitHub API has CORS - use it to get the latest release tag
  const res = await fetch("https://api.github.com/repos/commaai/vamOS/releases/latest");
  if (!res.ok) throw new Error(`No releases found: ${res.status}`);
  const { tag_name } = await res.json();

  // Fetch manifest through proxy
  const manifestRes = await fetch(`${PROXY_BASE}/${tag_name}/manifest.json`);
  if (!manifestRes.ok) throw new Error(`Failed to fetch manifest: ${manifestRes.status}`);
  const manifest: ManifestEntry[] = await manifestRes.json();

  // Rewrite image URLs to go through proxy
  for (const entry of manifest) {
    const filename = entry.url.split("/").pop();
    entry.url = `${PROXY_BASE}/${tag_name}/${filename}`;
  }

  return { tag: tag_name, manifest };
}
