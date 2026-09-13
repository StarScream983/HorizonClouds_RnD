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

## Wind speed exposed to ImGui

Split into two separate tunables instead of one merged scalar, so the verified reference value stays distinguishable from the guessed one:

- `r.HorizonClouds.WindSpeed` (default `12.0`) — the verified reference `WindSpeed` value.
- `r.HorizonClouds.TimeScale` (default `0.001`) — unverified Time-to-UV conversion scalar (reference's exact `Time` semantics/`.cpp` Tick implementation were never read).

Both are `SHADER_PARAMETER(float, ...)` on `FHorizonCloudsPS::FParameters`, with sliders in the "HORIZON CLOUDS" ImGui window (`HorizonCloudsCVars.h` holds the extern declarations so both `HorizonCloudsViewExtension.cpp` and `HorizonCloudsImGui.cpp` can reference them). Wind = `normalize(0.25, 0, -0.05) * WindSpeed * TimeScale * View.GameTime`. Wind _direction_ stays hardcoded at the verified value — not exposed, since it isn't in question. Requires a C++ recompile.

## Still-unverified placeholders, unresolved

- `AtmosphereFogColor` — real formula found (see above), but not yet implemented; still a hardcoded constant pending the scene-color-copy work.

## Bottom line

Box intersection, our own step count (1500 vs reference's 500), and the mip clamp are the only remaining intentional deviations, all justified by our much larger box scale. Density formula, tile-scale derivation, main-loop control flow, start jitter, light direction, and the shadow-step formula are all now confirmed faithful ports. Only atmosphere fog color's scene-sampling is identified-but-unimplemented.

---

## MIPMAP DISTANCE THRESHOLDS

Three distinct visual zones observed at a grazing viewing angle across the box, each caused by a different distance-based mechanism compounding:

**Close range (sharp detail):** `StepDistanceScale` is ~0 (inside `StepScaleDistance=200,000`/2km), so the primary ray advances at its base step size — fine-grained density sampling. `MipMapDistanceScale` is 0 (full-res texture). Both mechanisms are at their sharpest, so shape and surface detail are both correct.

**Mid range (looks like smooth iso-blobs with detailed surface):** Past 2km, `StepDistanceScale = clamp((dist-200000)/200000, 0, 10)` ramps up and saturates at **10x** by ~4km — so `RayDir` (the per-iteration step vector) becomes up to 11x longer. Density/opacity is now accumulated over much coarser intervals along the ray: the _macro_ silhouette gets smoothed into rounded, isosurface-like blobs because there aren't enough samples per unit depth to resolve fine density gradients at the edges anymore. Meanwhile `MipMapDistanceScale` is likely still low in the nearer part of this band (under the 5km `MipMapScaleDistance` threshold), so each individual sample still reads sharp, undegraded noise-texture values — hence detailed surface bumps sitting on top of an over-smoothed blob shape. Two separate mechanisms, two different frequencies, out of sync with each other.

**Far range (blown out, hard to read):** Both mechanisms are now fully saturated (`StepDistanceScale=10` max, `MipMapDistanceScale` clamped at our `5.0` cap), so texture detail is essentially gone. On top of that, at this near-horizontal grazing angle the ray travels an enormous distance _through_ the 2km-thick cloud layer (nearly parallel to it), so `Transmittance` collapses toward 0 very quickly — a mostly-opaque, low-variation white mass regardless of texture detail, just from sheer accumulated optical depth.

All three mechanisms (`StepDistanceScale`, `MipMapDistanceScale`, and our added `min(...,5.0)` clamp) are either verified-faithful to the reference or a documented deviation — this banding is inherent to the technique at this box scale and viewing angle, not a new bug. Open question: accept it (faithful) or soften the transitions (deviation) — same fork as the earlier LOD-threshold discussion, now also affecting the primary step size.

---

## DIFFERENCES FROM HZD PIPELINE

Sharing lineage but diverging significantly in execution. VC_Clouds is clearly built on the published HZD equations (Schneider's 2015 GDC talk) — the density formula shape (Perlin-Worley base shape + Worley erosion, height-gradient/cloud-type shaping, dual Henyey-Greenstein phase, Beer's law, in/out-scattering, ambient-occlusion-ish terms) is the same lineage we've verified in `SampleDensity`. But the actual production techniques diverge in several concrete ways:

- **Stepping — the biggest difference.** HZD's actual technique is genuine coarse→fine adaptive marching: take large, cheap steps sampling _only_ the low-frequency base shape (no erosion) until density is first detected, then switch to small, fine steps and start sampling full detail for accurate lighting. VC*Clouds does **not** do this — we traced it exactly: it's a fixed per-ray step size (chosen once) that only toggles between 1x (in cloud) and 2x (in empty space), never refining below the base step, and it samples the \_same* density function (full formula) regardless of whether it's "searching" or "inside." The LOD reduction in VC_Clouds is tied purely to camera distance (`floor(dist/MipMapScaleDistance)`), not to the coarse/fine search state like HZD's — exactly why we get the 3-zone distance banding: HZD's LOD is content-adaptive, VC_Clouds' is just distance-adaptive.
- **Weather generation.** HZD simulates an evolving weather map procedurally across an open world (wind-driven, gameplay-relevant). VC_Clouds uses one static painted 2D texture (`T_WeatherMap`), UV-tiled across a fixed box — no simulation.
- **Performance architecture.** HZD renders at reduced/checkerboard resolution with temporal reprojection to hit frame budget at open-world scale. VC*Clouds renders full-resolution every frame with a hard step cap and no temporal accumulation at all — its own actor description literally admits *"major optimisation issues."\_ This absence of temporal amortization is why it leans so hard on the distance-based step/LOD hacks to survive at range.
- **Wind/motion.** HZD uses curl-noise-driven distortion for wisping/turbulence. VC_Clouds' wind is a plain linear UV offset (`Position/Tile + Wind`) — no curl noise texture exists anywhere in its parameter list. (Notably, `OrbisClouds` already has a `CurlNoiseFBM` texture staged on disk, so that's a real point of divergence to keep in mind for the planet-scale project.)
- **Scope/scale.** HZD is an unbounded atmospheric layer wrapping the whole game world/horizon. VC_Clouds is a bounded local AABB "prop" (hence needing a hard box intersection and `ActorMin`/`ActorMax` clamps at all) — architecturally it's a local volumetric object, not a planet-scale system.

**Bottom line:** same equations, much simpler (and less optimized) execution — VC_Clouds is a marketplace-scale approximation of HZD's math without HZD's adaptive-sampling or performance architecture.

---

## ISOVOLUME IN THE DISTANCE

**The grey flatten-out** is fully by design, conceptually — this is `AtmosphereBlendDistance` (verified at 5km/500,000 units) doing exactly what it's meant to: fading the cloud into an "atmospheric fog" color as distance increases, so far clouds don't render with full sharp contrast forever (real aerial perspective does this too). The math saturates to 100% fog color once you're roughly 7.5km from the point where the ray first entered the cloud — past that, the output is _entirely_ the fog color, no lighting contribution survives at all. The reason it looks like a flat, dead grey rather than a natural haze is that our `AtmosphereFogColor` is still the hardcoded placeholder we flagged earlier — the real reference samples the actual scene/sky color there instead of a fixed constant, so a real implementation would fade into whatever's actually behind it rather than snapping to one flat tone. This is squarely the still-unimplemented item from the DEVLOG.

**The iso-volume look** (before the grey kicks in) comes from the same two mechanisms behind the earlier three-zone banding: past `StepScaleDistance` (2km) the primary ray's step size balloons up to 11x, and past `MipMapScaleDistance` (5km) the noise texture degrades toward a flat blurred mip. With both coarsened, there just isn't enough sampling resolution left to resolve local density/lighting variation — so most of what you see is dominated by whatever shading got computed at the very first few (very large) steps into the cloud, which is fairly uniform across a wide swath of screen at that distance. That reads as a flat-shaded blob rather than something with internal light scattering, because internal light scattering literally isn't being sampled anymore at that point.

**So: yes, controllable.** The two levers are `StepScaleDistance`/`MipMapScaleDistance` (push these farther out to keep real shading detail alive longer before it degrades) and implementing the real scene-sampled `AtmosphereFogColor` (so the far fade looks like haze instead of a flat wall of grey). Neither requires touching the core density/lighting math — both are the same distance-threshold knobs already in the shader.

---

## HORIZON BAND ARTIFACT — RESOLVED

Bug: standing inside the cloud, looking dead-level, showed a thin horizontal band.

Ruled out (via debug-mode visualizations): constant `HeightGradient` on level rays, `Transmittance` not saturating, `AtmosphereFogColor`/fog blend, step-length-weighted `Transmittance` compensation.

Root cause: box is thin in Z, wide in XY, so a level ray's `tFar` spikes far higher than a pitched ray's. That inflates the per-step distance right at the horizon past the noise tile's feature size — spatial aliasing, not an integration bug.

Fix: decouple step size from `RayMaxSteps`. Use a fixed physical step size, let step _count_ vary instead:

```hlsl
const float FixedStepSize = 4000000.0 / 512.0;
const float RayLength = tFar - tNear;
const float BaseStepSize = (RayLength >= FixedStepSize * 2.0) ? FixedStepSize : ((RayLength / 10.0));
float3 RayDir = RayDirection * BaseStepSize;
```

`RayMaxSteps` now just caps total iterations — long horizon rays stop tracing early instead of aliasing (fine, since that region is a thin screen-space strip anyway). Our own addition, not from the reference. Generalizes directly to planet-scale spherical shells.

---

## LIGHTING WALKTHROUGH, LINE BY LINE

Walking through it block by block, in order:

**Shadow march (lines 194–203)** — how much cloud is between this sample and the sun:
```hlsl
float DensityToSun = 0.0;
float3 ShadowRayPos = RayPos;
const float3 ShadowStep = normalize(LightDir) * (Extent.x / RayMaxSteps) * LightStepScale;
for (int s = 0; s < ShadowMaxSteps; s++)
{
    ShadowRayPos += ShadowStep;
    DensityToSun += SampleDensity(ShadowRayPos, floor(s/2), ...);
}
```
From the current point, step 8 times *toward the sun* and sum the density along that mini-path. `DensityToSun` is the accumulated "optical thickness" between here and the light source — more cloud in the way = more sun blocked. Each step uses a coarser mip (`floor(s/2)`) since precision matters less farther into the shadow march.

**Sun attenuation (207–210)** — turn that optical thickness into a 0–1 attenuation factor:
```hlsl
AttenPrim = exp(-BeerLawDensity * DensityToSun);
```
This is literally **Beer's Law**: light through a medium falls off exponentially with the amount of material in the way. `AttenPrim` is "how much direct sunlight reaches this point."
```hlsl
AttenSec = exp(-BeerLawDensity * AttenClampIntensity) * 0.7;
SunAtten = Remap(DotLight, 0, 1, AttenSec, AttenSec*0.5);
Atten = max(SunAtten, AttenPrim);
```
`AttenSec` is a *floor* value — light never goes fully to zero, there's always a small ambient contribution regardless of shadow depth (real clouds aren't pitch black inside). `SunAtten` interpolates that floor based on `DotLight` (angle between view and sun direction). `Atten` takes whichever is *brighter* of the two — the real Beer's-law falloff, or the ambient floor.

