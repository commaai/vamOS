const build = await Bun.build({
  entrypoints: ["./src/index.html"],
  outdir: "./dist",
  sourcemap: "linked",
  minify: true,
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
