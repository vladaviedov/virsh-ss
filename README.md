# virsh-ss

## Build
Normal build: \
`make`

Debug build: \
`make debug`

## Usage
`virsh-ss <domain> <string> [options]`
- domain: libvirt domain name
- string: string of characters to send (if prompt not set)
Options:
- `-h, --help`: show usage information
- `-v, --version`: show program version
- `-p, --prompt`: prompt for string, instead of reading from arguments
- `-s, --secret`: inputted string will be hidden (if prompt is set)
- `-n, --newline`: send newline character at the end
- `-l, --speed`: maximum characters spent per `virsh` command (1-15), default: 1

## Example

### Unlocking Encrypted VM
Unlock a encrypt VM without using a GUI:
1. Run the command `virsh-ss --prompt --secret --newline DOMAIN`
2. Input luks password

DOMAIN will not be unlocked
