import { PassThrough } from "node:stream";
import { EventEmitter } from "node:events";
import { beforeEach, expect, test, vi } from "vitest";
const mocks = vi.hoisted(() => ({ spawn: vi.fn(), exists: vi.fn(() => true) }));
vi.mock("node:child_process", () => ({ spawn: mocks.spawn }));
vi.mock("node:fs", () => ({ existsSync: mocks.exists }));
import { deathcommEndpoint, startDeathcommClient } from "../electron/services/deathcomm-client.js";
beforeEach(() => { mocks.spawn.mockReset(); mocks.exists.mockReturnValue(true); });
test("regional WSS endpoint, loopback development, no plaintext public audio", () => {
  expect(deathcommEndpoint("https://voice.example/old?profile=a#unused")).toBe("wss://voice.example/voice/v1/deathcomm");
  expect(deathcommEndpoint("http://127.0.0.1:8080")).toBe("ws://127.0.0.1:8080/voice/v1/deathcomm");
  for (const url of ["http://remote.example", "ftp://voice.example", "https://user:password@voice.example"])
    expect(() => deathcommEndpoint(url)).toThrow();
});
test("secret over stdin, hidden helper and game-lifetime teardown", () => {
  const child = Object.assign(new EventEmitter(), { stdin: new PassThrough(), unref: vi.fn(), kill: vi.fn(), exitCode: null, killed: false });
  let input = ""; child.stdin.on("data", (chunk: Buffer) => { input += chunk.toString(); });
  mocks.spawn.mockReturnValue(child);
  const stop = startDeathcommClient({ executable: "helper.exe", gamePid: 12, gameRoot: "fixture", voiceOrigin: "https://voice.example", ticket: "private-ticket" });
  expect(mocks.spawn).toHaveBeenCalledWith("helper.exe", [], expect.objectContaining({ windowsHide: true, shell: false }));
  expect(JSON.parse(input)).toEqual({ GamePid: 12, GameRoot: "fixture", Server: "wss://voice.example/voice/v1/deathcomm", Token: "private-ticket" });
  expect(child.unref).toHaveBeenCalledOnce(); stop(); expect(child.kill).toHaveBeenCalledOnce();
});
test("an unavailable optional helper does not prevent a game launch", () => {
  mocks.exists.mockReturnValue(false);
  expect(() => startDeathcommClient({ executable: "missing", gamePid: 12, gameRoot: "fixture", voiceOrigin: "https://voice.example", ticket: "private-ticket" })()).not.toThrow();
  expect(mocks.spawn).not.toHaveBeenCalled();
});
