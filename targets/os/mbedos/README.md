### About

This folder contains files to run JJS on
[FRDM-K64F board](https://os.mbed.com/platforms/frdm-k64f/) with
[Mbed OS](https://os.mbed.com/).
The document had been validated on Ubuntu 20.04 operating system.

#### 1. Setup the build environment

Clone the necessary projects into a `jjs-mbedos` directory.
The latest tested working version of Mbed is `mbed-os-6.15.0`.

```sh
mkdir jjs-mbedos && cd jjs-mbedos

git clone https://github.com/LightSourceEngine/jjs.git
git clone https://github.com/ARMmbed/mbed-os.git -b mbed-os-6.15.0
```

The following directory structure has been created:

```
jjs-mbedos
  + jjs
  |  + targets
  |      + os
  |        + mbedos
  + mbed-os
```

#### 2. Install dependencies of the projects

```sh
# Assuming you are in jjs-mbedos folder.
jjs/tools/apt-get-install-deps.sh

sudo apt install stlink-tools
pip install --user mbed-cli
# Install Python dependencies of Mbed OS.
pip install --user -r mbed-os/requirements.txt
```

#### 3. Build Mbed OS (with JJS)

```
# Assuming you are in jjs-mbedos folder.
make -C jjs/targets/os/mbedos MBED_OS_DIR=${PWD}/mbed-os
```

The created binary is a `mbedos.bin` named file located in `jjs/build/mbed-os` folder.

#### 4. Flash

Connect Micro-USB for charging and flashing the device.

```
# Assuming you are in jjs-mbedos folder.
make -C jjs/targets/os/mbedos MBED_OS_DIR=${PWD}/mbed-os flash
```

#### 5. Connect to the device

The device should be visible as `/dev/ttyACM0` on the host.
You can use `minicom` communication program with `115200` baud rate.

```sh
sudo minicom --device=/dev/ttyACM0 --baud=115200
```

Set `Add Carriage Ret` option in `minicom` by `CTRL-A -> Z -> U` key combinations.
Press `RESET` on the board to get the output of JJS application:

```
This test run the following script code: [print ('Hello, World!');]

Hello, World!
```
