Lets get started;
----------------------
we dont support Windows for now,  - building for mac and bsd , arch deriatives Are experimental and not made for real package creation now, . only RPM and DEB is tested; and TGZ eg. tar.gz ( compressed at 22 zstd ) is too available
To build you need to type this in your terminal
 ```bash
 
 
 
                                  "./build"



```

and hit enter it would surely ask for what to do;

./build --clean to remove build directories,
./build --options  {CMAKE PROFILE VALUE} {DBS VALUE}

cmake profile value can be 1 to 4
- 1 is Debug
- 2 is Release
- 3 is RElease with Debug info
- 4 is Min size release



while DBS ( Distribution build scripts )
has 8 values

- 1 is for Debian/Ubuntu  Derivatives 
- 2 is for Fedora/Red hat Linux  Derivatives 
- 3 is for Arch Linux Derivatives ( Arch Linux needed
- 4 is for Mac and it cannot work on anything but mac
- 5 is for FreeBSD and its Derivatives , same as mac ( it cannot run on anything but FreeBSD)
- 6 is Tar gz which is a format for sharing this, it contains compiled version of Onu with the icons 
- 7 is Appimage, the script downloads and installs the depending packages and creates an  Appimage,
- 8 is all of above

so as i want to create all package in release without wasting any time on onboarding , i would do
./build --options 2 8
2 is release of cmake 
and 8 is "all of above" of DBS 
so its easy to make some combinations and run this script.

If you face any problems Contact me at <zynomon@proton.me> or create an issue here:- https://github.com/zynomon/onu/issues
Alternatively this project can be found on https://codeberg.org/zynomon/onu
