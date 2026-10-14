const { createWriteStream } = require("fs");

const UNITS = { B: 1, KB: 1024, MB: 1024 ** 2, GB: 1024 ** 3 };
const CHUNK_SIZE = 1024 ** 2;

function parseArgs(argv) {
  const args = {};
  for (const arg of argv) {
    const [key, ...rest] = arg.split("=");
    args[key] = rest.join("=");
  }
  return args;
}

function parseSize(str) {
  const match = /^(\d+(?:\.\d+)?)\s*(B|KB|MB|GB)?$/i.exec(str ?? "");
  if (!match) return null;
  const unit = (match[2] ?? "B").toUpperCase();
  return Math.floor(parseFloat(match[1]) * UNITS[unit]);
}

async function main() {
  const { filename, size: sizeArg } = parseArgs(process.argv.slice(2));
  const size = parseSize(sizeArg);

  if (!filename || size === null) {
    console.error("usage: node create-input filename=<filename> size=<size>");
    console.error("  size examples: 100, 512B, 64KB, 500MB, 1GB");
    process.exit(1);
  }

  const chunk = Buffer.alloc(CHUNK_SIZE, "a");
  const stream = createWriteStream(filename);
  let remaining = size;

  while (remaining > 0) {
    const n = Math.min(CHUNK_SIZE, remaining);
    remaining -= n;
    if (!stream.write(chunk.subarray(0, n))) {
      await new Promise((resolve) => stream.once("drain", resolve));
    }
  }

  await new Promise((resolve, reject) => {
    stream.on("error", reject);
    stream.end(resolve);
  });

  console.log(`wrote ${size} bytes to ${filename}`);
}

main().catch((err) => {
  console.error(err);
  process.exit(1);
});
