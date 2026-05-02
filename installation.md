# Installation

Chloride mainly targets Linux, Mac, Windows, and FreeBSD, in order of priority. These targets aren't exclusive, so if you're having issues on a different platform, feel free to open an issue and we will try to resolve it. Also, feel free to fix the issue yourself and send a Pull Request.

Official builds are being made for linux x86_64, windows x86_64, and macOS arm64 at [the project's Jenkins instance](https://jenkins.wbell.dev/job/Argon/).

Jenkins builds deb, rpm, a Windows setup executable, and has a Homebrew formula for Mac (and technically Linux).

## Debian (and Debian based distributions)

Add Open-Argon as a repository for apt.

```bash
sudo curl https://git.wbell.dev/api/packages/Open-Argon/debian/repository.key -o /etc/apt/keyrings/gitea-Open-Argon.asc
echo "deb [signed-by=/etc/apt/keyrings/gitea-Open-Argon.asc] https://git.wbell.dev/api/packages/Open-Argon/debian trixie main" | sudo tee -a /etc/apt/sources.list.d/gitea.list
sudo apt update
```
Then you should be able to install argon.
```bash
sudo apt install argon
```

## Fedora (and Fedora based distributions)

Add the chloride public signature to rpm.

```bash
curl https://git.wbell.dev/Open-Argon/Chloride/raw/branch/main/gpg/argon-packages-public.gpg | sudo rpm --import -
```
Then you should be able to add the rpm repository to dnf.
```bash
# on RedHat based distributions
dnf config-manager --add-repo https://git.wbell.dev/api/packages/Open-Argon/rpm.repo

# Fedora 41+ (DNF5)
dnf config-manager addrepo --from-repofile=https://git.wbell.dev/api/packages/Open-Argon/rpm.repo
```
Finally, you should be able to install argon.
```bash
sudo dnf install argon
```

## Windows

Windows builds don't have a proper distribution method yet (e.g. winget or chocolatey), but it does have a windows setup file which can be [found here](https://jenkins.wbell.dev/job/Argon/lastStableBuild/).

## macOS (Homebrew)

Homebrew is the recommended way to install for macOS. It should support both apple silicon and intel macs.

Add Open-Argon as a tap.

```bash
brew tap open-argon/argon https://git.wbell.dev/Open-Argon/homebrew-open-argon
```

Then install argon.
```bash
brew install argon
```

## Build from source

If the above does not satisfy your requirements, feel free to build for your platform. 

There are two ways to build Chloride. **Conan is recommended for anyone who is not developing Chloride**. Conan is a cross platform package manager and compiler tool.

If you are developing Chloride, it is recommended to use **make**, as that has been set up to build for dynamic linking and has debug tools.

### Conan

For **conan**, the dependencies are `conan`, `flex`, `cmake` and `gcc`.

install using conan.
```
conan install . --build=missing
```

and finally build using conan.
```
conan build .
```

The final build can be found in `build/dist/bin`.

### Make

For **make**, there are more dependencies, since we are not using conan to manage them. The exact dependencies are not fixed, so you may need to read through the **Makefile** to determine which packages are required (or attempt a build to see what is missing).

Development is only currently set up to be possible on posix systems. If you are on windows, it's recommended to use **WSL**.

To build normally, run `make -j$(nproc)`.

If you are building from posix to windows, run `make -j$(nproc) TARGET_OS=windows`.

If you are wanting to debug, use `make -j$(nproc) full-debug`. Of course if you are wanting to debug for windows, add `TARGET_OS=windows`.