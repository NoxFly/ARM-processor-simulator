# ARM processor simulator - PROG5 project

# About

This is a university project (first semester of third year).

This project is done by a group of 6 members.

__Our members__ :

[Dorian Thivolle](https://github.com/NoxFly/), [Dorian Lorek](https://github.com/EloGamerr), [Lilian Russo](https://github.com/Leer0r),
[Clément Corbillé](https://github.com/corbillc), [Etienne Gabert](https://github.com/MiniGabert), [Arthur Ducros](https://github.com/SpyciBear).

__Roles repartition__ :

Dorian Thivolle : Lead developer

All others : developers

## Branches

For each modified file (.c), we created a branch.

We're merging each of these branches on the dev's one, to test if it is working, then we merge it on master in that case.

## Execute the code

The given code in this repository isn't complete. We've added some folders in the gitignore, including the Makefile, because these are specific configured folders / file depending of user's system.

If you have the missing folders + Makefile, you can execute the code doing the following steps :

- __Ensure you have a cross compiler__: install it with these two cli commands :
    - `sudo apt-get install libc6-armel-cross libc6-dev-armel-cross binutils-arm-linux-gnueabi libncurses5-dev build-essential bison flex libssl-dev bc`
    - `sudo apt-get install gcc-arm-linux-gnueabi g++-arm-linux-gnueabi`
- __Configure and compile the code__ : go on `arm_simulator-1.4` folder, then write :
    - `./configure CFLAGS='-Wall -Werror -g'`
    - then if it succeed, write `make`
- Finally, __execute the created executable__ : `./arm_simulator`

Once the simulator is started, you'll see 2 port's numbers.

Open a second terminal, and do following steps to connect to the server side :
- write `gdb-multiarch` on the same folder
- write `file Examples/example1` (for example, to focus exemple1 file)
- write `target remote localhost:<simulator's given port>`
- to step forwards, do `stepi`

## License

This code has a [MIT License](https://github.com/NoxFly/ARM-processor-simulator/blob/master/License).