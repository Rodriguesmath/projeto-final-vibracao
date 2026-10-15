# Projeto Final: Monitor de Vibração Industrial de Baixo Consumo (Autonomia de 10 Anos)
**Formação em Sistemas Embarcados Virtus-CC — Curso 2: Periféricos**  
**Prof. D.Sc. Rafael Bezerra Correia Lima**

---

> [!NOTE]
> **Estilo de Codificação:** A arquitetura de firmware, nomenclatura de identificadores, encapsulamento via struct de dados locais (Regra 4.8), camada de abstração de hardware (`Bsp`), tratamento de eventos assíncronos e padronização Doxygen deste repositório foram inspiradas nas diretrizes de boas práticas do **Laboratório ASSERT (IFPB)**.

---

## 1. Descrição do Projeto

Este projeto consiste em um **nó sensor autônomo para manutenção preditiva industrial** montado em mancal de motores e redutores elétricos.
O dispositivo avalia os níveis de severidade de vibração mecânica de acordo com a norma internacional **ISO 10816**, operando com uma única bateria de cloreto de tionil de lítio ($\text{Li-SOCl}_2$) com vida útil projetada superior a **10 anos**.

### Principais Características:
- **Sensor:** Acelerômetro MEMS triaxial MPU-6050 operando em modo de amostragem em surto (*burst*) a $250\text{ Hz}$ com filtro passa-baixa digital (DLPF de $44\text{ Hz}$).
- **Estratégia de Gerenciamento de Energia:**
  - O microcontrolador permanece em **Stop Mode** de ultra-baixo consumo ($< 5\,\mu\text{A}$) a maior parte do tempo.
  - O acelerômetro MPU-6050 é colocado em **Sleep Mode** ($5\,\mu\text{A}$) enquanto inativo.
  - O despertar periódico é controlado via **RTC WakeUp Timer** interno a cada 8 horas (ou acionado manualmente pelo botão do operador via **EXTI**).
- **Processamento de Sinal:** Cálculo do valor eficaz da componente alternada (RMS dinâmico com remoção do offset gravitacional de $1\text{g}$) em janelas de 512 amostras (~2,05 segundos).
- **Alarme Visual de Baixo Dreno:** Em caso de anomalia mecânica, o LED de alarme sinaliza a falha através de micro-pulsos com corrente limitada por resistor de $1.5\text{ k}\Omega$ (~$1.3\text{ mA}$ por pulso).

---

## 2. Pinagem de Conexão

| Elemento do Circuito | Pino MCU (Black Pill / F411) | Pino MCU (H723 / F767ZI) | Função |
| :---: | :---: | :---: | :--- |
| **MPU-6050 SCL** | `PB6` | `PB6` | `I2C1_SCL` (Clock com pull-up de 4.7k) |
| **MPU-6050 SDA** | `PB7` | `PB7` | `I2C1_SDA` (Dados com pull-up de 4.7k) |
| **MPU-6050 AD0** | `GND` | `GND` | Endereço I²C `0x68` |
| **MPU-6050 VCC/GND** | `3.3V / GND` | `3.3V / GND` | Barramento de alimentação |
| **LED Alarme** | `PC13` | `PC13` / `PB0` (Green) | Alerta visual (resistor de 1.5k) |
| **Botão de Teste (EXTI)** | `PA0` | `PA0` / `PC13` (Blue Btn) | Despertar manual sob demanda |
| **UART Telemetria** | `PA9` (TX) | `PA9` / `PD8` (ST-Link VCP) | Diagnóstico serial a 115200 bps |

---

## 3. Estimativa de Consumo e Prova Matemática de Autonomia (> 10 Anos)

### Perfil de Operação por Ciclo de 8 Horas ($T_{\text{periodo}} = 28.800\text{ s}$):

1. **Fase Ativa ($T_{\text{ativo}} \approx 2,1\text{ s}$):**
   - MCU ativo processando: $15\text{ mA}$
   - MPU-6050 medindo: $3,5\text{ mA}$
   - Telemetria UART: $1,0\text{ mA}$
   - $I_{\text{ativo}} = 19,5\text{ mA}$

