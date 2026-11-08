# AI prompt — `vuesp` firmware and device contract

Read [ecosystem-master-context.md](./ecosystem-master-context.md) first. Work only on this firmware repository and the device-contract portion of the Vuesp ecosystem.

## Objective

Plan a backward-compatible device contract and firmware evolution path that allows `vuesp-hub` to discover devices, display generic entities, validate commands, and later provide normalized data to local integrations and opt-in cloud telemetry.

## Scope

- Inspect the current firmware, target boards, discovery, WebSocket messages, identity/configuration storage, commands, embedded UI, and current tests/build configuration.
- Propose a versioned device protocol/manifest and compatibility strategy for existing Vuesp firmware.
- Define stable project/device identifiers, firmware version/board/capability reporting, entity semantics, command schemas, error/ACK behavior, and manifest validation.
- Preserve UDP discovery and binary WebSocket as the initial transport; keep the contract transport-independent.
- Define persistent UUID lifecycle: survives firmware updates and ordinary settings resets; only an explicit identity reset changes it; full NVS erase creates a new UUID.
- Keep hardware/chip IDs as local diagnostic/migration data, not public identity. Do not design cloud transmission of hardware IDs.
- Specify legacy handling: current legacy commands and embedded UI remain available; do not guess field meanings for MQTT/cloud mappings.
- Define command semantics: request ID and ACK/error/timeout; no offline command queue or replay; same-request-ID retries only for idempotent set commands; no automatic retry for buttons/impulse actions.
- Include board-specific firmware bundle information needed by the site flasher (board target, offsets, required images), without implementing the flasher.
- Keep OTA out of the MVP.

## Approved hub-device WebSocket/dashboard direction

- The hub initiates the WebSocket connection after the user enrolls a device. Saved devices reconnect automatically with backoff.
- For the new protocol, use versioned JSON control frames on the same WebSocket for handshake/device description, manifest and dashboard configuration, and command ACK/error responses. Keep binary frames for state/telemetry where appropriate.
- The versioned device description provides a `schemaId`/version for the binary `struct` decoder over WebSocket; the hub uses a built-in registry of supported decoders. Do not require a separate HTTP fetch for protocol metadata. New binary layouts require a decoder added to the hub; never guess or decode an unknown schema.
- Connection sequence: the hub sends `HELLO` with supported protocol versions; the device responds with its description (identity, firmware/board, struct/decoder schema version, capabilities); the hub then requests manifest and dashboard separately, validates them, and requests an initial state snapshot. Generic commands remain disabled until manifest validation succeeds. Define exact message names and payload schemas jointly with the hub.
- `struct` describes binary encoding and decoding only. The versioned manifest is the source of truth for stable entity IDs, entity types, and typed commands.
- Dashboard configuration is declarative presentation: ordering, labels, widget type/options, and references to manifest entity/command IDs. It must not redefine entity meaning or command semantics.
- Treat configuration received from a device as untrusted data. Never evaluate embedded JavaScript or import executable modules from the device.
- Preserve released binary-protocol firmware through explicit legacy adapters. Map only known legacy fields/commands; unsupported or unmapped fields remain read-only or unavailable. An unknown manifest major version keeps the device visible but disables generic commands and integrations.
- The wire format and semantics of new generic command requests remain open and must be designed jointly with the hub; do not infer them from the legacy protocol.
- New binary layouts require coordinated firmware and hub releases that register the corresponding `schemaId`/decoder.

## Out of scope

- Implementing the hub, website, account system, cloud backend, MQTT broker, or web flasher.
- Breaking changes to released firmware without an explicit migration path.
- Adding transport encryption/authentication as an unapproved MVP change. The accepted LAN trust limitation must be documented; identify future hardening separately.

## Workflow and deliverables

1. Inspect the repository and summarize current protocol/firmware behavior with file references.
2. Identify compatibility risks, unknowns, and relevant existing tests/builds.
3. Propose the contract, manifest shape, protocol/schema versioning, migration/legacy strategy, and an implementation sequence with measurable acceptance criteria.
4. List unresolved choices separately; ask before deciding anything that changes compatibility, security, or hardware support.
5. Stop before editing code and wait for explicit implementation approval.

After approval, implement only the approved stages, preserve existing worktree changes, add/update focused tests and documentation, and report exact validation results.
