# Etapa 4 - Checklist de Prontidao Arquitetural

Data: 2026-03-07
Projeto: nc-OS (ESP32-S3 + ST7789 + LovyanGFX)

## 1) Regras de dependencia (status)
- `app -> core/services/hal/drivers/interfaces`: OK
- `core -> interfaces/models/config`: OK
- `core -> services concretos`: REMOVIDO
- `services -> drivers`: NAO ENCONTRADO
- `services -> HAL por interface`: OK
- `hal -> interfaces de dispositivo`: OK
- `drivers -> services`: NAO ENCONTRADO
- `config` centralizando pinos/constantes fixas: OK para trilha visual atual

## 2) Consolidacoes executadas
- EventBus evoluido com:
  - unsubscribe por tipo/listener
  - unsubscribeAll
  - assinatura global via `EventType::Any`
  - protecao contra subscribe duplicado
- Contrato de eventos evoluido com:
  - `EventSource` tipado
  - eventos de pipeline (`FaceFrameRendered`, `CameraFrameSampled`)
- `SystemManager` desacoplado de implementacoes concretas:
  - agora depende de `IVisualService` e `IVisionService`
- `FaceService` e `VisionService` desacoplados de HAL concreto:
  - agora dependem de `IDisplayPort` e `ICameraPort`

## 3) Impacto para proximas integracoes
- Sensores novos podem entrar sem alterar `core`:
  - criar interface de service
  - injetar no composition root (`app_factory`)
  - publicar eventos no EventBus
- Motion/Emotion/Behavior podem ser plugados como listeners do EventBus (`EventType::Any` ou tipos especificos)

## 4) Pendencias intencionais (fora de escopo)
- Nao foi implementado sensor real/touch/IMU/servo/audio/cloud.
- Nao foi feita reescrita de placeholders (`emotion_service`, `behavior_service`, etc.).

## 5) Riscos residuais
- Build nao validado neste ambiente por ausencia de `pio/platformio` no PATH.
- Como mitigacao, mudancas foram incrementais e sem alteracao de pipeline visual do Face Engine.

## 6) Checklist de entrada da Etapa 4
- [ ] Executar `pio run` localmente e validar compilacao.
- [ ] Confirmar boot + face render sem regressao visual.
- [ ] Adicionar `SensorService` por interface (sem acesso direto a driver).
- [ ] Definir novos `EventType` de sensores no `core/event_types.h`.
- [ ] Conectar novos services no `app_factory` (composition root).
- [ ] Manter pinos/parametros fixos apenas em `config/hardware_pins.h` e `config/hardware_config.h`.
