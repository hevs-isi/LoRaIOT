# LoRaIOT
One platform to rule them all

# Testing Zephyr OS on ST nucleo l432kc

## Setup zephyr os and zephyr SDK
1. Follow the original [getting-started](https://docs.zephyrproject.org/latest/getting_started/getting_started.html),
up to and including the **Bootstrap west** step.

1. install some more tools:
    ```
    # For the tests and CodingStyle
    sudo apt-get install -y lcov uncrustify
    # For the building the documentation:
    sudo apt-get install --no-install-recommends doxygen librsvg2-bin \
      texlive-latex-base texlive-latex-extra latexmk texlive-fonts-recommended
    ```

1. Update cmake (the versions provided by Ubuntu 16.04 and 18.04 are too old)
    ```
    sudo apt purge --auto-remove cmake
    mkdir -p ~/git/ && cd ~/git
    git clone https://gitlab.kitware.com/cmake/cmake.git
    cd cmake
    ./bootstrap
    make -j9
    sudo make install
    ```

1. Use west to clone this directory into `~/git/loraiot`:
    ```
    mkdir -p ~/git/loraiot
    cd ~/git/loraiot
    west init -m ssh://git@github.com:/hevs-isi/LoRaIOT.git
    cd zephyr/
    west update
    ```
1. Since west does something <s>nasty</s> I don't get with branches, let's configure remote branches as we want:
    ```
    cd ~/git/loraiot/zephyr
    git fetch --all
    git checkout --track origin/lora_shell
    ```

## Testing the zephyr install
```
cd ~/git/loraiot/zephyr/samples/hello_world
source ~/git/loraiot/zephyr/zephyr-env.sh
west build -b native_posix
./build/zephyr/zephyr.elf
```
Expected output:
```
***** Booting Zephyr OS zephyr-vX.XX.X-XXXX-gXXXXXXX *****
Hello World! native_posix
^C
Stopped at 1.520s
```
**Please don't even go further if `hello_world` is not working.**

## Testing on the nucleo_l432kc board + WiMOD_AB_01 + usb board + iM880x
1. Wiring

| mcu | testbutton |
|--- | ---------- |
| GND | X6 pin 9 - GND |
| pin D1 - PA9 - TX | X7 pin 20 - TX |
| pin D0 - PA10 - RX | X7 pin 14 - RX |

1. Connect the board, and open it's serial port (115200,8,n,1).

1. Go to the app directory:
    ```
    cd ~/git/loraiot/zephyr/app
    source ~/git/loraiot/zephyr/zephyr-env.sh
    ```

1. Compile and run lora_shell using :

    ```
    west build -b nucleo_l432kc
    west flash
    ```

1. Expected output on the serial port:

    ```
    ***** Booting Zephyr OS 1.14.0-rc1 *****


    [00:00:00.003,643] <dbg> main.main: main
    [00:00:01.003,700] <dbg> wimod_hci_driver.wimod_hci_send_message: here
    [00:00:01.183,734] <dbg> wimod_hci_driver.wimod_hci_process_rx_message: wimod_hci_process_rx_message
    [00:00:01.183,740] <dbg> wimod_lorawan_api.wimod_lorawan_process_rx_msg: here
    [00:00:01.183,745] <dbg> wimod_lorawan_api.wimod_lorawan_process_lorawan_message: msg_id:10
    [00:00:01.183,755] <dbg> wimod_lorawan_api.wimod_lorawan_show_response: join network response: - Status(0x00) : ok
    [00:00:01.349,073] <dbg> wimod_hci_driver.wimod_hci_process_rx_message: wimod_hci_process_rx_message
    [00:00:01.349,081] <dbg> wimod_lorawan_api.wimod_lorawan_process_rx_msg: here
    [00:00:01.349,084] <dbg> wimod_lorawan_api.wimod_lorawan_process_lorawan_message: msg_id:11
    [00:00:01.349,094] <dbg> wimod_lorawan_api.wimod_lorawan_process_join_tx_indication: join tx event:1, ChnIdx:0, DR:5 - Status : ok
    [00:00:06.624,313] <dbg> wimod_hci_driver.wimod_hci_process_rx_message: wimod_hci_process_rx_message
    [00:00:06.624,323] <dbg> wimod_lorawan_api.wimod_lorawan_process_rx_msg: here
    [00:00:06.624,326] <dbg> wimod_lorawan_api.wimod_lorawan_process_lorawan_message: msg_id:12
    [00:00:06.624,336] <dbg> wimod_lorawan_api.wimod_lorawan_process_join_network_indication: join network accept event - DeviceAddress:0x2601237D, ChnIdx:0, DR:5, RSSI:158, SNR:7, RxSlot:1
    [00:00:06.624,340] <inf> lora: LoRaWAN Network joined.
    [00:00:07.795,808] <dbg> wimod_hci_driver.wimod_hci_process_rx_message: wimod_hci_process_rx_message
    [00:00:07.795,816] <dbg> wimod_lorawan_api.wimod_lorawan_process_rx_msg: here
    [00:00:07.795,820] <dbg> wimod_lorawan_api.wimod_lorawan_process_lorawan_message: msg_id:15
    [00:00:07.795,830] <dbg> wimod_lorawan_api.wimod_lorawan_show_response: send U-Data TX indication: - Status(0x01) : error
    [00:00:09.827,820] <dbg> wimod_hci_driver.wimod_hci_process_rx_message: wimod_hci_process_rx_message
    [00:00:09.827,827] <dbg> wimod_lorawan_api.wimod_lorawan_process_rx_msg: here
    [00:00:09.827,832] <dbg> wimod_lorawan_api.wimod_lorawan_process_lorawan_message: msg_id:22
    [00:00:09.827,835] <dbg> wimod_lorawan_api.wimod_lorawan_process_lorawan_message: no data received indication
    ```

    Typing `lora` + <kbd>TAB</kbd> will display the list of lora commnds:
    ```
    uart:~$ lora 
    firmware       custom         getmode        get_rtc        set_rtc
    get_rtc_alarm  factory        deveui         set            join
    nwk            setrstack      rstack         set_rstack     udata
    cdata
    ```
1. When the code is run for the first time with the modem, the APP_EUI and APP_KEY
    must be programmed into the modem: (the key is fixed in the code, see
    `../ext/hal/wimod/wimod_lorawan_api.c`):

    ```
    lora set
    ```

1. Join the network (only needed after `lora set`, it's automatically done at reboot time):

    ```
    lora join
    ```

1. Send unreliable data to the network:

    ```
    lora udata
    ```

1. Send reliable data to the network:

    ```
    lora cdata
    ```

1. Data may be received just after some data has been transmitted, here `AA BB CC`:

    ```
    uart:~$ lora udata 
    [00:00:10.187,476] <dbg> wimod_hci_driver.wimod_hci_send_message: here
    [00:00:10.194,539] <dbg> wimod_hci_driver.wimod_hci_process_rx_message: wimod_hci_process_rx_message
    [00:00:10.194,545] <dbg> wimod_lorawan_api.wimod_lorawan_process_rx_msg: here
    [00:00:10.194,550] <dbg> wimod_lorawan_api.wimod_lorawan_process_lorawan_message: msg_id:14
    [00:00:10.194,560] <dbg> wimod_lorawan_api.wimod_lorawan_show_response: send U-Data response: - Status(0x00) : ok
    [00:00:11.527,889] <dbg> wimod_hci_driver.wimod_hci_process_rx_message: wimod_hci_process_rx_message
    [00:00:11.527,897] <dbg> wimod_lorawan_api.wimod_lorawan_process_rx_msg: here
    [00:00:11.527,902] <dbg> wimod_lorawan_api.wimod_lorawan_process_lorawan_message: msg_id:15
    [00:00:11.527,912] <dbg> wimod_lorawan_api.wimod_lorawan_show_response: send U-Data TX indication: - Status(0x01) : error
    [00:00:13.701,051] <dbg> wimod_hci_driver.wimod_hci_process_rx_message: wimod_hci_process_rx_message
    [00:00:13.701,059] <dbg> wimod_lorawan_api.wimod_lorawan_process_rx_msg: here
    [00:00:13.701,064] <dbg> wimod_lorawan_api.wimod_lorawan_process_lorawan_message: msg_id:16
    [00:00:13.701,067] <dbg> wimod_lorawan_api.wimod_lorawan_process_u_data_rx_indication: U-Data rx event: port:0x01app-payload:
    [00:00:13.701,072] <dbg> wimod_lorawan_api.wimod_lorawan_process_u_data_rx_indication: AA 
    [00:00:13.701,075] <dbg> wimod_lorawan_api.wimod_lorawan_process_u_data_rx_indication: BB 
    [00:00:13.701,076] <dbg> wimod_lorawan_api.wimod_lorawan_process_u_data_rx_indication: CC 
    [00:00:13.701,081] <dbg> wimod_lorawan_api.wimod_lorawan_process_u_data_rx_indication: 
    [00:00:13.701,084] <dbg> wimod_lorawan_api.wimod_lorawan_process_u_data_rx_indication: ack for uplink packet:no
    [00:00:13.701,088] <dbg> wimod_lorawan_api.wimod_lorawan_process_u_data_rx_indication: frame pending:no
    [00:00:13.701,105] <dbg> wimod_lorawan_api.wimod_lorawan_process_u_data_rx_indication: ChnIdx:128, DR:3, RSSI:167, SNR:6, RxSlot:2
    uart:~$ 
    ```
