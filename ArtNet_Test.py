#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
===============================================================================
PROJETO: PICO ARTNET DMX NODE (RP2040)
ARQUIVO: ArtNet_Test.py
AUTOR: Matheus Dev (@Matheus-devpixel)
DESCRIÇÃO: Transmissor e testador de estresse de alta precisão para protocolo
           Art-Net v4 (UDP Porta 6454).
           Transmite até 8 universos DMX simultâneos (4.096 canais) em conjunto
           a uma taxa estável de até 60 quadros por segundo (60 FPS = 480 pkt/s).

ANATOMIA DO PACOTE ARTDMX (530 BYTES):
  Bytes 0 a 7   : Assinatura textual fixa "Art-Net\\0" (8 bytes).
  Bytes 8 a 9   : OpCode 0x5000 (OpDmx - Little Endian: 0x00, 0x50).
  Bytes 10 a 11 : Versão do protocolo Art-Net (ProtVer 14 - Big Endian: 0x00, 0x0E).
  Byte 12       : Sequence counter (1 a 255; 0 desativa verificação de ordem).
  Byte 13       : Physical Input Port (0x00).
  Bytes 14 a 15 : Universo DMX / SubNet (Little Endian: bits 0-7, bits 8-15).
  Bytes 16 a 17 : Tamanho do payload DMX (Big Endian: 0x02, 0x00 = 512 canais).
  Bytes 18 a 529: 512 canais de intensidade DMX512 (valores de 0 a 255).

MODOS DE OPERAÇÃO:
  - show   : Simulação de palco profissional real (Moving Heads 16-bit, Wash RGB,
             Dimmers, Strobes e Efeitos Pixel Mapping).
  - strobe : Todos os 4.096 canais piscando em sincronia (Full 255 / 0).
  - rgb    : Gradiente contínuo de cor RGB espalhado por todos os universos.
  - chase  : Onda luminosa correndo velozmente pelos 4.096 canais.
  - ramp   : Fader linear contínuo subindo de 0 a 255.
  - random : Ruído aleatório (estresse de largura de banda e processamento).
  - idle   : Mesa de luz em repouso (canais em 0, Pan/Tilt centralizados).

EXEMPLOS DE USO:
  python ArtNet_Test.py
  python ArtNet_Test.py 60 2.0.1.1
  python ArtNet_Test.py --ip=2.0.1.1 --fps=60 --mode=strobe
  python ArtNet_Test.py --mode=rgb --univ=0
