# ENGG54 – Laboratório Integrado III-A  
## Reconstruçção de efeitos de uma mesa digital (Vedo/Teyun A8) no DSP TMS320C5502 eZdsp

Este repositório contém o projeto semestral da disciplina **ENGG54**, e seu objetivo foi **reconstruir efeitos de áudio** presentes na mesa digital **Vedo/Teyun A8** e implementá-los em **tempo real** no kit **Texas Instruments TMS320C5502 eZdsp**.

**Resumo do sistema**  
- **Entrada/Saída de áudio:** codec (ADC/DAC)  
- **Transporte de amostras:** **McBSP** + **DMA** (buffers pingpong)  
- **Tempo real:** processamento por bloco, baixa latência.  
- **Interface:** LCD + switches (I2C/GPIO)  
- **Efeitos implementados:** base de **reverb (FreeVerb)** + efeitos derivados (ex.: chorus, “radio voice”, pitch shift)  
- **Ferramenta:** Code Composer Studio (CCS)

---

## Objetivos do trabalho

- Reconstruir efeitos da Vedo/Teyun A8 no C5502 eZdsp, conforme a regra **Eₙ = 3n**.
- Implementar algoritmos de PDS em **tempo real**, com **DMA + McBSP**, garantindo estabilidade.
- Desenvolver uma **IHM** simples (LCD + switches) para controlar execução/estado do sistema.
- Validar com testes práticos (microfone → DSP → fones) e comparação auditiva com referências.

---

## Efeitos e algoritmos

### Reverb (base principal)
- Implementação inspirada no **FreeVerb** (mais denso e “suave” que Schroeder clássico).
- Aritmética em **Q15** e uso de **intrinsics** para eficiência.

### Pitch Shifter (PSOLA/OLA simplificado)
- Buffer circular com **dois taps** + **crossfade triangular**.
- **Interpolação linear** para reduzir artefatos de leitura fracionária.
- Usado para aproximar efeitos onde o autotune não ficou viável em tempo real.

---

## Arquitetura do sistema

### Caminho do áudio (tempo real)
1. **AIC3204** (master de clock) define taxa de amostragem ( 48 kHz).
2. **McBSP** recebe/transmite frames (configuração compatível com 32 bits por frame).
3. **DMA** move amostras entre McBSP ↔ buffers em memória, em modo **ping-pong**.
4. CPU processa o bloco **enquanto o DMA preenche o próximo**.

### Controle do sistema (FSM)
- Máquina de estados com:
  - `INITIAL` (setup/recursos)
  - `RUNNING` (processamento contínuo)
- A FSM evita reconfigurações repetidas e mantém o caminho de áudio “limpo”.

### Interface (IHM)
- LCD via I2C
- Switches via I2C/GPIO
- Atualizações de interface desacopladas do processamento crítico de áudio.

---

## Estrutura do repositório

O repositório está organizado em múltiplos projetos do Code Composer Studio (CCS),
cada um correspondente a um efeito/preset específico do trabalho.  
Apesar de estarem em pastas diferentes (`Project03`, `Project06`, …), **todos os projetos
seguem mais ou emnos a mesma estrutura interna**.  
Abaixo está apresentada a estrutura, de forma resumida.

### Estrutura típica de um projeto (`ProjectXX/`)

ProjectXX/
├── src/
│   ├── Arquivo principal do projeto. Contém a inicialização do sistema,
│   │   a máquina de estados e a integração entre áudio, efeitos e interface.
│   │
│   ├── effects/
│   │   ├── reverb.c / reverb.h
│   │   ├── pitch_shifter.c / pitch_shifter.h
│   │   ├── autotune.c / autotune.h   (quando aplicável)
│   │   └── outros efeitos específicos do projeto
│   │
│   ├── drivers/
│   │   ├── aic.c / aic.h / aic3204_init.c
│   │   ├── mcbsp.c / mcbsp.h
│   │   ├── dma.c / dma.h / dma4.pjt / vectors_dma4.s55
│   │   ├── LCD.C / LCD.H
│   │   └── drivers auxiliares (I2C, GPIO, etc.)...
│   │
│   ├── state_machine/
│   │   ├── state_machine.c / state_machine.h
│   │
│   └── states/
│       ├── state_initial.c
│       ├── state_initial.h
│       ├── tate_running.c
│       └── state_running.h
│
├── include/
│   Headers globais, definições de tipos, macros e protótipos.
│
├── lcd-osd9616/
│   ├── lcd.c / lcd_.h...
│   Controle do display LCD.
│
├── lib/
│   Bibliotecas auxiliares e código reutilizável.
│
├── lnkx.cmd
│   Arquivo de linker. 
│   main.c
│
├── .ccsproject
├── .project
├── .cproject
├── .settings/
└── .launches/



---

## Requisitos

### Hardware
- TexasInstruments **TMS320C5502 eZdsp**
- Codec **TLV320AIC3204**
- Microfone/Line-In e fones/Line-Out

### Software
- **Code Composer Studio (CCS)**

---

## Como compilar e executar (CCS)

1. Abra o CCS e importe o projeto (`File > Import Project`).
2. Confirme o **linker command file** ativo (ex.: `lnk.cmd`) e as seções de memória.
3. Conecte o kit e inicie uma sessão de debug.
4. Carregue o `.out` e execute.

### Execução em tempo real
- Conecte microfone/line-in
- Conecte fones/line-out
- Use os **switches** para iniciar/parar (`INITIAL` ↔ `RUNNING`)
- O LCD indica estado atual.

---

## Notas de implementação importantes

- O projeto usa **ponto fixo (Q15)** para atender tempo real no C5502.
- Operações críticas utilizam **intrinsics** (`_smpy`, `_sadd`, `_ssub`) para eficiência.
- Buffers de DMA devem ficar em memória adequada (ex.: DARAM) para estabilidade.
- Efeitos com custo alto (ex.: detecção de pitch AMDF) podem exigir:
  - redução de taxa de atualização
  - execução fora do ISR/caminho crítico
  - otimizações adicionais de memória e ciclos

---

## Limitações / Trabalho futuro

- O efeito **autotune** (detecção + correção em tempo real) é computacionalmente custoso e,
  nesta versão, **não ficou estável em tempo real** sob as restrições do kit.
- Trabalhos futuros:
  - otimização de detecção de pitch (redução de complexidade, processamento em background)
  - melhor metodologia de calibração (métricas objetivas além de avaliação auditiva)
  - permitir alternância dinâmica entre múltiplos efeitos no mesmo firmware (melhor organização de memória)

---

## Autores

- Felipe Ma  
- João Cerqueira  
- Rísia Cerqueira  
- Vinicius de Jesus  

UFBA – DEEC – ENGG54 (2025)

---

## Referências principais

- Oppenheim & Schafer — *Discrete-Time Signal Processing*
- Lyons — *Understanding Digital Signal Processing*
- Zölzer — *DAFX: Digital Audio Effects*
- TI — Documentação C55x, DMA, AIC3204 e eZdsp kit