**"Powder"/depth-based ambient occlusion (212–214)**:
```hlsl
Depth = CloudOutScatterAmbient * pow(DensitySample, Remap(HeightGradient, 0.3, 0.9, 0.5, 1.0));
Vertical = pow(saturate(Remap(HeightGradient, 0, 0.3, 0.8, 1.0)), 0.8);
AmbientOutScatter = 1.0 - saturate(Depth * Vertical);
```
This approximates multiple light scattering inside dense cloud without actually simulating it — the classic "powder sugar" trick. Thicker, lower-altitude cloud (`HeightGradient` near the base) darkens more (`AmbientOutScatter` shrinks toward 0); thin/high cloud stays brighter. It's a cheap fake for how light bounces around inside a dense medium rather than passing straight through.

**Phase function — how light scatters relative to view angle (216–220)**, this is the **dual Henyey-Greenstein** part:
```hlsl
FirstHG  = InScatterIntensity * HG(InScatter, DotLight);
SecondHG = SilverLightIntensity * pow(saturate(DotLight), SilverLightExp);
InScatterHG = max(FirstHG, SecondHG);
OutScatterHG = HG(OutScatter, DotLight);
SunHighlight = lerp(InScatterHG, OutScatterHG, InOutScatterLerp);
```
`InScatter`/`OutScatter` model forward-scattering (looking toward the sun through cloud, get a bright glow — think looking at clouds near the sun) vs. back-scattering. `SecondHG` is the **silver lining** effect — a sharp bright rim exactly where you're looking almost straight at the sun through the cloud edge. `SunHighlight` blends both scattering directions into one value.

