USB to BSC Serial Dongle
========================

An attempt to create a device that provides a RS-232/V.24 synchronous serial interface
that is attached to the host via USB. The device uses an Arduino board and several
MAX232 ICs for level shifting.

3274/3270 Data -- Not verified
==============================

Poll control unit 0, device 0
------------------------------

Put 3174 in control mode to monitor line

PAD  LPAD LPAD SYN  SYN  EOT  PAD
FF   55   55   32   32   37   FF

Then the poll

PAD  LPAD LPAD SYN  SYN  SP   SP   SP   SP   ENQ  PAD
FF   55   55   32   32   40   40   40   40   2D   FF

The EBCDIC x'40' (space) is the poll address character for CU 0.

### Response from poll when no data available is EOT

PAD  SYN  SYN  EOT  PAD
FF   32   32   37   FF

### or response of text

PAD  SYN  SYN  STX  ...  ETX  BCC  BCC  PAD



Select control unit 0, device 0
-------------------------------

Put 3174 in control mode to monitor line

PAD  LPAD LPAD SYN  SYN  EOT  PAD
FF   55   55   32   32   37   FF

Then the select. This is the same as a poll with different unit address char.

PAD  LPAD LPAD SYN  SYN  -    -    SP   SP   ENQ  PAD
FF   55   55   32   32   60   60   40   40   2D   FF

The EBCDIC x'60' (-) is the select address character for CU 0.

### Response from select is an ACK0

PAD  SYN  SYN  DLE  /    PAD
FF   32   32   10   70   FF





Send data to the control unit following a select
------------------------------------------------

PAD  LPAD LPAD SYN  SYN  DLE  STX  ESC  EW   WCC  SBA
FF   55   55   32   32   10   02   27   F5   42   11   40   40

SF        H    E    L    L    O         W    O    R    L    D
1D   60   C8   C5   D3   D3   D6   40   E6   D6   D9   D3   C4

               IC   DLE  ETX  BCC----   PAD
40   40   40   13   10   03   6C   16   FF

### Response from CU is ACK0|1 or NAK

PAD  SYN  SYN  DLE      PAD
FF   32   32   10   70   FF     (ACK0)

PAD  SYN  SYN  DLE  /    PAD
FF   32   32   10   61   FF     (ACK1)



Read data from the device
-------------------------

After the select ...

PAD  SYN  SYN  STX  ESC  READ ETX  BCC----   PAD
FF   32   32   02   27   F6   03   B7   AA   FF

### Response from device is text msg

PAD  SYN  SYN  STX  ADDR ADDR           ETX  BCC----   PAD
FF   32   32   02   40   40   60   40   03   65   7A   FF


