### About

This folder contains files to run JJS on
[STM32F4-Discovery board](https://www.st.com/en/evaluation-tools/stm32f4discovery.html) with
[RIOT](https://www.riot-os.org/).
The document had been validated on Ubuntu 20.04 operating system.

### How to build

#### 1. Setup the build environment for STM32F4-Discovery board

Clone the necessary projects into a `jjs-riot` directory.
The latest tested working version of RIOT is `2021.10`.

```sh
# Create a base folder for all the projects.
mkdir jjs-riot && cd jjs-riot

git clone https://github.com/LightSourceEngine/jjs.git
git clone https://github.com/RIOT-OS/RIOT.git -b 2021.10
```

#### 2. Install dependencies of the projects

```
# Assuming you are in jjs-riot folder.
jjs/tools/apt-get-install-deps.sh

sudo apt install gcc-arm-none-eabi openocd minicom
```

The following directory structure has been created:

```
jjs-riot
  + jjs
  |  + targets
  |      + os
  |        + riot
  + RIOT
```

#### 3. Build RIOT (with JJS)

```
# Assuming you are in jjs-riot folder.
make BOARD=stm32f4discovery -f jjs/targets/os/riot/Makefile
```

The created binary is a `riot_jjs.elf` named file located in `jjs/build/riot-stm32f4/bin/` folder.

#### 4. Flash the device

Connect Mini-USB for charging and flashing the device.

```
# Assuming you are in jjs-riot folder.
make BOARD=stm32f4discovery -f jjs/targets/os/riot/Makefile flash
```

Note: `ST-LINK` also can be used that is described at [this page](https://github.com/RIOT-OS/RIOT/wiki/ST-LINK-tool).

#### 5. Connect to the device

Use `USB To TTL Serial Converter` for serial communication. STM32F4-Discovery pins are mapped by RIOT as follows:

```
  STM32f4-Discovery PA2 pin is configured for TX.
  STM32f4-Discovery PA3 pin is configured for RX.
```

* Connect `STM32f4-Discovery` **PA2** pin to **RX** pin of `USB To TTL Serial Converter`
* Connect `STM32f4-Discovery` **PA3** pin to **TX** pin of `USB To TTL Serial Converter`
* Connect `STM32f4-Discovery` **GND** pin to **GND** pin of `USB To TTL Serial Converter`

The device should be visible as `/dev/ttyUSB0`. Use `minicom` communication program with `115200`.

* In `minicom`, set `Add Carriage Ret` to `off` in by `CTRL-A -> Z -> U` key combinations.
* In `minicom`, set `Hardware Flow Control` to `no` by `CTRL-A -> Z -> O -> Serial port setup -> F` key combinations.

```sh
sudo minicom --device=/dev/ttyUSB0 --baud=115200
```

RIOT prompt looks like as follows:

```
main(): This is RIOT! (Version: 2021.10)
You are running RIOT on a(n) stm32f4discovery board.
This board features a(n) stm32 MCU.
>
```

You may have to press `RESET` on the board and press `Enter` key on the console several times to make RIOT prompt visible.

#### 6. Run JJS

Type `help` to list shell commands:

```
> help
Command              Description
---------------------------------------
test                 JJS Hello World test
reboot               Reboot the node
version              Prints current RIOT_VERSION
pm                   interact with layered PM subsystem
```

Type `test` to execute JJS:

```
> test
This test run the following script code: [print ('Hello, World!');]

Hello, World!
```
