# Pinout Consolidado - nc-OS (Perfil Real)

Placa alvo: Freenove ESP32-S3-WROOM CAM N16R8

## Display ST7789 2"
- SCLK: GPIO47
- MOSI: GPIO21
- MISO: N/A
- DC: GPIO48
- CS: GPIO14
- RST: GPIO2
- BL: 3V3 direto (`BL = -1` no firmware)

## Camera OV2640
- Via conector flat dedicado da placa
- `PWDN` e `RESET` em placeholder (`-1`) ate implementar `esp32-camera` com profile da placa

## Audio I2S
- BCLK: GPIO43
- WS/LRCLK: GPIO44
- INMP441 SD: GPIO42
- MAX98357A DIN: GPIO1

## IMU MPU6050 (I2C)
- SDA: GPIO41
- SCL: GPIO40

## Servo Bus (FE-TTLinker-Mini)
- DATA: GPIO39

## Touch capacitivo
- DATA: GPIO38

## Perfil simulado
- Definido por `NCOS_PIN_PROFILE_SIM=1`
- Mantem pinagem do `diagram.json` para Wokwi

## Observacoes de conflito
- Conflito removido: `Display CS GPIO14` x `Servo DATA` (agora GPIO39)
- `ARDUINO_USB_CDC_ON_BOOT=1` para liberar UART e manter debug por USB nativo