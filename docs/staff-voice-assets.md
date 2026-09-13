# ROTK staff voice badge asset release

The moderator extension adds the same ROTK crowned skull as the administrator,
with a bright green (`#28ff18`) fading strip, ivory outline and dark shadow.
This recipe packages that extension against the published assets-v1.8.1 catalog.
It preserves the administrator badge, rank menu, settings panels, staff UI gate
and every unrelated catalog entry.

The native voice-rank fix in launcher [#42](https://github.com/MzKaxD/rotk-launcher/pull/42)
is also required. This packaging change does not itself install anything or
publish a release. No proprietary client assets or simulated participants are
stored in the repository.

## Reconstruct the reviewed candidate

Use Java 17, FFDec 26.2.1, Python 3 and Node 24.12.0 for the validated pack
transforms. Work in a separate staging directory, never on installed/hardlinked
packs. `SOURCE_CANDIDATE` identifies the three assets-v1.8.1 input packs by
size and SHA-256. Inputs must match before transforming them.

1. Extract `UIRoot.gfx` and `HudGroupVoiceWindow.gfx` from the published main
   pack with the server's `devts/tools/pack2-workbench.mjs`. The server builder
   pins these inputs and FFDec before writing a new output directory:

   ```sh
   node devts/tools/build-moderator-voice-client1315.mjs <java> <ffdec.jar> <python> <UIRoot.gfx> <HudGroupVoiceWindow.gfx> <new-output-dir>
   ```

2. Assemble the three final packs with the server workbench. Every step writes
   a separate intermediate output, and the workbench checks unaffected entries.

   | Pack | Operations in order |
   | --- | --- |
   | `assets_x64_0.pack2` | Replace UIRoot; replace HudGroupVoiceWindow |
   | `ui_x64_0.pack2` | Replace UIRoot; replace HudGroupVoiceWindow |
   | `ui_x64_2.pack2` | Preserve the published settings/Top Ten pack byte for byte |

   Use `replace <input> --asset <name> --file <new-file> --out <new-pack>`.
   The moderator reuses the already-published `rotk_staff_badge_32.dds` texture.

3. Compare all three final packs with `CANDIDATE` in
   `scripts/prepare-staff-voice-assets.mjs`. The package command enforces those
   exact sizes/hashes. Never substitute the visual harness's root, which injects
   simulated voices. Changed inputs or tool outputs require fresh validation.

## Prepare and verify

Fetch the current full `feed.json` and `asset-payloads.v1.json` from
`h1z1rotk/assets`. Choose a version strictly newer than that feed; 1.9.0 below
is an example, not a reserved release number.

```sh
npm run assets:prepare-staff-voice -- <reviewed-packs-dir> <current-feed.json> <current-payloads.json> <new-output-dir> 1.9.0
npm run test:assets:staff-voice
```

The command validates ownership and source pins before creating output. It
streams archive creation and full decompression/readback, then writes
`assets_x64_0.payload`, `rank_menu_ui.payload`, the two complete manifests and
`verification.json`. Existing files/versions outside those two archives remain
unchanged. It refuses stale sources, ownership conflicts and an existing output
directory. Keep `.payload` suffixes so upload alone cannot activate a partial
update through the launcher's automatic ZIP discovery.

Set `ROTK_STAFF_VOICE_RELEASE_PROOF` to that output directory and rerun the test
to exercise the actual archives through production AssetSyncService in a
temporary client. It verifies installation, repeat sync without downloading,
same-size corruption repair from cache and restoration of synthetic originals.
Allow roughly 8 GB of additional temporary space with the candidate/archives
already prepared. Tests without the variable use tiny synthetic packs.

## Maintainer rollout

1. Keep the native voice-rank fix shipped in launcher 2.0.10 (PR #42).
2. Upload both attachments and publish the matching full manifests together in
   the assets repository. Recheck its current feed before publication.
3. Refresh the server's normal asset/proxy attestation allowlist for that release.
4. Deploy the server moderator-badge extension, retaining
   `H1Z1_ROTK_STAFF_VOICE_BADGE=1` on public lobby/Solo processes.
5. Verify a real two-client microphone session with admin, moderator and player.

The server uses the authenticated role independently of moderator duty: admins
receive voice tier 9/0, moderators 10/0. The tenth frame requires these new client
assets; assets-v1.8.1 clients hide it. Seasonal rank, killfeed and permissions are
unchanged. Disable the staff setting and restart the affected roles to restore
seasonal voice badges, or roll back the two coordinated asset archives and
manifests together. Do not revert only one UI pack.
