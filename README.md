# ft_ping


## Overview

`ft_ping` is an implementation of the `ping` command, designed after the `inetutils-2.0` implementation.
The goal of this project is to understand low-level networking by sending ICMP(Internet Control Message Protocol) Echo Request packets and handling responses.
And requires to use raw sockets and packet structures.


## Main Goal:

- Construct and sending ICMP Echo Request
- Implementing checksum calculation for integrity check
- Handling ip paccket processing and error detection
- Calculation for the RTT (Round-Trip Time) for each packet
- Handling signal interuptions
- Display statistics


## Usage

``` SHELL
./ft_ping [options] <destination>
```

## Example

``` SHELL
./ft_ping google.com
```
![image](https://github.com/user-attachments/assets/7c6fbe7f-8043-4de7-8d92-c50e35049405)

## Options

-? : Display help message
-c <count> : Stop after sending <count> packets
-i <interval> : Set time interval between packets
-t <ttl> : Set Time-To-Live value for packets
-q : Display only statistics
-w <timeout> : Set timeout duration after which the command stops


## Compilation

To compile the project, run:
``` SHELL
make
```


## Notes

ft_ping requires raw socket privileges, so it may need to be run with sudo.

The project aims to replicate basic functionality of the system ping command.
