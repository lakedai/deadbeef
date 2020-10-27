### change to get artwork from local album image file
- if no cover image, load distinct file from album name
- the song must have artist/album/title - use album for search
- or artist/title - use title as album for search
- put album.jpg or album.png or title.jpg or title.png in that folder, that is all

### changed src files:
- 修改：     plugins/artwork-legacy/artwork.c

### install instruction:
	sudo apt install deadbeef
	./autogen.sh
	CC=clang CXX=clang++ ./configure --prefix=/tmp --disable-adplug
	make
	sudo make install
	sudo cp ~/tmp/lib/deadbeef/artwork.so /opt/deadbeef/lib/deadbeef/

### build required:(ubuntu20.04)
- clang
- libjansson-dev
- libimlib2-dev
- libcurl4-gnutls-dev
- libdispatch-dev
