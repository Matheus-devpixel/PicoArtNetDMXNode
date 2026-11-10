"""
============================================================================
PROJETO: PICO ARTNET DMX NODE (RP2040)
ARQUIVO: compile_targets.py
AUTOR: Matheus Dev (@Matheus-devpixel)
DESCRIÇÃO: Script de automação de compilação cruzada para o Raspberry Pi RP2040.
           Monta drive virtual P: (para contornar caracteres especiais no path),
           chama CMake + Ninja com a toolchain ARM (arm-none-eabi-gcc 14.2),
           compila o firmware bare-metal e converte o binário ELF/BIN para UF2
           pronto para ser arrastado e solto na unidade RPI-RP2.
============================================================================
"""

import os, subprocess, shutil, stat, sys, struct

def remove_readonly(func, path, excinfo):
    os.chmod(path, stat.S_IWRITE)
    func(path)

PROJECT_DIR = r"F:\PROJETOS EMBARCADOS\pi pico w dmx artnet C#\artnet_dmx"
OUTPUT_DIR = os.path.join(PROJECT_DIR, "binarios_finais")
os.makedirs(OUTPUT_DIR, exist_ok=True)

# Garante ferramentas no PATH do processo
extra_paths = [
    r"F:\AndroidSDK\cmake\3.22.1\bin",
    r"F:\tools\arm-none-eabi\bin",
    r"F:\.home\w64devkit\bin",
    r"C:\Program Files\Inkscape\bin"
]
os.environ['PICO_SDK_PATH'] = r'F:\tools\pico-sdk'
os.environ['PICO_TOOLCHAIN_PATH'] = r'F:\tools\arm-none-eabi'
os.environ['PATH'] = ";".join(extra_paths) + ";" + os.environ.get('PATH', '')

cmake_bin = shutil.which("cmake") or r"F:\AndroidSDK\cmake\3.22.1\bin\cmake.exe"
ninja_bin = shutil.which("ninja") or r"F:\AndroidSDK\cmake\3.22.1\bin\ninja.exe"

def bin_to_uf2(bin_path, uf2_path, base_addr=0x10000000, family_id=0xe48bff56):
    with open(bin_path, 'rb') as f:
        data = f.read()
    num_blocks = (len(data) + 255) // 256
    with open(uf2_path, 'wb') as f:
        for block_no in range(num_blocks):
            chunk = data[block_no*256 : (block_no+1)*256]
            chunk = chunk.ljust(256, b'\x00')
            header = struct.pack('<IIIIIIII',
                0x0A324655,
                0x9E5D5157,
                0x00002000,
                base_addr + block_no * 256,
                256,
                block_no,
                num_blocks,
                family_id
            )
            padding = b'\x00' * (476 - 256)
            footer = struct.pack('<I', 0x0AB16F30)
            f.write(header + chunk + padding + footer)

def setup_subst_drive():
    # Cria unidade virtual P: para contornar problemas de caracteres especiais (# no nome da pasta) no CMake/Ninja
    if not os.path.exists("P:\\CMakeLists.txt"):
        subprocess.run(["subst", "P:", PROJECT_DIR], capture_output=True)
    if os.path.exists("P:\\CMakeLists.txt"):
        return "P:\\"
    return PROJECT_DIR

def build_universal():
    print("=" * 65)
    print("COMPILANDO FIRMWARE DEDICADO USB ETHERNET (RNDIS / CDC-ECM)")
    print("=================================================================")
    
    work_dir = setup_subst_drive()
    build_dir = os.path.join(work_dir, "build_universal")
    os.makedirs(build_dir, exist_ok=True)

    cmake_cmd = [
        cmake_bin, "-G", "Ninja",
        "-DPICO_BOARD=pico",
        f"-DCMAKE_MAKE_PROGRAM={ninja_bin}",
        work_dir
    ]
    print("Configurando com CMake...")
    p = subprocess.Popen(cmake_cmd, cwd=build_dir)
    if p.wait() != 0:
        print("ERRO: Falha ao configurar CMake")
        return False

    print("Compilando com Ninja...")
    p = subprocess.Popen([ninja_bin], cwd=build_dir)
    if p.wait() != 0:
        print("ERRO: Falha ao compilar com Ninja")
        return False

    bin_src = os.path.join(build_dir, "artnet_dmx.bin")
    uf2_src = os.path.join(build_dir, "artnet_dmx.uf2")
    if not os.path.exists(bin_src):
        print(f"ERRO: Arquivo BIN nao foi gerado em {bin_src}")
        return False

    print("Convertendo binario para formato UF2 (RP2040)...")
    bin_to_uf2(bin_src, uf2_src)

    # Limpa arquivos UF2 antigos na pasta de distribuição
    for f in os.listdir(OUTPUT_DIR):
        if f.endswith(".uf2"):
            try:
                os.remove(os.path.join(OUTPUT_DIR, f))
            except Exception:
                pass

    dest_path = os.path.join(OUTPUT_DIR, "artnet_dmx.uf2")
    shutil.copy2(uf2_src, dest_path)
    
    print("\n" + "=" * 65)
    print("COMPILACAO CONCLUIDA COM SUCESSO!")
    print("=================================================================")
    print(f"  [+] artnet_dmx.uf2 ({os.path.getsize(dest_path):,} bytes)")
    print("=================================================================")
    return True

ok = build_universal()
if not ok:
    sys.exit(1)
