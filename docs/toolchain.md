# Toolchain setup
What the `make` targets need and how to install it. None of this is needed to build in Microchip
Studio, which the [README](../README.md) covers.

---

## What each target needs

| Target                             | Needs                                                                   |
| ---------------------------------- | ----------------------------------------------------------------------- |
| `make build`                       | `avr-gcc`, `avr-objcopy`, `avr-size`, and the AVR-Dx device family pack  |
| `make flash`                       | the above, plus `avrdude` and a serial UPDI adapter                     |
| `make test`                        | `gcc`, `g++`, and the `libs/test` submodule checked out                 |
| `make format`, `make format-check` | `clang-format`                                                          |

Every target also needs `make` and `bash`.

---

## Linux and WSL

```bash
sudo apt -y update
sudo apt -y install gcc-avr binutils-avr avr-libc avrdude g++ make clang-format
```

`avr-libc` has no support for this device, so the device family pack is downloaded once into
`dfp/`:

```bash
wget http://packs.download.atmel.com/Atmel.AVR-Dx_DFP.1.10.114.atpack
unzip -q -d dfp Atmel.AVR-Dx_DFP.1.10.114.atpack
```

On WSL the build also finds the pack in a Windows-side Microchip Studio installation.

Flashing from WSL needs the USB serial adapter passed through to Linux. Running
`bash ci/flash.sh` from Git Bash on the Windows side, against the same `build/main.hex`, is
easier.

---

## Windows, in Git Bash
Run everything from **Git Bash**, not from `cmd` or PowerShell. The `winget` commands are the
exception.

### Adding something to PATH
Each tool below ends up in a directory that has to be added to PATH:

```bash
echo 'export PATH="/c/some/directory/bin:$PATH"' >> ~/.bashrc
source ~/.bashrc
```

Use single quotes around the `echo` argument, and point at the directory holding the executable.
`cygpath -u "C:\some\directory"` converts a Windows path.

If the change has no effect, check that `~/.bash_profile` contains
`[ -f ~/.bashrc ] && . ~/.bashrc`.

### make

```powershell
winget install ezwinports.make
```

WinGet does not put it on PATH, so find the binary and add its directory:

```bash
find "$(cygpath -u "$LOCALAPPDATA")/Microsoft/WinGet/Packages" -name make.exe
```

Chocolatey (`choco install make`) and Scoop (`scoop install make`) put it on PATH by themselves.

### avr-gcc
Microchip Studio ships a complete AVR toolchain, including `avr-objcopy` and `avr-size`:

```bash
echo 'export PATH="/c/Program Files (x86)/Atmel/Studio/7.0/toolchain/avr8/avr8-gnu-toolchain/bin:$PATH"' >> ~/.bashrc
source ~/.bashrc
avr-gcc --version
```

Without Studio, download Microchip's `avr8-gnu-toolchain-*-win32.any.x86_64.zip`, unpack it
somewhere without spaces, such as `C:\avr\avr-gcc`, and add its `bin` directory to PATH.

### The device family pack
With Studio installed the build finds the pack by itself. Without Studio, unpack a copy into
`dfp/`:

```bash
curl -LO http://packs.download.atmel.com/Atmel.AVR-Dx_DFP.1.10.114.atpack
unzip -q -d dfp Atmel.AVR-Dx_DFP.1.10.114.atpack
```

`DFP_DIR` points the build at a pack kept somewhere else. Keep that path free of spaces:

```bash
make build DFP_DIR=/c/avr/packs/AVR-Dx_DFP
```

### avrdude
Needed only for `make flash`. Download the Windows zip from the
[avrdude releases](https://github.com/avrdudes/avrdude/releases), unpack it somewhere without
spaces, such as `C:\avr\avrdude`, and add that directory to PATH.

The port defaults to `COM3`. Device Manager lists the one the adapter actually took:

```bash
make flash PORT=COM4
```

### clang-format
The smallest install is through pip:

```bash
pip install clang-format==18.1.8
python -c "import sysconfig; print(sysconfig.get_path('scripts'))"
```

Add the directory the second command prints to PATH. The alternative is LLVM, which is much
larger:

```powershell
winget install LLVM.LLVM
```

```bash
echo 'export PATH="/c/Program Files/LLVM/bin:$PATH"' >> ~/.bashrc
source ~/.bashrc
```

Use version 18, the one the CI pipeline runs. `clang-format` changes its output between major
versions, so another version can fail `make format-check` on code nobody touched.

---

## Checking it worked
Open a new shell, then run from the repository root:

```bash
git submodule update --init  # Fetches the test framework, needed once.
make build                   # Ends with the size of main.elf.
make test                    # Ends with the test results.
make format-check            # Prints only the file count when everything is formatted.
```

---

## Troubleshooting

| Symptom | Fix |
| ------- | --- |
| `command not found` right after installing something | Open a new Git Bash, or `source ~/.bashrc` |
| Edits to `~/.bashrc` change nothing | Add `[ -f ~/.bashrc ] && . ~/.bashrc` to `~/.bash_profile` |
| `error: AVR-Dx device family pack not found` | Unpack a pack into `dfp/`, or set `DFP_DIR` |
| `error: test framework not found in libs/test` | `git submodule update --init` |
| Recipe lines fail with `cmd`-style errors | Run `make` from Git Bash |
| Stray `\r` errors in recipes or scripts | `git config core.autocrlf false`, then check the files out again |
| `avrdude: ser_open(): can't open device` | `make flash PORT=COM4`, with the port Device Manager lists |
| `make` vanishes after `winget upgrade` | Run the `find` again and update `~/.bashrc` |
| `make format-check` fails on code nobody touched | Install `clang-format` 18 |

---
