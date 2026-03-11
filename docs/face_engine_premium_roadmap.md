# Face Engine Premium ++ - Roadmap Oficial

## Objetivo geral

Levar o Face Engine atual para um nivel de personagem premium, com:

- identidade visual forte
- geometria muito mais rica
- movimentos longos na tela
- saccades convincentes
- presets memoraveis
- composicao visual governada
- coerencia forte com emocao, atencao e neck motion

## Pre-condicao (gate arquitetural)

Antes de iniciar F1/F2, manter congelado o contrato minimo em:

- `docs/face_architectural_gate.md`
- `src/models/face_render_state.h`
- `src/interfaces/i_face_render_state_provider.h`
## Principios

- evoluir em cima do engine atual, sem reescrever tudo
- manter `FaceService` como fachada, nao como deposito de logica
- separar geometria, gaze, clips e composicao
- preservar compatibilidade com `Emotion`, `Behavior`, `Routine` e `Motion`
- priorizar legibilidade visual e linguagem de personagem

## Ordem oficial recomendada

1. `F1 - Face Geometry V2`
2. `F2 - Gaze & Saccade System`
3. `F4 - Premium Preset Library`
4. `F5 - Face Composition & Arbitration`
5. `F3 - Long Motion Clips`
6. `F6 - Multimodal Sync & Tooling`

## Logica da ordem

- primeiro amplia o vocabulario visual
- depois da intencao ao olhar
- depois cria presets fortes
- depois governa as camadas
- depois adiciona clips ricos
- por fim fecha a experiencia com sincronizacao e tooling

## Criterios premium congelados

- `Legibilidade`: cada preset principal deve ser reconhecivel pela silhueta
- `Direcao de olhar`: o usuario deve perceber para onde o robo esta olhando sem depender do pescoco
- `Sustentacao`: o idle visual deve continuar interessante depois de varios minutos
- `Coerencia`: blink, gaze, preset e clip devem parecer do mesmo personagem
- `Estabilidade`: transicoes nao podem gerar flicker nem disputa de camada
- `Integrabilidade`: o sistema deve continuar compativel com `Emotion`, `Behavior`, `Routine` e `Neck Motion`

## Arquitetura alvo

Arquivos principais esperados:

- `src/models/face_shape_profile.*`
- `src/models/face_gaze_target.*`
- `src/models/face_clip.*`
- `src/services/face/face_geometry.*`
- `src/services/face/face_gaze_controller.*`
- `src/services/face/face_clip_player.*`
- `src/services/face/face_compositor.*`
- `src/services/face/face_render_state.*`
- `src/services/face/expression_preset.*`
- `src/services/face/face_service.*`
- `src/services/face/face_renderer.*`
- `tools/face_preset_editor/*`

Regra principal:

- `FaceService` orquestra
- `FaceGeometry` define forma
- `FaceGazeController` move o olhar
- `FaceClipPlayer` executa animacoes assinatura
- `FaceCompositor` combina camadas
- `FaceRenderer` desenha

---

## F1 - Face Geometry V2

### Objetivo

Expandir o vocabulario visual dos olhos para sair do modelo limitado de retangulos arredondados escalados.

### Funcao desta fase

- aumentar a variedade real de silhuetas
- permitir assimetria controlada por olho
- preparar base para presets premium e gaze mais expressivo

### Escopo

- `src/models/face_shape_profile.*`
- `src/services/face/face_geometry.*`
- evolucao de `src/services/face/eye_model.h`
- ajustes em `src/services/face/face_renderer.*`

### O que deve ser feito

1. criar `shapeMode` por olho
2. suportar modos minimos:
   - `rounded_rect`
   - `wedge`
   - `trapezoid`
   - `soft_capsule`
3. adicionar cant interno e externo
4. adicionar largura de topo e base
5. adicionar roundness por canto ou por pares
6. permitir override por olho esquerdo e direito
7. preservar compatibilidade com o pipeline atual durante a transicao

### Criterio de aceite

- uma expressao muda de silhueta de forma clara
- olhos podem ser simetricos ou levemente assimetricos
- presets deixam de parecer apenas variacoes do mesmo bloco

### Criterio complementar obrigatorio

- executar `silhouette tests`
- congelar frames-chave
- validar se os estados principais sao reconheciveis sem contexto

---

## F2 - Gaze & Saccade System

### Objetivo

Fazer o olhar parecer intencional, vivo e legivel.

### Funcao desta fase

- dar direcao real ao olhar
- introduzir saccades curtas, medias e longas
- sustentar atencao visual sem parecer ruido aleatorio

### Escopo

- `src/models/face_gaze_target.*`
- `src/services/face/face_gaze_controller.*`
- integracao com `src/services/gaze/gaze_service.*`
- integracao com `src/services/face/face_service.*`

### O que deve ser feito

1. criar alvo de olhar em coordenadas normalizadas
2. implementar saccades curtas, medias e longas
3. implementar overshoot e settle
4. implementar fixation com duracao controlada
5. adicionar micro-saccades para estados como `listening` e `thinking`
6. adicionar `edge peek`, `recovery glance` e pequenos scans
7. manter o gaze desacoplado da logica de comportamento

### Criterio de aceite

