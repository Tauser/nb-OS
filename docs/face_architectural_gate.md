# Gate Arquitetural Obrigatorio — Face Engine Premium++

## Objetivo

Congelar contratos minimos do subsistema facial antes de expandir geometria (`F1`) e gaze/saccades (`F2`), sem reescrever o engine atual.

## Leitura do estado atual

### Como os modulos conversam hoje

- `BehaviorService`, `RoutineService`, `GazeService`, `GestureService`, `MotionSyncService` e `ActionOrchestratorService` acionam face via `IFaceController::requestExpression(...)`.
- `FaceService` consome estado emocional via `IEmotionProvider`.
- `FaceService` consome estado fisico de pescoco via `IMotionStateProvider`.
- `FaceService` aplica preset/layers e envia geometria final para `FaceRenderer`.
- `FaceRenderer` apenas desenha (`IDisplayPort::drawEye(...)`) e faz dirty rect/culling.

### Ponto forte

- pipeline atual ja e em camadas (base, mood, attention, neck, idle, transient, blink) e renderer nao arbitra estado.

### Lacunas arquiteturais

- ownership de camadas estava implicito no `FaceService`.
- nao havia contrato formal de estado facial semantico para consumo externo.
- fronteira entre estado base, modulacao, reacao transitoria e clip temporal nao estava formalizada em modelo.

## Contrato minimo congelado

Arquivo principal:

- `src/models/face_render_state.h`

### Ownership explicito por camada

- `base expression` -> `BaseExpressionController`
- `gaze` -> `GazeController`
- `blink` -> `BlinkController`
- `idle` -> `IdleController`
- `mood` -> `MoodController`
- `transient reaction` -> `TransientReactionController`
- `clip` -> `ClipPlayer`

### Distincoes semanticas obrigatorias

- `estado base`: intencao principal de expressao + prioridade + hold/lock
- `modulacao`: ajustes continuos de baixa/medio prioridade (gaze, idle, mood, blink)
- `reacao transitoria`: resposta curta a eventos, com duracao limitada
- `clip temporal`: sequencia temporal autoriazada que pode ocupar camadas por janela definida

## Mudancas implementadas (incrementais e seguras)

### Novos arquivos

- `src/models/face_render_state.h`
- `src/interfaces/i_face_render_state_provider.h`

### Arquivos alterados

- `src/services/face/face_service.h`
- `src/services/face/face_service.cpp`

### O que foi alterado no FaceService

- passa a implementar `IFaceRenderStateProvider`.
- passa a manter snapshot semantico `FaceRenderState`.
- adiciona sincronizacao incremental do snapshot no fluxo ja existente (`init/update`), sem alterar API externa de expressao.
- renderer continua sem arbitragem.

## Compatibilidade e restricoes

- nao houve quebra em `IFaceController`.
- nao houve mudanca de contrato do renderer.
- nao houve migracao de arbitragem para renderer.
- nao houve reescrita do engine atual.

## Baseline para F1 e F2

- `F1 (Geometry V2)` passa a ter contrato semantico fixo para acoplar nova geometria sem inflar `FaceService` com estado opaco.
- `F2 (Gaze/Saccades)` passa a ter camada semantica formal (`FaceGazeState`) para evolucao de alvo, modo e fase temporal.

## Sequencia minima de commits recomendada

1. `face(gate): adicionar FaceRenderState semantico e ownership explicito`
2. `face(gate): expor snapshot via IFaceRenderStateProvider no FaceService`
3. `docs(face): registrar gate arquitetural e baseline para F1/F2`
