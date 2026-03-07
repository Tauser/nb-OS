# Decisões da Arquitetura V2

- Separação em `.h/.cpp`
- Camadas: core, interfaces, drivers, hal, services, ai
- `main.cpp` limpo
- composição central em `AppFactory`
- serviços não acessam hardware diretamente
- Event Bus desde o início
- suporte futuro a hardware real e simulação
