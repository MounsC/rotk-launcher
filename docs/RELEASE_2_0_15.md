# ROTK Launcher 2.0.15

Hotfix for 2.0.14, which did not open on part of the player base (issue #59).

2.0.14 was the first published build to request administrator rights
(`requireAdministrator`, introduced by 2.0.12 for the TPM anchor). Chromium
starts its GPU, renderer and utility processes as new instances of the
launcher executable under restricted sandbox tokens, so that requirement
applied to every child. On sessions that run with a full administrator token
without UAC's split — the built-in `Administrator` account, common on
preinstalled or ghosted Windows — Windows refused to create them: `GPU process
launch failed: error_code=18`, `Renderer process launch-failed`, then the abort
`GPU process isn't usable. Goodbye.` (exception `0x80000003` in the WER report).
Players saw either a process that died at once or three processes and no
window. `--no-sandbox` confirmed it (the children then run with the parent's
token); the install location is not involved (a copy under `%LOCALAPPDATA%`
fails the same way, the `Program Files` ACLs are the defaults). 2.0.11,
`asInvoker`, ran on those same sessions and worked. Nothing in the application
can catch that abort.

Changes:

- The launcher runs as the invoking user again (`asInvoker`). The per-machine
  install stays; the assisted installer asks for elevation itself when
  updating. The TPM anchor keeps its unelevated behaviour (no EK certificate,
  activation refused, server observes); the elevated steps will move to a
  one-shot helper.
- The window is created before the startup steps that can stall and is shown
  on the first of `ready-to-show`, `did-finish-load` or a five-second
  deadline. A renderer that cannot start is reported in a dialog instead of
  leaving a process without a window.
- A second launcher no longer runs `initialize()` while quitting. When the
  window was closed with the game running, launching again brings it back.
- Startup breadcrumbs in `%APPDATA%\ROTK Launcher\startup.log` (previous run
  in `startup.previous.log`), including child processes that end abnormally
  with Chromium's reason.

Rollout:

- Players on a broken 2.0.14 cannot receive this through the in-app updater:
  the process dies before the update check runs. Post the installer link.
- 2.0.11 is admitted by the production server and is the interim fallback;
  its updater offers 2.0.14 but installs nothing on its own.
- Admit 2.0.15 in the server's launcher version policy (with its attestation
  root) before publishing the release, as for every launcher build.
- A machine that failed on 2.0.14 is the acceptance test: 2.0.15 must open
  there without `__COMPAT_LAYER=RunAsInvoker` or `--no-sandbox`.
