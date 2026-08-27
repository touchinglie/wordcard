# wordcard
A wordcard application based on GTK Lib

## complie

```bash
git clone https://github.com/touchinglie/wordcard.git
cd wordcard
source build.sh
./wordsCard
```

## build the application yourself

Make sure you have the newest requirements to pack AppImage

Here will give the requirements on Ubuntu24.04

```bash
sudo apt update
sudo apt install -y build-essential cmake pkg-config \
    libgtk-4-dev libglib2.0-dev libcairo2-dev libpango1.0-dev \
    wget unzip file patchelf
```

Complie as usual

```bash
git clone https://github.com/touchinglie/wordcard.git
cd wordcard
source build.sh
cp ./wordsCard ../AppDir/usr/bin/
```

Then you will need to pack libharfbuzz.so.0 with elf manually.

```bash
cd ..
wget -c "https://github.com/linuxdeploy/linuxdeploy/releases/download/continuous/linuxdeploy-x86_64.AppImage"
chmod +x linuxdeploy-x86_64.AppImage
wget http://archive.ubuntu.com/ubuntu/pool/main/h/harfbuzz/libharfbuzz0b_8.3.0-2build2_amd64.deb
dpkg -x libharfbuzz0b_8.3.0-2_amd64.deb ./harfbuzz_extract
cd harfbuzz_extract/usr/lib/x86_64-linux-gnu/
cp -r ./libharfbuzz.so.0* ../../../../AppDir/usr/lib
cd ../../../..
cp -r ./words ./AppDir/
./linuxdeploy-x86_64.AppImage --appdir ./AppDir --executable ./AppDir/usr/bin/wordsCard --icon-file ./AppDir/usr/share/icons/hicolor/256x256/apps/wordsCard.png --desktop-file ./AppDir/usr/share/applications/wordsCard.desktop   --output appimage
```
