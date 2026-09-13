# Fidelity to reference (VC_Clouds) — audit

Several things have drifted from the verified reference since we started troubleshooting. Breaking it down.

## Verified, faithfully ported (unchanged)

- Full density formula (`Remap` chains, height-gradient shape/detail altering, dual Henyey-Greenstein, Beer's law atten, ambient occlusion, transmittance/light-energy compositing) — from `IanovDensityFunction.ush` (string-extracted `CustomExpression1`).
- `RayMaxSteps=500` origin, step-back-on-first-hit retry, double-step-on-miss, self-shadow march structure — from `IanovRaymarch.ush` (verbatim, braced main loop).
- Distance-from-camera semantics for `StepDistanceScale`/`MipMapDistanceScale` (`length(RayPos.xy - CamPos.xy)`) — preserved through the box-local refactor via `RayPos - LocalRayOrigin`.
- `BaseNoiseUVW`/`SmallNoiseUVW` confirmed as a **single scalar tile divisor applied uniformly to all 3 axes** of `RayPos` (`frac(RayPos / BaseNoiseTexTile + Wind)`) — matches our structure, not a per-axis split.
- Per-pixel start jitter (`RayPos += Jitter * RayDir` using `Rand3DPCG16(int3(SvPosition.xy, View.StateFrameIndexMod8))`) — confirmed from `IanovRaymarch.ush` and now ported (was missing entirely before).
- `DepthCheck < 0` scene-depth break condition — confirmed real in the reference; intentionally omitted here (no scene depth wired).
- `BaseNoiseTexTile = BoxSizeX / NoiseTile`, `SmallNoiseTexTile = BoxSizeX / SmallNoiseTile` — traced directly in the main material graph (`NoiseTile` param → `Divide` by 1 → `Multiply` by `(ActorScale.x * 50 * 2)`, same pattern as the already-confirmed `WeatherTexTile`). With our defaults (`CloudsVolume.x=4,000,000`, `NoiseTile=20`, `SmallNoiseTile=230`) this gives `200,000` / `17,391` — which is exactly what the shader started with before any of the guessing. The cone-field artifact was never a tile-scale problem; it was the camera-relative sampling bug and the unclamped mip runaway, both fixed separately since.
- `LightDirection` verified as `(0.492453, -0.086837, 0.865996)` from the actual material instance's live parameter values — replaces the earlier hardcoded guess.
- Shadow-march step formula traced verbatim: `ShadowStep = normalize(LightDirection) * RayOffset * LightStepScale`, where `RayOffset = (ActorScale.x * 50) / RayMaxSteps` — a **fixed** reference length (box half-extent-X divided by step count), not tied to the actual per-ray traversal distance. Ported as `normalize(LightDir) * (Extent.x / RayMaxSteps) * LightStepScale`, replacing the earlier `length(RayDir)`-based guess.
- `AtmosphereFogColor` traced to a `SceneColor` (Input Data) node — it samples the actual scene/background color, not a constant. Confirms aerial-perspective-style fog (fades into whatever is actually behind the clouds). **Not yet implemented** — requires a scene-color copy before our pass writes into the same render target (a real architecture addition, not a one-line hardcode); still using the placeholder constant pending a decision on scope.

## Deviations introduced, not from the reference

- `RayMaxSteps`: 500 → **1500** (tuning for our much larger effective view distance, not a verified value).
- `MipMapDistanceScale` clamped to `min(..., 5.0)` — the reference has no clamp; ours needed one because our 40km box massively exceeds whatever box scale `MipMapScaleDistance=500000` was originally tuned for.
- Noise/density sampling uses **box-local position**, not world-space — necessary because of our camera-relative double-float precision system (the reference has no such system, it just uses plain world position). Functionally equivalent for a static box, but architecturally different.

## Real bug found and fixed: compositing was opaque, not translucent

The material graph after the raymarch does `Mask(A) → 1-x → Opacity`, i.e. the real material's alpha is `1 - Transmittance`, feeding a proper translucent blend so the actual scene shows through where there's no cloud. Our screen-pass was using `FDefaultBlendState` (`TStaticBlendState<>`, defaults to `BF_One`/`BF_Zero`) — a fully **opaque overwrite** that ignores alpha entirely. What looked like "sky" in the gaps between clouds was actually our own synthesized `ShadowColor`/`AtmosphereFogColor` output painted opaquely over the real scene, not the real background. Fixed:
- `.usf`: box-miss early-out now outputs `float4(0,0,0,0)` (fully transparent) instead of `(0,0,0,1)`; final output alpha is now `1.0 - Transmittance` (matches the material's own `1-x` node) instead of raw `Transmittance`.
- `HorizonCloudsViewExtension.cpp`: blend state changed from `FDefaultBlendState` to `TStaticBlendState<CW_RGBA, BO_Add, BF_SourceAlpha, BF_InverseSourceAlpha, BO_Add, BF_Zero, BF_One>` — real straight-alpha blending, destination alpha preserved. **Requires a C++ recompile**, not a shader hot-reload.

## Still-unverified placeholders, unresolved

- `AtmosphereFogColor` — real formula found (see above), but not yet implemented; still a hardcoded constant pending the scene-color-copy work.

## Bottom line

Box intersection, our own step count (1500 vs reference's 500), and the mip clamp are the only remaining intentional deviations, all justified by our much larger box scale. Density formula, tile-scale derivation, main-loop control flow, start jitter, light direction, and the shadow-step formula are all now confirmed faithful ports. Only atmosphere fog color's scene-sampling is identified-but-unimplemented.
