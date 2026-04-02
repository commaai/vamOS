const gitSha = Bun.spawnSync(["git", "rev-parse", "HEAD"]).stdout.toString().trim();

const build = await Bun.build({
  entrypoints: ["./src/index.html"],
  outdir: "./dist",
  sourcemap: "linked",
  minify: true,
  define: {
    "process.env.GIT_SHA": JSON.stringify(gitSha),
  },
});

if (!build.success) {
  for (const log of build.logs) console.error(log);
  process.exit(1);
}

const outputs = build.outputs.map(({ path, kind, size }) => ({
  path,
  kind,
  "size (KiB)": (size / 1024).toFixed(1),
}));
console.table(outputs);
