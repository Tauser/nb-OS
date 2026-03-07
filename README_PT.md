# nc-OS - Robot Desktop (ESP32-S3)

Base de firmware para robot desktop com arquitetura modular (drivers, HAL, services e core).

## Hardware alvo
- Freenove ESP32-S3-WROOM CAM N16R8
- ST7789 2"
- OV2640
- MPU6050
- INMP441 + MAX98357A + speaker 4 ohms 3W
- 2x Feetech SCS0009
- Touch capacitivo por fita de cobre
- SW6106 + bateria 3.7V

## Estrutura do projeto
- Arquitetura e bootstrap: `docs/PROJETO_ESTRUTURA_PT.md`
- Pinagem consolidada (perfil real): `docs/PINOUT_CONSOLIDADO_PT.md`

## Perfis de build
- Hardware real: `env:esp32s3`
- Simulacao Wokwi: `env:esp32s3_sim`

O `wokwi.toml` ja aponta para o build `esp32s3_sim`.

## Status atual
- Pipeline de display/face ativo.
- Pipeline de camera estruturado (interface + driver + HAL + service).
- Feature flags por modo em `src/config/feature_flags.h`.

## Proximo passo
Implementar `esp32-camera` no driver OV2640 com perfil final da placa.