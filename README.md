# virsh-ss

`virsh send-key` wrapper for quickly sending full strings to libvirt virtual
machines.

## Requirements

- C99-capable tool chain.
- GNU make.
- `gzip` (for `make install`).
- `virsh` (for runtime).

## Build
```
git submodule update --init
make && make install
```

### Development

- `make release` - Build release binary (same as `make`).
- `make debug` - Build debug binary.
- `make clean` - Remove build files.

### Configuration

|Macro|Description|Default|
|---|---|---|
|`VIRSH_BIN`|Name of the system default virsh binary|`virsh`|

Also, [nanorl](https://github.com/vladaviedov/c-utils/blob/master/nanorl/README.md) has configuration options.

## Install

`make install` will install the program using in the `/usr/` prefix.
If you wish to change the install location pass a custom `PREFIX` to
`make install`

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

Environment Variables:
- `VIRSH`: overrides virsh binary name

## Examples

### Unlocking Encrypted VM

Unlock a LUKS encrypted VM without using a GUI:
1. Run the command `virsh-ss --prompt --secret --newline DOMAIN`
2. Input luks password
