# How to compile fdbus pb-example

## Deps
before compiling, make sure the follwing tools installed.

`sudo apt-get install autoconf automake libtool curl make g++ unzip`

## Protobuf
need to build protobuf compiler and shared libs first.

please refer to the following commands:

```shell
PKG_FOLDER=~/Downloads/protobuf
INSTALL_PREFIX=$HOME/.local

if [ -d $PKG_FOLDER ]; then
	echo "$PKG_FOLDER already exists"
	cd $PKG_FOLDER
	git reset --hard HEAD && git checkout 3.20.x # tested working
else
	git clone -b 3.20.x https://github.com/protocolbuffers/protobuf.git $PKG_FOLDER
	cd $PKG_FOLDER
fi

git submodule update --init --recursive
cd cmake && mkdir -p build && cd build
cmake -DCMAKE_CXX_STANDARD=14 -DCMAKE_INSTALL_PREFIX=$INSTALL_PREFIX -Dprotobuf_BUILD_TESTS=OFF -DBUILD_SHARED_LIBS=1 ..
make -j8 install

echo "export PATH=$INSTALL_PREFIX/bin:\$PATH" >> ~/.bashrc
source ~/.bashrc
protoc --version # need to make sure protoc can be found
```

## Fdbus
```shell
PKG_FOLDER=~/Downloads/fdbus
INSTALL_PREFIX=$HOME/.local

if [ -d $PKG_FOLDER ]; then
	echo "$PKG_FOLDER already exists"
	cd $PKG_FOLDER
else
	git clone https://github.com/jeremyczhen/fdbus.git $PKG_FOLDER
	cd $PKG_FOLDER
fi

mkdir -p build/install
cd build
# install fdbus into build/install, not INSTALL_PREFIX
cmake -DSYSTEM_ROOT=$INSTALL_PREFIX -DCMAKE_INSTALL_PREFIX=install ../cmake
make -j8 install
```

## Fdbus pb-example
```shell
PKG_FOLDER=~/Downloads/fdbus
INSTALL_PREFIX=$HOME/.local
cd $PKG_FOLDER && mkdir -p build-pb-example/install && cd build-pb-example
cmake -DSYSTEM_ROOT="$INSTALL_PREFIX;~/Downloads/fdbus/build/install" -DCMAKE_INSTALL_PREFIX=install ../cmake/pb-example
make -j8 install
```

## Run
```shell
cd ~/Downloads/fdbus/build && ./name_server &

cd ~/Downloads/fdbus/build-pb-example && ./fdbservertest my_test &

cd ~/Downloads/fdbus/build-pb-example && ./fdbclienttest my_test &

cd ~/Downloads/fdbus/build && ./logsvc
```

## Appendix
qnx side:
```
-Dfdbus_LOG_TO_STDOUT=ON
-Dfdbus_BUILD_JNI=OFF
-Dfdbus_SECURITY=ON
-Dfdbus_UDS_ABSTRACT=ON
-Dfdbus_ALLOC_PORT_BY_SYSTEM=ON
-Dfdbus_USING_ZIP=ON
-Dfdbus_QNX_DIRENT=ON
-Dfdbus_QNX_KEEPALIVE=ON
```

linux side:
```
-Dfdbus_BUILD_JNI=OFF
-Dfdbus_QNX_DIRENT=OFF
-Dfdbus_QNX_KEEPALIVE=OFF
```
