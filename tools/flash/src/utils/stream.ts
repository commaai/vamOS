type ProgressCallback = (progress: number) => void;

const getContentLength = (response: Response): number => {
  const total = response.headers.get("Content-Length");
  if (total) return parseInt(total, 10);
  throw new Error("Content-Length not found in response headers");
};

interface FetchStreamOptions {
  maxRetries?: number;
  retryDelay?: number;
  onProgress?: ProgressCallback;
}

export async function fetchStream(
  url: string | URL,
  requestOptions: RequestInit = {},
  options: FetchStreamOptions = {},
): Promise<ReadableStream<Uint8Array>> {
  const maxRetries = options.maxRetries || 3;
  const retryDelay = options.retryDelay || 1000;

  const fetchRange = async (startByte: number, signal: AbortSignal) => {
    const headers: Record<string, string> = {
      ...(requestOptions.headers as Record<string, string> || {}),
    };
    if (startByte > 0) headers["range"] = `bytes=${startByte}-`;
    const response = await fetch(url, { ...requestOptions, headers, signal });
    if (!response.ok || (response.status !== 206 && response.status !== 200)) {
      throw new Error(`Fetch error: ${response.status}`);
    }
    return response;
  };

  const abortController = new AbortController();
  let startByte = 0;
  let contentLength: number | null = null;

  return new ReadableStream({
    async pull(stream) {
      for (let attempt = 0; attempt <= maxRetries; attempt++) {
        try {
          const response = await fetchRange(startByte, abortController.signal);
          if (contentLength === null) contentLength = getContentLength(response);
          const reader = response.body!.getReader();
          while (true) {
            const { done, value } = await reader.read();
            if (done) { stream.close(); return; }
            startByte += value.byteLength;
            stream.enqueue(value);
            options.onProgress?.(startByte / contentLength!);
          }
        } catch (err) {
          console.warn(`Attempt ${attempt + 1} failed:`, err);
          if (attempt === maxRetries) {
            abortController.abort();
            stream.error(new Error("Max retries reached", { cause: err as Error }));
            return;
          }
          await new Promise((res) => setTimeout(res, retryDelay));
        }
      }
    },
    cancel(reason) {
      console.warn("Stream canceled:", reason);
      abortController.abort();
    },
  });
}
