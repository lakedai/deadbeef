### change to get artwork from local album image file
- if no cover image, load distinct file from album name
- put album.jpg or album.png in that folder, that is all

### changed src files:
- 修改：     plugins/artwork-legacy/artwork.c

### install instruction:
	sudo apt install deadbeef
	./autogen.sh
	CC=clang CXX=clang++ ./configure --prefix=/tmp --disable-adplug
	make
	make install
	sudo cp ~/tmp/lib/deadbeef/artwork.so /opt/deadbeef/lib/deadbeef/

### build required:(ubuntu20.04)
- clang
- libjansson-dev
- libimlib2-dev
- libcurl4-gnutls-dev
- libdispatch-dev