- o usuario percebe para onde o robo esta olhando
- o idle visual nao parece preso no centro
- saccades nao parecem jitter nem ruido mecanico

---

## F4 - Premium Preset Library

### Objetivo

Substituir presets apenas funcionais por presets assinatura.

### Funcao desta fase

- definir a linguagem visual do personagem
- formalizar presets reutilizaveis e contrastantes
- tornar a leitura do rosto independente de logs

### Escopo

- `src/services/face/expression_preset.*`
- integracao com `src/services/face/face_service.*`

### Presets minimos recomendados

- `neutral_premium`
- `curious_soft`
- `sleepy_sink`
- `warm_happy`
- `focused_listen`
- `thinking_side`
- `shy_peek`
- `surprised_open`
- `low_energy_flat`
- `attention_lock`

### Readability tiers

- `Tier A - leitura imediata`
- `surprised_open`
- `sleepy_sink`
- `attention_lock`
- `Tier B - leitura media`
- `focused_listen`
- `shy_peek`
- `thinking_side`
- `Tier C - leitura sutil`
- `neutral_premium`
- `curious_soft`
- `warm_happy`

### O que deve ser feito

1. remodelar a biblioteca de presets com assinatura visual clara
2. ajustar contraste entre presets conforme o tier de leitura
3. definir parametros base de blink e transicao por preset
4. manter coerencia com emocao, mood e estados de atencao

### Criterio de aceite

- cada preset tem assinatura visual propria
- o usuario reconhece o estado pela forma
- os presets mais importantes tem leitura imediata

### Criterio complementar obrigatorio

- executar `silhouette tests`
- congelar frames principais
- validar reconhecimento por forma, sem contexto

---

## F5 - Face Composition & Arbitration

### Objetivo

Formalizar a composicao das camadas visuais e impedir disputa entre blink, gaze, preset, transientes e clips.

### Funcao desta fase

- governar o resultado final da face
- eliminar conflito entre camadas
- reduzir flicker e comportamento nervoso

### Escopo

- `src/services/face/face_compositor.*`
- simplificacao interna de `src/services/face/face_service.*`
- integracao com `Emotion`, `Routine`, `Behavior` e `Gaze`

### Camadas minimas

- `base expression`
- `gaze layer`
- `blink layer`
- `idle layer`
- `mood layer`
- `transient reaction layer`
- `clip layer`

### O que deve ser feito

1. criar composicao formal por camadas
2. definir prioridades e ownership entre camadas
3. definir `minimum hold time`
4. definir `transition cooldown`
5. definir `clip cooldown`
6. definir anti-spam de `transient reactions`
7. garantir que blink e gaze nao sejam sobrescritos incorretamente

### Criterio de aceite

- multiplas influencias coexistem sem flicker
- clips, blink, gaze e transientes nao brigam entre si
- o resultado final e previsivel e tunavel

---

## F3 - Long Motion Clips

### Objetivo

Adicionar animacoes longas e memoraveis na tela, ja apoiadas em uma composicao governada.

### Funcao desta fase

- criar momentos assinatura do personagem
- enriquecer wake, sleep, listening, surprise e acknowledgement
- sustentar movimentos visuais que vao alem de micro-modulacao

### Escopo

- `src/models/face_clip.*`
- `src/services/face/face_clip_player.*`
- integracao com `src/services/face/face_service.*`

### Clips minimos recomendados

- `wake_up`
- `go_to_sleep`
- `attention_recovery`
- `thinking_loop`
- `soft_listen`
- `shy_retract`
- `happy_ack`

### O que deve ser feito

1. criar modelo de clip temporal com keyframes
2. permitir clips de 300 ms a 1500 ms
3. integrar clips ao compositor
4. respeitar cooldowns e ownership definidos em `F5`
5. evitar acoplamento direto de clip com comportamento de alto nivel

### Criterio de aceite

- o rosto consegue sustentar animacoes longas com intencao clara
- existem momentos visuais memoraveis
- clips nao quebram blink, gaze nem idle

---

## F6 - Multimodal Sync & Tooling

### Objetivo

Fechar a experiencia premium com sincronizacao multimodal e ferramentas de tuning.

### Funcao desta fase

- alinhar face, pescoco, LED e audio
- acelerar tuning de presets e clips
- reduzir iteracao cega em serial e codigo

### Escopo

- `src/services/motion_sync/motion_sync_service.*`
- `src/services/face/face_service.*`
- `tools/face_preset_editor/*`

### O que deve ser feito

1. sincronizar gaze visual com neck yaw/tilt quando fizer sentido
2. sincronizar clips com gestos fisicos leves
3. sincronizar face com LED e audio de forma secundaria
4. evoluir o editor visual com preview de blink, saccade e clip
5. permitir export direto de presets e parametros

### Criterio de aceite

- face e pescoco contam a mesma historia
- tuning de preset e clip deixa de depender de tentativa cega
- o sistema visual fica pronto para polish final de produto

---

## Observacoes de implementacao

- `F3` fica propositalmente depois de `F5`
- clips longos so devem nascer depois da composicao estar governada
- o objetivo e evitar disputa entre `clip`, `blink`, `gaze` e `transient reaction`
- o engine atual deve ser evoluido incrementalmente, sem reescrita total desnecessaria



