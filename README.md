# SilverChannel 01

Channel strip VST3 da família **SILVER**: prata com textos em azul-marinho, a mesma paleta do RefPro SILVER, do SILVER-MB e do VX_Leveler.
O layout segue o Scheps Omni Channel 2, da Waves.

```
[ PRE ] [ GATE ] [ DS2 ] [ EQ ] [ COMP ] [ INSERT ]  →  [ MASTER ]
```

Os seis blocos podem ser reordenados com as setas `<` `>` no cabeçalho de cada módulo. Clicar no **nome** do módulo liga ou desliga o módulo. GATE, DS2 e COMP também têm o botão **SC**, que abre a vista expandida com o sidechain e o modo estéreo.
Ao abrir o plugin, todos os módulos começam desligados, como no Omni.

## Módulos

| Módulo | Controles | Como soa / como foi modelado |
|---|---|---|
| **PRE** | SAT (0–100 %), ODD / EVEN / HEAVY / CRUSH, HP e LP de 20 Hz a 20 kHz com inclinação 6/12/18/24R dB/oit., RES (0,1–4), THUMP (+2/+4 dB) | ODD usa `tanh` e gera só harmônicos ímpares. EVEN é assimétrico, com H2 forte. HEAVY é um clipper de joelho firme. CRUSH é uma distorção agressiva com dobra de onda. A saturação roda com oversampling 2x e compensação de nível. THUMP é um shelf grave ressonante em ~85 Hz com um leve recuo em ~210 Hz. A ressonância atua nos modos 12, 18 e 24. |
| **GATE** | GATE/EXP, Threshold (−inf a 0), Floor, Close (histerese), Attack e Release (0,1 ms–10 s) | Na vista expandida: filtros HP/LP de sidechain com LINK, que transforma os dois em passa-banda. Também tem EXT SC, LISTEN e modo ST/DUO/MS. |
| **DS2** | Dois de-essers em qualquer frequência: LO SHF, BELL, NOTCH ou HI SHF, Freq, Threshold (−48 a 0), LEDs de −3/−6/−9/−12, LISTEN e OUTPUT (±12) | É um EQ dinâmico: só a banda detectada é atenuada, e o resto do sinal passa intacto. |
| **EQ** | HIGH, MID, TONE e LOW, com ±18 dB, 20 Hz–20 kHz e Q de 0,2 a 10 no modo PARA | HIGH e LOW têm RES (shelf com sobressinal), SHELF e PARA. **MID** WIDE é limpo, com Q proporcional ao ganho. **TONE** WIDE é "espalhado" (dois sinos escalonados) e serve para colar a faixa na mix. No NARROW, o Q do MID chega a 2,0 e o do TONE a 1,6, conforme medições publicadas no fórum da Waves. |
| **COMP** | VCA / FET / OPT / SOFT, Threshold (−50 a 0), Ratio (1:1 a inf:1), Attack (0,4–150 ms), Release (50 ms–5 s), MIX e OUTPUT | **VCA**: feed-forward rápido e limpo. **FET**: realimentado, com pico e cor harmônica que aumenta com a redução de ganho, e bombeia em ratios altos. **OPT**: RMS lento, com "memória longa" (release fica mais lento depois de compressão sustentada) e cor sutil mesmo sem comprimir. **SOFT**: joelho de 18 dB. Trocar o tipo não mexe nos outros ajustes, o que permite comparar A/B direto. A vista expandida tem os filtros de sidechain: Low Shelf, Bell e HPF. |
| **INSERT** | Botão **+**: lista os VST3 instalados, permite procurar um arquivo, abrir o editor ou remover | Hospeda qualquer VST3 de terceiros dentro da cadeia. O plugin e o estado dele são salvos no projeto. |
| **MASTER** | VUs L/R (IN, OUT ou GR total), TRIM de 4 a 26 dB, faders IN/OUT com medidor de pico, clip e hold, LIMITER (0 a −30 dB), Ø PHASE e MONITOR (ST/MONO/L/R/M/S) | O limiter é brickwall de latência zero e distorce se for muito empurrado, como no original. |

Recursos gerais:

- Modo estéreo ST/DUO/MS por módulo dinâmico.
- Sidechain externo: entrada auxiliar estéreo do plugin, ativada pelo botão EXT em cada módulo.
- Undo/redo de 32 níveis, incluindo a reordenação dos módulos.
- Presets: 14 de fábrica, presets de usuário salvos em `Documentos\SilverChannel 01\Presets`, *Set as Default* e *Full Reset*.
- Redimensionamento de 70 % a 150 %.

## Compilar (GitHub Actions)

1. Crie o repositório, por exemplo `SILVERVST3/SilverChannel-01`, e envie todo o conteúdo desta pasta.
2. O workflow `.github/workflows/build.yml` roda sozinho no Windows.
3. Em **Actions → artefato `SilverChannel01-VST3-Windows`**, baixe e copie `SilverChannel 01.vst3` para `C:\Program Files\Common Files\VST3`.
4. No REAPER: *Options → Preferences → Plug-ins → VST → Re-scan*.

Para usar o **sidechain externo** no REAPER, mande o sinal para os canais 3/4 da faixa e ligue o EXT no módulo.

## Diferenças em relação ao Omni (próximas versões)

- Os controles L/R desacoplados (LINK) ainda não existem: os dois lados usam sempre o mesmo ajuste. Os modos ST, DUO e MS já atuam na detecção dos módulos dinâmicos.
- A reordenação é feita pelas setas `<` `>`, e não arrastando os módulos.
- Ainda não há modo FOCUS.
- O atalho Ctrl+arraste no knob de frequência (para fazer sweep com listen) ainda não existe.
