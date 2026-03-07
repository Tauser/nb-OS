# nc-OS - Estrutura Profissional (ESP32-S3 Robot Desktop)

## 1) Objetivo
Organizar o firmware para evoluir de forma previsivel, com modulos isolados por responsabilidade e integracao gradual do hardware:

- Freenove ESP32-S3-WROOM CAM
- ST7789 2"
- OV2640
- MPU6050
- INMP441 + MAX98357A + falante 4 ohms
- 2x Feetech SCS0009
- Touch capacitivo por fita de cobre
- SD card

## 2) Estrutura de pastas

```text
src/
  app/        -> composicao da aplicacao
  config/     -> pinagem, flags e parametros de hardware
  core/       -> ciclo de vida, eventos, diagnostico, armazenamento
  drivers/    -> acesso direto ao hardware
    audio/
    camera/
    display/
    imu/
    motion/
    power/
    storage/
    touch/
  hal/        -> adaptadores entre drivers e servicos
  interfaces/ -> contratos de modulo
  models/     -> estados e modelos de dominio
  services/   -> comportamento de alto nivel (voz, visao, interacao etc.)
  utils/      -> utilitarios
```

## 3) Ordem de bootstrap recomendada
1. `Diagnostics`
2. `ConfigManager`
3. `Storage (SD)`
4. `Display`
5. `Touch`
6. `IMU`
7. `Audio In/Out`
8. `Motion (servos)`
9. `Camera`
10. Servicos de alto nivel (face, voice, vision, interaction)

## 4) Regra de evolucao
- Driver: so inicializa e le/escreve hardware.
- HAL: traduz dados de driver para formato da aplicacao.
- Service: aplica regra de negocio (estado, timing, fluxo).
- Core: orquestra boot, erro, heartbeat e eventos.

## 5) Proximo marco tecnico
- Fechar pinagem final em bancada.
- Ativar modulo por modulo via `feature_flags.h`.
- Criar teste de smoke por modulo (init + leitura/escrita minima).
- Registrar todos os resultados no serial durante boot.