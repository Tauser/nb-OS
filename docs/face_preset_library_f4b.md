# Face Premium++ F4B - Official Preset Library

## Status

- library state: frozen
- exploration flag: off for all official presets
- canonical preset count: 10

## Official Presets

- neutral_premium (Tier C)
- curious_soft (Tier C)
- sleepy_sink (Tier A)
- warm_happy (Tier C)
- focused_listen (Tier B)
- shy_peek (Tier B)
- surprised_open (Tier A)
- low_energy_flat (Tier A)
- attention_lock (Tier A)
- thinking_side (Tier B)

## Readability Tiers

- Tier A: immediate readability under modulation (surprised_open, sleepy_sink, low_energy_flat, attention_lock)
- Tier B: medium readability with context (focused_listen, shy_peek, thinking_side)
- Tier C: subtle readability for baseline personality (neutral_premium, curious_soft, warm_happy)

## Preset Contracts

- each preset has layer blend policy (`mood`, `attention`, `gaze`, `idle`, `transient`, `clip`, `blink` gains)
- each preset has tuning guardrails (width/height/roundness/openness/lids ranges)
- each preset has usage profile (`role`, `recommendedHoldMs`, `prefersAttentionLock`, `expectsClipSupport`, `tuningNotes`)
- presets do not require expression-specific hacks in `FaceService`

## Visual Stability Checklist

- [ ] silhouette remains readable after blink + gaze + clip overlap
- [ ] no preset enters flicker loop under transition cooldown
- [ ] Tier A presets remain recognizable in < 300 ms
- [ ] Tier B presets remain recognizable with moderate mood modulation
- [ ] Tier C presets preserve personality without visual noise
- [ ] low_energy_flat keeps slow cadence and reduced blink amplitude
- [ ] attention_lock keeps stable gaze under active voice/listening states
- [ ] shy_peek and thinking_side keep asymmetry without clipping
- [ ] warm_happy and neutral_premium do not collapse into same silhouette
- [ ] all presets return cleanly to baseline after clip/transient end
