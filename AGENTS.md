# AGENTS.md

## Repository

This repository is **Gores Client / BestClient**, a C++ DDNet client fork.

Primary goals for agent changes:
- preserve gameplay correctness and existing client feel;
- keep changes small and reviewable;
- avoid unrelated refactors;
- prefer fixing the verified root cause over adding parallel systems.

## Default workflow

Before editing:
1. Verify the current hosted `main` branch and record its SHA.
2. Start work from the current hosted `main` unless the task explicitly names another base.
3. Inspect the relevant existing implementation before proposing a replacement.
4. Check recent related changes when the task touches prediction, input, rendering, or menu architecture.

During editing:
- Make one focused change per task/PR.
- Do not modify unrelated files just for cleanup.
- Do not rename existing configs or change their defaults unless explicitly requested.
- Reuse existing architecture and helpers where practical.
- Prefer narrow fixes over broad rewrites.
- Preserve comments that document intentional behavior.
- Add comments only where the reasoning is non-obvious.

After editing:
1. Run `clang-format` on touched C/C++ files.
2. Run `git diff --check`.
3. Build the strongest relevant client target available in the environment.
4. Run relevant tests when practical.
5. Inspect the final diff for unrelated changes.
6. Report:
   - hosted base SHA;
   - branch name;
   - commit SHA;
   - changed files;
   - verified root cause;
   - behavior before/after;
   - tests/builds performed;
   - remaining manual/Windows checks.

Do not merge a PR unless the user explicitly asks for a merge.

## Build and test

Initialize submodules when needed:

```bash
git submodule update --init --recursive
```

Preferred Ninja debug configure:

```bash
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug -DDEV=ON -DVULKAN=ON
```

Build:

```bash
cmake --build build --target everything
```

Tests:

```bash
cmake --build build --target run_tests
```

If Vulkan or optional system libraries are unavailable, use the strongest practical fallback and report the limitation instead of weakening the code change silently. A common client-focused fallback is:

```bash
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug -DDEV=ON -DVULKAN=OFF
cmake --build build --target game-client
```

Do not treat missing external development packages as a source-code failure. Distinguish environment failures from compile/test failures caused by the patch.

## Gameplay and input changes

Gameplay/input changes require extra conservatism.

Unless the task explicitly asks otherwise:
- preserve normal A/D behavior;
- preserve jump, hook, fire, weapon switching, mouse/subtick behavior, and packet format;
- do not change server/gamecore physics;
- do not change tuning constants;
- do not change PREINPUT protocol semantics;
- do not add velocity extrapolation as a visual workaround;
- do not add local-player smoothing that changes input feel;
- do not change Smart Stop / Snap Tap behavior in unrelated tasks;
- do not change shotgun, dragger, pickup, hook, or hammer behavior unless directly required.

For gameplay/input PRs, a **Windows gameplay A/B test is required before merge**. Codex should report this as still required when it cannot perform that test itself.

## Gores input architecture

`BC_INPUTS_GORES` is input mode `7`.

Treat the current Gores prediction/render architecture as intentional unless the task explicitly requires changing it.

Preserve these invariants:
- generation-tagged exact Gores prediction history;
- exact tick validation before consuming ring-buffer samples;
- one selected display timeline per rendered tee;
- body position and predicted render metadata must come from the same resolved Gores sample/timeline;
- freeze state, hook state, weapon/attack metadata, angle, velocity, and position must not be mixed from different horizons;
- active interaction members must remain temporally coherent where the current shared-horizon policy requires it;
- remote reconciliation must remain functional;
- speculative prediction may continue internally even when display policy becomes more conservative;
- fallback from unavailable/stale exact samples must be atomic rather than partially mixing timelines.

Relevant code is primarily in:
- `src/game/client/gameclient.cpp`
- `src/game/client/gameclient.h`
- `src/game/client/components/players.cpp`
- `src/game/client/components/controls.cpp`
- `src/game/client/prediction/`

When fixing a Gores visual issue, first determine whether the fault is:
- prediction state;
- horizon selection;
- exact-sample validation;
- display sample selection;
- metadata consumption;
- interaction-group/shared-horizon policy;
- reconciliation.

Do not introduce a second independent Gores rendering pipeline unless there is no smaller correct solution.

## Freeze and anti-ping behavior

Existing freeze-related configs have established semantics. Preserve them unless the task explicitly requests a redesign.

In particular:
- `tc_remove_anti` is not equivalent to globally disabling prediction;
- respect the existing `tc_remove_anti_ticks` and `tc_remove_anti_delay_ticks` behavior;
- do not blindly route Gores through legacy render-history paths if that would bypass generation-tagged exact Gores samples;
- when adapting a legacy behavior to Gores, prefer deriving the intended timeline and then resolving an exact Gores sample from that timeline.

For speculative freeze handling:
- distinguish speculative future freeze from confirmed regular/current freeze;
- avoid displaying a speculative frozen state merely because a future prediction touched freeze if the regular timeline has not confirmed it;
- preserve as much safe positive display horizon as possible rather than collapsing to zero without evidence;
- verify exact tick/intra boundary behavior before assuming a "safe horizon" is strictly pre-transition.

## UI and menu scope

Do not touch menu/UI code during gameplay, input, prediction, or freeze tasks unless the task explicitly requires UI changes.

Relevant menu files include:
- `src/game/client/components/menus.cpp`
- `src/game/client/components/menus.h`
- `src/game/client/components/menus_start.cpp`
- `src/game/client/components/menus_start.h`
- `src/game/client/components/menus_settings.cpp`
- `src/game/client/components/menus_bestclient.cpp`

Avoid mixing visual redesigns with gameplay fixes.

## Config changes

Config variables live primarily in:
- `src/engine/shared/config_variables_bestclient.h`
- `src/engine/shared/config_variables_tclient.h`

Rules:
- do not rename configs without explicit instruction;
- do not alter defaults without explicit instruction;
- preserve backward compatibility with existing config files where practical;
- if adding a config, keep its scope narrow and document why it is necessary.

## Git and PR hygiene

- Base focused work on current hosted `main`.
- Keep commits focused.
- Do not include generated build artifacts.
- Do not include formatting churn in unrelated lines/files.
- Do not modify an unrelated open PR branch as the base for a new gameplay task.
- If the hosted `main` moved since the prompt was written, use the actual hosted base and report the change.
- Create a PR only when requested by the task.
- Never merge unless explicitly instructed.

## Debugging expectations

When a bug report describes a visual symptom:
1. Reproduce or trace the exact code path first.
2. Identify the first point where predicted state diverges from intended displayed state.
3. Prefer adding temporary targeted diagnostics over guessing.
4. Remove noisy temporary logging before the final commit.
5. Reuse existing debug infrastructure such as Gores debug metrics where appropriate.

Do not claim a root cause is confirmed unless the code path or a reproducible test supports it.

## Scope fallback

If a requested fix turns out to require a much larger architecture change:
- do not silently expand a focused task into a broad refactor;
- implement the smallest independently correct portion if possible;
- report clearly what remains and why it should be a separate PR.