**Putting it together, accumulating front-to-back (222–224)**:
```hlsl
Light = Atten * AmbientOutScatter * SunHighlight * LightIntensity * DensitySample * Transmittance;
LightEnergy += Light;
Transmittance *= (1.0 - DensitySample);
```
This step's contribution = (how much sun reaches here) × (ambient darkening) × (scattering brightness) × (density here) × (how much light has *already* been blocked by cloud in front of it — `Transmittance`). Classic front-to-back volume compositing: each new sample matters less as `Transmittance` shrinks, since earlier, closer cloud has already blocked most of the light.

**After the loop — final grade (254–256)**:
```hlsl
LightEnergy = pow(max(lerp(ShadowColor, LightColor, LightEnergy), 0), LightPow);
```
`LightEnergy` (an unbounded accumulator, not a 0–1 color) is used as a **blend factor** between a dark `ShadowColor` and a bright `LightColor` — not added as raw light. Then `pow(..., LightPow)` is pure stylization (contrast/gamma push), not physically motivated.
```hlsl
AtmosphereBlendLerp = saturate(((dist_to_first_hit / AtmosphereBlendDistance) - 0.5) * AtmosphereBlendIntensity);
LightEnergy = lerp(LightEnergy, AtmosphereFogColor, AtmosphereBlendLerp);
```
Fades distant cloud into an atmosphere/haze color — aerial perspective. `OutColor.a = 1 - Transmittance` is the final opacity.

So the whole pipeline is: shadow-march → Beer's law attenuation with an ambient floor → powder-style depth darkening → dual-HG phase/silver-lining → front-to-back composite → stylized color grade → distance fog. All standard HZD-lineage cloud lighting, nothing exotic.

---
