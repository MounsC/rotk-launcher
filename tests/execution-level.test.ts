import { readFile } from "node:fs/promises";
import { expect, it } from "vitest";

interface PackageManifest {
  build: {
    win: { requestedExecutionLevel?: string };
    nsis: { perMachine?: boolean };
  };
}

/**
 * Launcher 2.0.14 shipped with `requireAdministrator` and did not open on
 * part of the player base (issue #59): from an elevated parent, Chromium could
 * not create its sandboxed GPU and renderer children ("GPU process launch
 * failed: error_code=18", "Renderer process launch-failed") and aborted the
 * process. Nothing in-app can catch that abort, so the launcher runs as the
 * invoking user; the TPM steps that need elevation are the job of a one-shot
 * helper, not of the whole process. Flipping this back is a release decision,
 * and this test makes it a visible one.
 */
it("runs the launcher as the invoking user, never elevated as a whole", async () => {
  const manifest = JSON.parse(await readFile(new URL("../package.json", import.meta.url), "utf8")) as PackageManifest;
  expect(manifest.build.win.requestedExecutionLevel).toBe("asInvoker");
  // The per-machine install stays: one copy under Program Files, updated by
  // the assisted installer that asks for elevation itself.
  expect(manifest.build.nsis.perMachine).toBe(true);
});
