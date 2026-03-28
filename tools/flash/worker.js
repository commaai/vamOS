// Cloudflare Worker: CORS proxy for GitHub release assets
// Deploy: npx wrangler deploy worker.js --name vamos-release-proxy

const ALLOWED_REPO = "commaai/vamOS";

export default {
  async fetch(request) {
    if (request.method === "OPTIONS") {
      return new Response(null, { headers: corsHeaders() });
    }

    const url = new URL(request.url);
    const path = url.pathname.slice(1);

    if (!path) {
      return new Response("Usage: /{tag}/{filename}\nExample: /v17.2/manifest.json", {
        headers: { "content-type": "text/plain", ...corsHeaders() },
      });
    }

    const ghUrl = `https://github.com/${ALLOWED_REPO}/releases/download/${path}`;
    const resp = await fetch(ghUrl, { redirect: "follow" });

    if (!resp.ok) {
      return new Response(`Not found: ${path}`, { status: resp.status, headers: corsHeaders() });
    }

    return new Response(resp.body, {
      status: 200,
      headers: {
        "content-type": resp.headers.get("content-type") || "application/octet-stream",
        "content-length": resp.headers.get("content-length"),
        ...corsHeaders(),
      },
    });
  },
};

function corsHeaders() {
  return {
    "access-control-allow-origin": "*",
    "access-control-allow-methods": "GET, OPTIONS",
    "access-control-allow-headers": "Content-Type",
  };
}
