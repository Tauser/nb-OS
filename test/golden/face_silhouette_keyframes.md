# Face Silhouette Keyframes (F4A - Exploration)

Objetivo: validar legibilidade de silhueta da biblioteca exploratoria F4A sem congelar ainda a library final.

## Estado da biblioteca

- modo: exploracao
- frozen: nao
- foco: contraste e assinatura visual por tier

## Presets exploratorios

- Tier A (leitura imediata): `surprised_open`, `sleepy_sink`, `attention_lock`
- Tier B (leitura media): `focused_listen`, `thinking_side`, `shy_peek`
- Tier C (leitura sutil): `neutral_premium`, `curious_soft`, `warm_happy`
- suporte complementar: `low_energy_flat`

## Captura recomendada (SIM)

1. `monitor on`
2. capturar Tier C:
   - `face neutral` -> `neutral_premium`
   - `face curious` -> `curious_soft`
   - `face happy` -> `warm_happy`
3. capturar Tier B:
   - `face angry` -> `focused_listen`
   - `face thinking` -> `thinking_side`
   - `face shy` -> `shy_peek`
4. capturar Tier A:
   - `face surprised` -> `surprised_open`
   - `face sad` -> `sleepy_sink`
   - `face listening` -> `attention_lock`
5. capturar `face alert` -> `low_energy_flat`

## Regras de captura

- capturar sem overlay externo
- manter mesmo crop/zoom
- manter mesmo brilho
- salvar em `test/golden/face_silhouette/`
- registrar timestamp de build e preset ativo

## Checklist de legibilidade

- [ ] Tier A reconhecivel imediatamente sem contexto
- [ ] Tier B distinguivel entre si sem colapso visual
- [ ] Tier C com identidade consistente sem exagero
- [ ] `surprised_open` != `attention_lock` de forma obvia
- [ ] `sleepy_sink` != `low_energy_flat` de forma obvia
- [ ] sem flicker perceptivel na transicao curta entre presets

## Observacao de fase

Esta validacao aprova exploracao de linguagem visual. Nao congela parametros finais da biblioteca.
