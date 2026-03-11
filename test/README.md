# Trilho de Protecao - Fase 6

Este diretorio contem a infraestrutura minima para reduzir regressao durante refatoracoes estruturais.

## Objetivos

- detectar regressao silenciosa cedo
- reduzir risco em arbitragem/eventos/estado agregado
- manter baseline reproduzivel no simulador

## Estrutura

- `test_unit_emotion_state/`: teste unitario de normalizacao do `EmotionState`
- `test_unit_homeostasis_state/`: teste unitario de clamp do `HomeostasisState`
- `test_unit_face_geometry/`: teste unitario de modos, cant e override por olho da Geometry V2
- `test_unit_face_gaze/`: teste unitario de saccade/fixation/micro-saccade/edge-peek da F2
- `test_unit_face_compositor/`: teste unitario de arbitragem/hold/cooldown entre base, blink, gaze, transiente e clip (F5)
- `test_unit_face_clip_player/`: teste unitario de preempcao/cancelamento/recovery do player de clips longos (F3)
- `test_unit_face_preset_library/`: teste unitario de congelamento/tiers/guardrails da biblioteca oficial de presets (F4B)
- `test_integration_event_bus/`: teste de integracao basico do `EventBus`
- `golden/`: scripts e baselines dos cenarios (`idle`, `touch`, `voice`, `stress`) e preparo de silhueta (`face_silhouette`)
- `scripts/verify_golden.ps1`: verificador de baseline por log

## Feature flag

- `FF_TEST_GATES_ENFORCED`
- objetivo: permitir desligar o trilho de validacao sem impactar runtime do produto
- em simulacao (`esp32s3_sim`) o flag esta habilitado no `platformio.ini`

## Como rodar a suite minima

1. Rodar build dos testes unitarios + integracao:

```powershell
pio test -e esp32s3_sim --without-uploading --without-testing
```

> Nota: neste ambiente SIM o `pio test -e esp32s3_sim` completo pode tentar etapa de upload e falhar mesmo com build dos testes OK.

2. Validar baseline de um cenario com log capturado:

```powershell
pwsh -NoProfile -File test/scripts/verify_golden.ps1 -Scenario idle -LogFile C:\temp\idle.log
pwsh -NoProfile -File test/scripts/verify_golden.ps1 -Scenario touch -LogFile C:\temp\touch.log
pwsh -NoProfile -File test/scripts/verify_golden.ps1 -Scenario voice -LogFile C:\temp\voice.log
pwsh -NoProfile -File test/scripts/verify_golden.ps1 -Scenario stress -LogFile C:\temp\stress.log
```

## Como usar os golden scripts

1. Abra o simulador.
2. Execute comandos do arquivo `test/golden/<cenario>.script.txt` no monitor serial.
3. Salve a saida do monitor em arquivo `.log`.
4. Rode `verify_golden.ps1` contra o baseline correspondente.

## Silhouette tests (F1)

- script base: `test/golden/face_silhouette.script.txt`
- manifesto de keyframes: `test/golden/face_silhouette_keyframes.md`
- objetivo: validar legibilidade de silhueta sem contexto textual

## Criterio de rollback

Se houver qualquer interferencia no loop principal/build normal, desligar por:

- remover `-DFF_TEST_GATES_ENFORCED=1` do ambiente alvo
- manter testes apenas no pipeline de validacao




