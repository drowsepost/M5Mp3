# M5 AMP for Cardputer + Adv(JPN)
VolosR's M5Mp3 fork for Cardputer and Cardputer Adv.

This is a test code for simply using the earphone jack of Cardputer Adv. You may be able to feel the potential of Cardputer Adv as a multimedia device.

8px was too small for kanji, so I adjusted the layout to use 12px characters, but since the font is changeable, you may find it more comfortable to change it to the font of your language.

I calling Speaker.begin() to enable output from the ES8311, but it may be possible to do this with simpler code.

- Support M5Stack Cardputer and Cardputer Adv
- Update Library(ESP32-audioI2S version 3.2.1)
- add support japanese font (Non-symbols)
- add support ID3 Title (Only UTF-8)
- Accurate Playback Time
- Code and Library cleanup
- Usability adjustments

## Usage
1. Store MP3 files in the "/music/" folder on your SD card.
2. When launched, it parses the "/music/" folder and displays a list of files.(Up to 128 files)
3. Use the Up and Down arrow keys to select a file and press Enter to start playback.

- Press Left and Right arrow keys to adjust the volume in one-level increments.(My personal preference is to use headphones at level 3.)
- Press L key to adgust BackLight
- Press N to play the next file in the current song on Filelist
- Press P to play the prev file in the current song on Filelist
- Press B to play the Random song

If esp-idf automatically resets the device while it is playing music, you may not be able to access the SD card until you restart the Cardputer.
This can be avoided by stopping playback with the A key before writing data.

## Build on Arduino
### Setting
- IDE: 2.3.6
- Board: M5Stack 3.2.2
- PartitionScheme: HugeApp(3MB No OTA/ 1MB SPIFFS)

### Library
- M5Cardputer 1.1.0
- schreibfaul1/ESP32-audioI2S-master 3.2.1 (GPL v3)

## bug
Playing mp3 files encoded with older versions of iTunes(ver 6-7) or files that contain data outside the specifications can cause the system to become unstable. This can sometimes be avoided by re-encoding with the latest LAME.

example: 
```
ffmpeg -i input.mp3 -codec:a libmp3lame -b:a 128k fixed.mp3
```

When you play a file that has ID3 data in an old character encoding (such as Shift JIS), you may see the nostalgic garbled characters.

## Watch VolosR's Original Video
https://www.youtube.com/watch?v=bU9YBojKZzc
