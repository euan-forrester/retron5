# Retron5 save file converter

Convert Retron5 save files to and from files that can be used by other emulators

## Intro

If you have a bunch of cartridges from retro videogame systems like the NES, SNES, Gameboy, etc. then your save files may get lost as these 30 year old batteries slowly die.

The Retron5 may be the most cost-effective machine that can read a large number of different cartridge types, and one of its little-known features is that it can copy save game data from a cartridge to an SD card and back again.

But the data is in a proprietary format, meaning that we're tied to our Retron5 and if it dies then we're back to square one. 

This script will convert this proprietary format into the more common format used by other emulators such as those included with the RetroPie, and also back again if we want to load our emulator saves onto a real cartridge.

So not only can we back up our save files, but then we can swap out those dying batteries and put our save files back onto our cartridges to last another 30 years.

## Data format

The Retron5 save format is described here: https://www.retro5.net/viewtopic.php?f=5&t=67&start=10

And specifically the file format is:

```
typedef struct
{
   uint32_t magic;
   uint16_t fmtVer;
   uint16_t flags;
   uint32_t origSize;
   uint32_t packedSize;
   uint32_t dataOffset;
   uint32_t crc32;
   uint8_t data[0];
} t_retronDataHdr;
```

It's a header and then a bunch of compressed data, so to unpack the file we need to read past the header and then uncompress the data.

## Sample calls

Convert Retron5 save to emulator:

```
./retron5.py -i retron-saves-in/The\ Legend\ of\ Zelda\ -\ Oracle\ of\ Seasons\ \(U\)\ \[C\]\[\!\].sav -o emulator-saves-out/ -d
```

Convert emulator save to Retron5:

```
./retron5.py -i emulator-saves-in/The\ Legend\ of\ Zelda\ -\ Oracle\ of\ Seasons\ \(U\)\ \[C\]\[\!\].srm -o retron-saves-out/ -t -d
```