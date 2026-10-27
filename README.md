<p align="center"><img src="banner.png" width="720" alt="ORYMY Bass"></p>

# ORYMY Bass — YMY Sounds VST

Amplificador de bajo **VST3** para **bachata y merengue** (Windows 64-bit).
Cadena completa: Noise Gate → Compresor → Bass Tone → Split (cruce LR4) + Sub-octava → Drive/Ampli (2x oversampling) → EQ gráfico de 11 bandas (Auto / Bachata / Merengue / Custom) → Cabina → Modulación (Chorus, Flanger, Phaser, Tremolo) → Delay (libre o sincronizado al tempo) → Reverb (Room, Hall, Plate, Spring).

**Presets:** Bachata Antony Santos · Raulín Rodríguez · Romeo Santos · Max Santos Bass · Slap Bachata · Mambo Clarity · Bajo Clásico · Bajo Moderno · Bajo Profundo · Bajo Suelto · Bachata Romántica · Merengue Brillante · Merengue Clásico · Urbano Bass · Personalizado (botón *Save Preset* guarda tu sonido en `Documentos\YMY Sounds\ORYMY Bass\Personalizado.xml`).

**Detalles en vivo:** espectro de audio detrás del EQ, osciloscopio, bocina que se mueve con el nivel, luces en el ampli/gate/compresor, ilustración que aparece al tocar (clic sobre ella para dejarla fija), detección de nota (YIN) y de escala, diapasón con la nota iluminada y veredicto **TOCANDO BIEN / FUERA DE ESCALA / DESAFINADO**.

## Compilar en GitHub (sin instalar nada)
1. Crea un repositorio nuevo y sube **todo** el contenido de esta carpeta (incluida `.github`).
2. Pestaña **Actions** → *Build ORYMY Bass* se ejecuta solo (o *Run workflow*).
3. Al terminar, descarga el artefacto **ORYMY-Bass-Windows**: `ORYMY_Bass_Setup_1.0.0.exe` + `ORYMY_Bass_VST3.zip`.
4. Para publicar un Release con el instalador: `git tag v1.0.0 && git push --tags`.

## Compilar en tu PC
```
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release
```
Resultado en `build/ORYMYBass_artefacts/Release/` (VST3 y Standalone). El instalador se genera con Inno Setup 6: `ISCC /DBuildDir=<ruta> Installer\setup.iss`.

## Regenerar imágenes
`python tools/make_assets.py` (requiere `pillow` y `numpy`) recorta logos/ilustración de `tools/reference_ui.png` y rehace las imágenes del instalador.

## Licencia de JUCE
El plugin usa [JUCE](https://juce.com). Para distribuirlo cerrado necesitas una licencia comercial de JUCE; con la licencia gratuita/AGPL el código debe ser abierto.
