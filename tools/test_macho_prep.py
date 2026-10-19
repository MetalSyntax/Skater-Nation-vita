#!/usr/bin/env python3
"""Regresión de tools/macho_prep.py con los casos de Skater Nation vistos en consola.

Uso: python3 -I tools/test_macho_prep.py [Executable]   (por defecto game_app/SkateNation)
"""
import os, struct, subprocess, sys, tempfile

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
BIN = sys.argv[1] if len(sys.argv) > 1 else os.path.join(ROOT, 'game_app', 'SkateNation')

CASES = {
    0x333c: True,     # corrida 6: literal absoluto del primer constructor
    0x902c: False,    # corrida 7: offset PIC (ldr r2, [pc, r2]) a >8 instrucciones
    0x3493cc: True,   # corrida 10: tabla de switch de CIrrDeviceIPhoneOS (primera entrada)
    0x349428: True,   # ... última entrada (cmp r3, #24 -> 25 entradas)
    0x2afd60: True,   # corrida 12: vtable de CImage; r3 se reutiliza luego como índice de switch
    0x2b0158: True,   # ídem, segundo constructor de CImage
    0x1264f4: False,  # corrida 16: firma 'FWS' de gameswf (cmp), constante en rango de la imagen
    0x1264f8: False,  # ídem 'CWS'
    0x7b758: True,    # cmp con puntero y después base de ldr: sigue siendo puntero
    0x2f1568: True,   # comparación con typeinfo (RTTI): puntero alineado
    0x51b01c: True,   # __mod_init_func: el loader confía en rebase.bin (convención ZI)
}

with tempfile.TemporaryDirectory() as d:
    out = os.path.join(d, 'rebase.bin')
    subprocess.run([sys.executable, '-I', os.path.join(ROOT, 'tools', 'macho_prep.py'), BIN, out],
                   check=True, stdout=subprocess.DEVNULL)
    b = open(out, 'rb').read()
    rebase = set(struct.unpack_from(f'<{struct.unpack_from("<I", b, 4)[0]}I', b, 8))
bad = [f'{a:#x} esperado {"en" if want else "fuera de"} rebase.bin' for a, want in CASES.items() if (a in rebase) != want]
print('\n'.join(bad) or f'macho_prep ok ({len(CASES)} casos)')
sys.exit(1 if bad else 0)
