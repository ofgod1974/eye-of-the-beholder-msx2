# Colonna sonora originale e audio MSX2

## Che cosa si sente

La demo riproduce **Character Generation**, composto da **Paul Mudra** per
Eye of the Beholder PC DOS, come sottofondo continuo dell'esplorazione.
Alla vittoria passa a **Ending Theme**. Non è una nuova composizione.

L'uso del tema durante l'esplorazione è una scelta di questa conversione:
nella versione PC originale la musica accompagnava logo, introduzione,
creazione personaggi e finale; nel dungeon erano presenti gli effetti.

L'introduzione usa anche Westwood Logo, Intro Part 1 e Intro Part 2. Nei menu parte Character Generation; il tema riparte entrando nel dungeon.

## Avvio e comandi

- `Gioca.cmd`: MSX2 di base; usa il PSG, senza espansioni richieste.
- `Gioca SCC.cmd`: aggiunge un SCC emulato a cinque voci.
- `Gioca FM-PAC.cmd`: aggiunge un FM-PAC emulato.
- `Gioca OPL4 FM.cmd`: aggiunge un OPL4 emulato e usa la sezione FM per il log AdLib.
- **U**: musica on/off. Quando viene riattivata, il tema riparte dall'inizio.
- **B**: passa fra il PSG e i chip rilevati; il brano riparte al cambio di chip.
- Il nome del chip attivo è indicato in basso a destra.

Sull'hardware reale la ROM rileva automaticamente i dispositivi disponibili:
FM OPL3/OPL4 compatibile ai port C4/C5, poi FM-PAC/MSX-MUSIC, SCC, infine PSG.
Il tasto B consente di scegliere anche il PSG quando sono disponibili espansioni.
Non sono state effettuate prove su hardware fisico.

Le estensioni emulatore fornite dagli script non richiedono firmware commerciale:
FM-PAC usa solo una ROM generata con la firma di rilevamento, perché il gioco pilota
il chip direttamente; OPL4 usa una ROM campioni vuota perché questa versione usa
esclusivamente la sezione FM. Non sono incluse né usate banche PCM/General MIDI.

## Differenze di resa

| Chip | Riproduzione |
|---|---|
| PSG | Riduzione delle note originali a tre voci, dando priorità ad acuti e basso; timbro a onda quadra |
| SCC | Riduzione a cinque voci con wavetable morbida |
| FM-PAC | Note originali su nove voci, con strumenti OPLL sostitutivi |
| OPL4 FM | Riproduzione degli aggiornamenti dei registri AdLib originali sulla sezione FM compatibile, quantizzati a 50 Hz |

PSG, SCC e FM-PAC sono adattamenti della partitura, non copie del timbro AdLib.
OPL4 offre la resa FM più vicina al log originale; non vengono usati i suoi campioni.
Il backend FM è compatibile anche con una scheda OPL3 ai port MoonSound C4/C5.
Gli effetti di passi, urto, colpo, uccisione, chiave, porta, cura, vittoria e morte
sono sintesi PSG nuove, collegate agli eventi del gioco: non sono rip originali.

## Ascolto

`music-psg.wav`, `music-scc.wav`, `music-fmpac.wav` e `music-opl4.wav` sono estratti
musicali di circa 20 secondi registrati dall'emulazione della ROM, senza effetti.
La registrazione è presa nella parte melodica del brano, dopo l'introduzione.
I livelli sono quelli del mix emulato, senza normalizzazione successiva.

## Origine dei dati

Rip musicale recuperato da VGMPF:
https://www.vgmpf.com/Wiki/index.php?title=Eye_of_the_Beholder_(DOS)
https://www.vgmpf.com/Wiki/images/3/38/Eye_of_the_Beholder_%28DOS%29.zip

Il campo GD3 del VGM di Character Generation indica Paul Mudra come autore e
Doommaster1994 come autore del rip. Le cinque tracce originali sono conservate
in assets/audio. Un altro catalogo del rip PC, di MusicFox, è disponibile su:
https://vgmrips.net/packs/pack/eye-of-the-beholder-ibm-pc-at

Gli ADL del pacchetto VGMPF coincidono con i DAT già estratti dai nostri floppy:

| File rip | File dei dischi | MD5 comune |
|---|---|---|
| ADLIB.ADL | ADLIB.DAT | e0c0f134185e8b99bc7fea4665c9df17 |
| SOUND.ADL | SOUND.DAT | 370880f4892e43f7c1bf527e152ff76d |

Il convertitore legge il VGM e genera flussi di eventi a 50 Hz. Non trascrive la
musica a orecchio. I registri di frequenza e key-on forniscono le note; la versione
OPL usa direttamente i registri originali. Gli SHA-256 dei cinque VGM effettivamente
riprodotti sono conservati in assets/generated/music.json.

## Implementazione e verifiche

- Sequencer e mixer sull'hook BIOS H.TIMI, con adattamento a 50 o 60 Hz.
- Dati musicali nella MegaROM a partire da 0x1E0000; il driver salva e ripristina
  il banco grafico a ogni tick, senza interferire con la decompressione della vista.
- Attesa VDP con interrupt abilitati: il tempo musicale continua durante le copie.
- Font originale FONT6.FNT convertito in VRAM, per evitare lunghe stampe BIOS.
- Gli effetti PSG usano temporaneamente il terzo canale e poi restituiscono la voce
  alla musica. Con le espansioni il PSG rimane disponibile per gli effetti.

`powershell -NoProfile -ExecutionPolicy Bypass -File scripts/verify-audio.ps1`

Nove controlli per ciascuno dei quattro backend: rilevamento/fallback, tick in
attesa di input, continuità durante il rendering, loop della partitura, tempo PAL,
mute, riattivazione e cambio/ripristino del backend. Risultati in build/audio-test.
Le prove FM-PAC e OPL4 usano i dispositivi sintetici di openMSX con dati ROM di test.

Le quattro registrazioni sono non silenziose e prive di clipping digitale.
Passano anche i 24 controlli di gioco, incluso il percorso completo fino alla vittoria.

Riferimenti per le interfacce hardware:
https://www.msx.org/wiki/MSX-MUSIC_programming
https://map.grauw.nl/resources/msx_io_ports.php

