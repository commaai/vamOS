import index from "./src/index.html";

const server = Bun.serve({
  static: {
    "/": index,
  },
  development: true,
  fetch() {
    return new Response("404!");
  },
});
console.info(`Running on http://${server.hostname}:${server.port}`);