2. **Fase de Repouso ($T_{\text{sleep}} = 28.797,9\text{ s}$):**
   - MCU em Stop Mode com RTC ativo: $4,5\,\mu\text{A}$
   - MPU-6050 em modo Sleep: $5,0\,\mu\text{A}$
   - Correntes de fuga em barramentos e PCB: $2,5\,\mu\text{A}$
   - $I_{\text{sleep}} = 12,0\,\mu\text{A} = 0,012\text{ mA}$

### Cálculo da Corrente Média Efetiva:

$$I_{\text{medio}} = \frac{I_{\text{ativo}} \times T_{\text{ativo}} + I_{\text{sleep}} \times T_{\text{sleep}}}{T_{\text{periodo}}}$$

$$I_{\text{medio}} = \frac{(19,5\text{ mA} \times 2,1\text{ s}) + (0,012\text{ mA} \times 28.797,9\text{ s})}{28.800\text{ s}}$$

$$I_{\text{medio}} = \frac{40,95 + 345,57}{28.800} \approx \frac{386,52}{28.800} \approx 0,0134\text{ mA} = \mathbf{13,4\,\mu\text{A}}$$

### Capacidade e Autonomia com Bateria $\text{Li-SOCl}_2$ (Ex: Saft LS14500 ou EVE ER14505):
- Capacidade nominal: $C = 2.400\text{ mAh}$ ($3,6\text{ V}$, tamanho AA).
- Fator de segurança térmico e derating: $85\% \implies C_{\text{util}} = 2.040\text{ mAh}$.
- Autodescarga típica da química $\text{Li-SOCl}_2$: $\approx 1\%/\text{ano} = 24\text{ mAh/ano}$.
- Consumo anual do firmware: $13,4\,\mu\text{A} \times 8.760\text{ h/ano} = 117,4\text{ mAh/ano}$.
- Consumo total anual: $117,4 + 24 = 141,4\text{ mAh/ano}$.

$$\mathbf{Autonomia} = \frac{2.040\text{ mAh}}{141,4\text{ mAh/ano}} \approx \mathbf{14,4\text{ Anos}}$$

> A autonomia estimada supera a meta estipulada de 10 anos em mais de 40%!

---

## 4. Severidade de Vibração Industrial (Norma ISO 10816)

A norma avalia a severidade vibracional através da velocidade/aceleração eficaz RMS:

| Faixa de Vibração ($g_{\text{rms}}$) | Classificação ISO | Ação Operacional |
| :---: | :---: | :--- |
| **$< 0,20\text{ g}$** | **Classe A / B (Bom)** | Máquina em perfeito estado de operação |
| **$0,20\text{ g} - 0,45\text{ g}$** | **Classe C (Aceitável)** | Operação irrestrita; requer inspeção de rotina |
| **$> 0,45\text{ g}$** | **Classe D (Alarme)** | Vibração destrutiva; aciona pulso de alarme |

### Equações do Processamento:

$$\mu_x = \frac{1}{N} \sum_{i=1}^{N} A_x[i], \quad \text{RMS}_x = \sqrt{\frac{1}{N} \sum_{i=1}^{N} (A_x[i] - \mu_x)^2}$$

$$\text{RMS}_{\text{total}} = \sqrt{\text{RMS}_x^2 + \text{RMS}_y^2 + \text{RMS}_z^2}$$

---

## 5. Roteiro de Calibração e Testes em Shaker de Bancada

1. Conecte o terminal serial a **115200 bps, 8-N-1**.
2. Fixe a placa firmemente à mesa vibratória (shaker de calibração).
3. **Teste em Repouso:**
   - Acione o botão de despertar `BTN_USER`.
   - O terminal deve registrar $\text{RMS}_{\text{total}} \le 0,05\text{ g}$ e status `BOM`.
4. **Teste Dinâmico em 30 Hz:**
   - Ajuste o shaker para uma vibração de aceleração controlada ($0,30\text{ g}$).
   - Pressione `BTN_USER`: o sistema reportará status `ACEITAVEL`.
5. **Teste de Alarme de Sobrecarga em 60 Hz:**
   - Eleve a excitação do shaker para $0,60\text{ g}$.
   - O firmware detecta que $\text{RMS}_{\text{total}} > 0,45\text{ g}$, reporta `ALARME CRITICO` e pisca o LED de alarme em 5 pulsos ultrarrápidos antes de retornar ao modo Stop.
