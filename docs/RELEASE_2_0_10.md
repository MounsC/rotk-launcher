# ROTK Launcher 2.0.10

The native voice HUD preserves server-provided ranks instead of replacing them
with a visible actor's zero or stale rank. Public Solo and its lobby receive
seasonal ranks; unranked modes retain no rank badge. This release includes the
reviewed asset packaging recipes for the rank menu, settings, staff-only panel
and ROTK administrator voice badge, including launcher PRs #42 and #43.

Publish only after the server explicitly accepts launcher 2.0.10 and its
version-specific game-file attestation root. Existing accepted launcher builds
remain supported during this rollout. Asset pack 1.8.0 distributes the staff
artwork separately; merging the recipe does not install packs.

The tag workflow rebuilds the native components, runs the complete tests,
packages the Windows installer and publishes a draft with checksums and GitHub
provenance. Verify these artifacts before publishing the stable release.
The administrator badge remains controlled by the server setting documented
in `staff-voice-assets.md`.