===============================================================================
"""

import socket
import time
import sys
import math
import urllib.request
import json

# =============================================================================
# OTIMIZAÇÃO DE TEMPORIZADOR MULTIMÍDIA DO WINDOWS
# =============================================================================
# No Windows, o timer padrão do sistema operacional opera com granularidade de
# 15.6 milissegundos. Chamando timeBeginPeriod(1), forçamos o escalonador do
# Windows a operar com resolução de 1.0 ms, eliminando o jitter na taxa de quadros.
try:
    import ctypes
    ctypes.windll.winmm.timeBeginPeriod(1)
except Exception:
    pass

# =============================================================================
# PARÂMETROS PADRÃO DE OPERAÇÃO
# =============================================================================
TARGET_IP = "2.0.1.1"               # Endereço IP do nó Pico Art-Net
TARGET_PORT = 6454                  # Porta UDP padrão do protocolo Art-Net
NUM_UNIVERSES = 8                   # Quantidade de universos sincronizados
CHANNELS_PER_UNIVERSE = 512         # Canais padrão por universo DMX512-A
TOTAL_CHANNELS = NUM_UNIVERSES * CHANNELS_PER_UNIVERSE  # 4.096 canais no total
TARGET_FPS = 60.0                   # Taxa alvo de envio de frames por segundo
START_UNIVERSE = None               # Universo inicial (auto-detectado se None)
MODE = "show"                       # Modo de simulação de iluminação

KNOWN_MODES = {
    "show", "live", "idle", "standby", "chase",
    "random", "ramp", "linear", "fader", "fast", "strobe", "rgb"
}

# =============================================================================
# 1. PARSER AVANÇADO DE ARGUMENTOS DA LINHA DE COMANDO
# =============================================================================
args = sys.argv[1:]
i = 0
while i < len(args):
    arg = args[i].strip()
    arg_lower = arg.lower()

    if arg_lower.startswith("--fps="):
        try:
            TARGET_FPS = float(arg.split("=")[1])
        except ValueError:
            pass
    elif arg_lower in ("--fps", "-fps") and i + 1 < len(args):
        try:
            TARGET_FPS = float(args[i + 1])
            i += 1
        except ValueError:
            pass
    elif arg_lower.startswith("--ip="):
        TARGET_IP = arg.split("=")[1]
    elif arg_lower in ("--ip", "-ip") and i + 1 < len(args):
        TARGET_IP = args[i + 1]
        i += 1
    elif arg_lower.startswith("--mode="):
        MODE = arg.split("=")[1].lower()
    elif arg_lower in ("--mode", "-m") and i + 1 < len(args):
        MODE = args[i + 1].lower()
        i += 1
    elif arg_lower.startswith("--univ="):
        try:
            START_UNIVERSE = int(arg.split("=")[1])
        except ValueError:
            pass
    elif arg_lower in ("--univ", "-u") and i + 1 < len(args):
        try:
            START_UNIVERSE = int(args[i + 1])
            i += 1
        except ValueError:
            pass
    elif "." in arg and len(arg.split(".")) == 4:
        TARGET_IP = arg
    elif arg_lower in KNOWN_MODES:
        if arg_lower in ("linear", "ramp"):
            MODE = "ramp"
        elif arg_lower in ("standby", "idle"):
            MODE = "idle"
        elif arg_lower in ("live", "show"):
            MODE = "show"
        else:
            MODE = arg_lower
    else:
        try:
            num = float(arg)
            if TARGET_FPS == 60.0 and num >= 1.0:
                TARGET_FPS = num
            elif START_UNIVERSE is None and num.is_integer() and 0 <= int(num) <= 255:
                START_UNIVERSE = int(num)
            else:
                TARGET_FPS = num
        except ValueError:
            pass
    i += 1

if TARGET_FPS <= 0:
    TARGET_FPS = 60.0

# =============================================================================
# 2. AUTO-DETECÇÃO DO UNIVERSO INICIAL VIA REST API OU IP
# =============================================================================
# Tenta consultar o endpoint /status do nó via HTTP. Se o dispositivo responder,
# lê o universo inicial exato configurado na memória Flash. Caso a conexão
# HTTP não responda a tempo, calcula o universo inicial pelo terceiro octeto do IP:
#   IP 2.0.1.1  -> Nó 1  -> Universos 0 a 7
#   IP 2.0.2.1  -> Nó 2  -> Universos 8 a 15
#   IP 2.0.16.1 -> Nó 16 -> Universos 120 a 127
if START_UNIVERSE is None:
    try:
        url = f"http://{TARGET_IP}/status?u=0"
        req = urllib.request.Request(url, headers={'User-Agent': 'ArtNetTestTool'})
        with urllib.request.urlopen(req, timeout=0.35) as resp:
            data = json.loads(resp.read().decode('utf-8'))
            if 'start_u' in data:
                START_UNIVERSE = int(data['start_u'])
                print(f"[REDE] Conectado ao Nó Pico: '{data.get('node_name', 'Pico')}' | "
                      f"Faixa de Universos: {START_UNIVERSE} a {START_UNIVERSE + 7}")
    except Exception:
        pass

if START_UNIVERSE is None:
    parts = TARGET_IP.split('.')
    if len(parts) == 4 and parts[0] == "2" and parts[1] == "0" and parts[3] == "1":
        try:
            node_id = int(parts[2])
            if 1 <= node_id <= 16:
                START_UNIVERSE = (node_id - 1) * 8
        except ValueError:
            START_UNIVERSE = 0
    else:
        START_UNIVERSE = 0

# =============================================================================
# 3. CRIAÇÃO DO SOCKET UDP DE ALTA VELOCIDADE
# =============================================================================
# Configura o socket com buffer de transmissão ampliado para 256 KB para evitar
# que rajadas atômicas de 8 pacotes consecutivos sofram drop no kernel do SO.
sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
try:
    sock.setsockopt(socket.SOL_SOCKET, socket.SO_BROADCAST, 1)
    sock.setsockopt(socket.SOL_SOCKET, socket.SO_SNDBUF, 262144)  # 256 KB
except Exception:
    pass

DEST = (TARGET_IP, TARGET_PORT)

# =============================================================================
# 4. PRÉ-MONTAGEM DOS 8 PACOTES ARTDMX EM BYTEARRAY ZERO-COPY
# =============================================================================
# Os cabeçalhos e buffers de 530 bytes são pré-alocados na memória antes do loop.
# Durante o envio, apenas os bytes de dados DMX e a sequência são atualizados,
# atingindo desempenho máximo com zero overhead de alocação de memória (GC).
packet_buffers = []
universe_list = []

for u in range(NUM_UNIVERSES):
    real_u = START_UNIVERSE + u
    universe_list.append(real_u)

    buf = bytearray(b'Art-Net\x00')
    buf += bytes([
        0x00, 0x50,                           # OpCode ArtDmx (0x5000)
        0x00, 0x0E,                           # ProtVer 14
        0x01,                                 # Sequence inicial
        0x00,                                 # Physical Port
        real_u & 0xFF, (real_u >> 8) & 0xFF,  # SubUni (Universo DMX)
        0x02, 0x00                            # Comprimento dos canais (512 bytes)
    ])
    buf += bytearray(CHANNELS_PER_UNIVERSE)   # 512 canais de dados (inicialmente zerados)
    assert len(buf) == 530, f"Erro estrutural no pacote ArtDmx: {len(buf)} bytes"
    packet_buffers.append(buf)

# Extrai referências diretas para evitar indexação de lista no loop de alta velocidade
p0 = packet_buffers[0]
p1 = packet_buffers[1]
p2 = packet_buffers[2]
p3 = packet_buffers[3]
p4 = packet_buffers[4]
p5 = packet_buffers[5]
p6 = packet_buffers[6]
p7 = packet_buffers[7]

# =============================================================================
# 5. GERADOR DE EFEITOS REALISTAS DE ILUMINAÇÃO CÊNICA (LIGHTING RIG)
# =============================================================================
def generate_show_frame(t, u_idx):
    """
    Gera matematicamente cenas e comportamentos cênicos autênticos para cada
    universo, simulando uma mesa de iluminação profissional (GrandMA / Avolites / QLC+):
      - Universo 0: Dimmers de frente de palco e luz branca/âmbar suave.
      - Universo 1: Ribalta RGB de contra-luz (backlight wash harmônico).
      - Universos 2 e 3: 32 Moving Heads Robóticos com Pan/Tilt de 16 bits.
      - Universos 4 e 5: Fita LED Pixel Mapping com efeito Chase veloz.
      - Universo 6: Strobes de impacto rítmico (blinders de alta energia).
      - Universo 7: Iluminação ambiente da plateia e efeitos gerais.
    """
    buf = bytearray(CHANNELS_PER_UNIVERSE)

    if u_idx == 0:
        # UNIVERSO 0: Frontlights e Dimmers de Palco
        for ch in range(0, CHANNELS_PER_UNIVERSE - 3, 4):
            v = int((math.sin(t * 1.5 + (ch * 0.08)) + 1.0) * 127.5)
            buf[ch]     = v                # Branco frio
            buf[ch + 1] = int(v * 0.85)    # Branco quente
            buf[ch + 2] = int(v * 0.3)     # Âmbar
            buf[ch + 3] = 0

    elif u_idx == 1:
        # UNIVERSO 1: Ribalta Wash RGB de Contra-Luz
        for ch in range(0, CHANNELS_PER_UNIVERSE - 2, 3):
            phase = (ch / 512.0) * 6.283 + (t * 2.0)
            buf[ch]     = int((math.sin(phase) + 1.0) * 127.5)          # Vermelho
            buf[ch + 1] = int((math.sin(phase + 2.09) + 1.0) * 127.5)   # Verde
            buf[ch + 2] = int((math.sin(phase + 4.18) + 1.0) * 127.5)   # Azul

    elif u_idx in (2, 3):
        # UNIVERSOS 2 e 3: Moving Heads Robóticos (32 canais por aparelho)
        # Pan e Tilt de 16 bits (Canal Coarse + Canal Fine)
        offset_u = 0.0 if u_idx == 2 else 3.1415
        for fix in range(16):
            base = fix * 32
            if base + 10 >= CHANNELS_PER_UNIVERSE:
                break
            fix_phase = (fix * 0.392) + offset_u
            
            # Pan 16-bit (Coarse no canal 0, Fine no canal 1)
            pan_val = int((math.sin(t * 1.2 + fix_phase) + 1.0) * 32767.5)
            buf[base + 0] = (pan_val >> 8) & 0xFF
            buf[base + 1] = pan_val & 0xFF
            
            # Tilt 16-bit (Coarse no canal 2, Fine no canal 3)
            tilt_val = int((math.cos(t * 2.4 + fix_phase) + 1.0) * 32767.5)
            buf[base + 2] = (tilt_val >> 8) & 0xFF
            buf[base + 3] = tilt_val & 0xFF
            
            # Dimmer do aparelho (canal 4)
            buf[base + 4] = int((math.sin(t * 1.8 + fix_phase) + 1.0) * 127.5)
            # Shutter aberto (canal 5)
            buf[base + 5] = 255
            # Cores RGB do feixe (canais 6, 7, 8)
            buf[base + 6] = int((math.sin(t * 0.8 + fix_phase) + 1.0) * 127.5)
            buf[base + 7] = int((math.cos(t * 0.8 + fix_phase) + 1.0) * 127.5)
            buf[base + 8] = 220

    elif u_idx in (4, 5):
        # UNIVERSOS 4 e 5: Pixel Mapping / Efeito Chase
        dir_speed = (t * 280.0) if u_idx == 4 else (-t * 280.0)
        for ch in range(CHANNELS_PER_UNIVERSE):
            dist = (ch - (dir_speed % 512)) % 512
            if dist < 70:
                buf[ch] = int(255 * (1.0 - dist / 70.0))
            else:
                buf[ch] = 0

    elif u_idx == 6:
        # UNIVERSO 6: Strobes e Cegadores de Palco (Blinders)
        beat_cycle = t % 3.5
        if beat_cycle < 0.45:
            strobe_flash = 255 if (int(t * 22.0) % 2 == 0) else 0
            for ch in range(CHANNELS_PER_UNIVERSE):
                buf[ch] = strobe_flash
        else:
            for ch in range(CHANNELS_PER_UNIVERSE):
                buf[ch] = 0

    else:
        # UNIVERSO 7: Luz Ambiente da Sala
        ambient = int((math.sin(t * 0.25) + 1.0) * 50.0) + 30
        for ch in range(0, CHANNELS_PER_UNIVERSE, 6):
            buf[ch] = ambient
            buf[ch + 1] = int(ambient * 0.5)

    return bytes(buf)

# =============================================================================
# 6. EXIBIÇÃO DE INFORMAÇÕES DE INÍCIO NO CONSOLE
# =============================================================================
if MODE == "show":
    mode_desc = "SHOW REAL DE PALCO (Moving Heads 16-bit, Wash RGB, Pixels, Dimmers e Strobes)"
elif MODE == "idle":
    mode_desc = "STANDBY DE MESA (DMX contínuo, Dimmers em 0%, Pan/Tilt centralizados)"
elif MODE == "chase":
    mode_desc = "CHASE SEQUENCIAL (Onda de luz correndo pelos 4.096 canais a 15 Hz)"
elif MODE == "random":
    mode_desc = "ESTRESSE MÁXIMO (4.096 canais com ruído aleatório em alta frequência a 60 FPS)"
elif MODE == "ramp":
    mode_desc = "RAMPA LINEAR CONSTANTE (Fader contínuo de 0 a 255)"
elif MODE == "strobe":
    mode_desc = "FLASH STROBE (Todos os 4.096 canais piscando juntos: 255 / 0)"
elif MODE == "rgb":
    mode_desc = "GRADIENTE RGB (Cores harmônicas cruzando todos os universos)"
else:
    mode_desc = f"{MODE.upper()} (Todos os 4.096 canais variam juntos)"

print("=" * 80, flush=True)
print("  TRANSMISSOR ART-NET DMX: 4.096 CANAIS SIMULTÂNEOS (8 UNIVERSOS)", flush=True)
print("=" * 80, flush=True)
print(f"  Destino IP:           {TARGET_IP}:{TARGET_PORT}", flush=True)
print(f"  Universos Ativos:     {universe_list} (8 Universos completos)", flush=True)
print(f"  Total de Canais:      {TOTAL_CHANNELS:,} canais DMX512 transmitidos em uníssono", flush=True)
print(f"  Taxa de Envio:        {TARGET_FPS:.1f} FPS ({int(TARGET_FPS * NUM_UNIVERSES)} pacotes/segundo - {int((TARGET_FPS * NUM_UNIVERSES * 530)/1024)} KB/s)", flush=True)
print(f"  Modo de Simulação:    {mode_desc}", flush=True)
print("=" * 80, flush=True)
print(f"  DICA: Abra http://{TARGET_IP}/monitor no navegador para acompanhar o grid ao vivo!", flush=True)
print("=" * 80 + "\n", flush=True)

# =============================================================================
# 7. MOTOR DE TRANSMISSÃO DE ALTA PRECISÃO (60.00 FPS SEM JITTER / SEM DRIFT)
# =============================================================================
frame_duration = (1.0 / TARGET_FPS) if TARGET_FPS > 0 else 0
start_time = time.perf_counter()
frame_count = 0

total_packets = 0
total_bytes = 0
packets_last_sec = 0
last_report_time = time.time()
seq_counter = 1

try:
    while True:
        # A. CÁLCULO TEMPORAL DO FRAME
        t = frame_count / TARGET_FPS if TARGET_FPS > 0 else (frame_count * 0.016)

        # Contador de sequência ArtDmx de 1 a 255 (sincronizado em todos os 8 pacotes)
        seq_byte = seq_counter & 0xFF
        if seq_byte == 0:
            seq_counter = 1
            seq_byte = 1

        # B. COMPOSIÇÃO DOS CANAIS DE ACORDO COM O MODO SELECIONADO
        if MODE == "show":
            p0[12] = seq_byte; p0[18:530] = generate_show_frame(t, 0)
            p1[12] = seq_byte; p1[18:530] = generate_show_frame(t, 1)
            p2[12] = seq_byte; p2[18:530] = generate_show_frame(t, 2)
            p3[12] = seq_byte; p3[18:530] = generate_show_frame(t, 3)
            p4[12] = seq_byte; p4[18:530] = generate_show_frame(t, 4)
            p5[12] = seq_byte; p5[18:530] = generate_show_frame(t, 5)
            p6[12] = seq_byte; p6[18:530] = generate_show_frame(t, 6)
            p7[12] = seq_byte; p7[18:530] = generate_show_frame(t, 7)
            current_level = p0[18]

        elif MODE == "idle":
            idle_buf = bytearray(CHANNELS_PER_UNIVERSE)
            for fix in range(16):
                base = fix * 32
                if base + 4 < CHANNELS_PER_UNIVERSE:
                    idle_buf[base + 0] = 128  # Pan centralizado
                    idle_buf[base + 2] = 128  # Tilt centralizado
            u_data = bytes(idle_buf)
            current_level = 0
            p0[12] = seq_byte; p0[18:530] = u_data
            p1[12] = seq_byte; p1[18:530] = u_data
            p2[12] = seq_byte; p2[18:530] = u_data
            p3[12] = seq_byte; p3[18:530] = u_data
            p4[12] = seq_byte; p4[18:530] = u_data
            p5[12] = seq_byte; p5[18:530] = u_data
            p6[12] = seq_byte; p6[18:530] = u_data
            p7[12] = seq_byte; p7[18:530] = u_data

        elif MODE == "chase":
            active_ch = int((t * 2048.0) % TOTAL_CHANNELS)
            for u_idx in range(NUM_UNIVERSES):
                buf = bytearray(CHANNELS_PER_UNIVERSE)
                u_start = u_idx * CHANNELS_PER_UNIVERSE
                for ch in range(CHANNELS_PER_UNIVERSE):
                    dist = (ch + u_start - active_ch) % TOTAL_CHANNELS
                    if dist < 120:
                        buf[ch] = int(255 * (1.0 - dist / 120.0))
                packet_buffers[u_idx][12] = seq_byte
                packet_buffers[u_idx][18:530] = buf
            current_level = packet_buffers[0][18]

        elif MODE == "strobe":
            val = 255 if (int(t * 4) % 2 == 0) else 0
            u_data = bytes([val] * CHANNELS_PER_UNIVERSE)
            current_level = val
            p0[12] = seq_byte; p0[18:530] = u_data
            p1[12] = seq_byte; p1[18:530] = u_data
            p2[12] = seq_byte; p2[18:530] = u_data
            p3[12] = seq_byte; p3[18:530] = u_data
            p4[12] = seq_byte; p4[18:530] = u_data
            p5[12] = seq_byte; p5[18:530] = u_data
            p6[12] = seq_byte; p6[18:530] = u_data
            p7[12] = seq_byte; p7[18:530] = u_data

        elif MODE == "rgb":
            r = int((math.sin(t * 3.0) + 1.0) * 127.5)
            g = int((math.sin(t * 3.0 + 2.09) + 1.0) * 127.5)
            b = int((math.sin(t * 3.0 + 4.18) + 1.0) * 127.5)
            u_data = (bytes([r, g, b]) * 171)[:CHANNELS_PER_UNIVERSE]
            current_level = r
            p0[12] = seq_byte; p0[18:530] = u_data
            p1[12] = seq_byte; p1[18:530] = u_data
            p2[12] = seq_byte; p2[18:530] = u_data
            p3[12] = seq_byte; p3[18:530] = u_data
            p4[12] = seq_byte; p4[18:530] = u_data
            p5[12] = seq_byte; p5[18:530] = u_data
            p6[12] = seq_byte; p6[18:530] = u_data
            p7[12] = seq_byte; p7[18:530] = u_data

        elif MODE == "ramp":
            val = int((t * 255.0 * 2.0) % 256)
            u_data = bytes([val] * CHANNELS_PER_UNIVERSE)
            current_level = val
            p0[12] = seq_byte; p0[18:530] = u_data
            p1[12] = seq_byte; p1[18:530] = u_data
            p2[12] = seq_byte; p2[18:530] = u_data
            p3[12] = seq_byte; p3[18:530] = u_data
            p4[12] = seq_byte; p4[18:530] = u_data
            p5[12] = seq_byte; p5[18:530] = u_data
            p6[12] = seq_byte; p6[18:530] = u_data
            p7[12] = seq_byte; p7[18:530] = u_data

        elif MODE == "random":
            import os
            rand_all = os.urandom(TOTAL_CHANNELS)
            p0[12] = seq_byte; p0[18:530] = rand_all[0:512]
            p1[12] = seq_byte; p1[18:530] = rand_all[512:1024]
            p2[12] = seq_byte; p2[18:530] = rand_all[1024:1536]
            p3[12] = seq_byte; p3[18:530] = rand_all[1536:2048]
            p4[12] = seq_byte; p4[18:530] = rand_all[2048:2560]
            p5[12] = seq_byte; p5[18:530] = rand_all[2560:3072]
            p6[12] = seq_byte; p6[18:530] = rand_all[3072:3584]
            p7[12] = seq_byte; p7[18:530] = rand_all[3584:4096]
            current_level = rand_all[0]

        else:
            val = int((math.sin(t * 6.283) + 1.0) * 127.5)
            u_data = bytes([val] * CHANNELS_PER_UNIVERSE)
            current_level = val
            p0[12] = seq_byte; p0[18:530] = u_data
            p1[12] = seq_byte; p1[18:530] = u_data
            p2[12] = seq_byte; p2[18:530] = u_data
            p3[12] = seq_byte; p3[18:530] = u_data
            p4[12] = seq_byte; p4[18:530] = u_data
            p5[12] = seq_byte; p5[18:530] = u_data
            p6[12] = seq_byte; p6[18:530] = u_data
            p7[12] = seq_byte; p7[18:530] = u_data

        seq_counter = (seq_counter % 255) + 1

        # C. DISPARO EM RAJADA ATÔMICA (BURST) - 8 PACOTES CONSECUTIVOS SEM INTERVALO
        sock.sendto(p0, DEST)
        sock.sendto(p1, DEST)
        sock.sendto(p2, DEST)
        sock.sendto(p3, DEST)
        sock.sendto(p4, DEST)
        sock.sendto(p5, DEST)
        sock.sendto(p6, DEST)
        sock.sendto(p7, DEST)

        total_packets += 8
        packets_last_sec += 8
        total_bytes += (8 * 530)
        frame_count += 1

        # D. CONTROLE DE FREQUÊNCIA DE PRECISÃO CIRÚRGICA (SPIN-WAIT NO ÚLTIMO 1.5 MS)
        # O time.sleep convencional é usado para a maior parte do intervalo (liberando CPU),
        # enquanto o micro-segundo final é sincronizado por busy-wait (perf_counter).
        if frame_duration > 0:
            target_next_tick = start_time + frame_count * frame_duration
            now = time.perf_counter()
            remaining = target_next_tick - now
            if remaining > 0.0015:
                time.sleep(remaining - 0.001)
            while time.perf_counter() < target_next_tick:
                pass

        # E. TELEMETRIA CONSOLIDADA NO TERMINAL A CADA 1.0 SEGUNDO
        now_wall = time.time()
        if now_wall - last_report_time >= 1.0:
            real_fps = packets_last_sec / NUM_UNIVERSES
            kbps = (packets_last_sec * 530) / 1024.0
            print(f"[STATUS] {real_fps:5.1f} FPS/Univ ({packets_last_sec:5d} Pkt/s - {kbps:6.1f} KB/s) | "
                  f"4.096 canais a {real_fps:4.1f} Hz reais | Nivel DMX: {current_level:3d}/255 | Total: {total_packets:,} pacotes", flush=True)
            packets_last_sec = 0
            last_report_time = now_wall

except KeyboardInterrupt:
    print("\n" + "=" * 80, flush=True)
    print("  TRANSMISSÃO FINALIZADA PELO USUÁRIO (Ctrl+C)", flush=True)
    print(f"  Total de Pacotes Enviados:  {total_packets:,} ({total_bytes / (1024 * 1024):.2f} MB)", flush=True)
    print(f"  Total de Frames Transmitidos: {frame_count:,} frames de 4.096 canais", flush=True)
    print("=" * 80, flush=True)
