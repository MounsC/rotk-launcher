import { spawn, type ChildProcess } from "node:child_process";
import { existsSync } from "node:fs";

export function deathcommEndpoint(origin: string): string {
  const url = new URL(origin);
  if (url.username || url.password || (url.protocol !== "https:" &&
      !(url.protocol === "http:" && ["localhost", "127.0.0.1", "[::1]"].includes(url.hostname))))
    throw new Error("Deathcomm requires HTTPS or a loopback test gateway");
  url.protocol = url.protocol === "https:" ? "wss:" : "ws:";
  url.pathname = "/voice/v1/deathcomm"; url.search = ""; url.hash = "";
  return url.toString();
}

/** The game owns the helper's lifetime; bearer credentials never enter its argv. */
export function startDeathcommClient(input: {
  executable: string; gamePid: number; gameRoot: string; voiceOrigin: string; ticket: string;
}): () => void {
  if (!existsSync(input.executable)) return () => {};
  let child: ChildProcess | undefined;
  try {
    const server = deathcommEndpoint(input.voiceOrigin);
    child = spawn(input.executable, [], { windowsHide: true, shell: false, stdio: ["pipe", "ignore", "ignore"] });
    child.on("error", () => {});
    child.stdin?.on("error", () => {});
    child.stdin?.end(JSON.stringify({ GamePid: input.gamePid, GameRoot: input.gameRoot, Server: server, Token: input.ticket }) + "\n");
    child.unref();
  } catch { /* Optional audio never blocks launch. */ }
  return () => { if (child && child.exitCode === null && !child.killed) child.kill(); };
}
