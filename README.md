Before Installing
=================

You are required to have `gcc`, `make` and `kernel-headers` installed for building the module.

To detect your kernel version run:
```
uname -r
```
It will output something like
```
6.11.10-2-MANJARO
```
Then, on Manjaro, run:
```
sudo pacman -S linux611-headers
```

To Install
==========
Clone the repo into a github directory in your home folder:
```
mkdir github
cd github
git clone https://github.com/twifty/amd-gpu-i2c.git
cd amd-gpu-i2c
```
Then run:
```
make install
```
The installer will prompt for your sudo password in order to insert the module into the kernel.

After Install
=============
For command line interaction with the I2C bus the `i2c-dev` module is required:
```
sudo pacman -S i2c-tools
sudo modprobe i2c-dev
```
List all i2c devices with:
```
sudo i2cdetect -l
```
It will output something like
```
i2c-0   i2c             AMDGPU SMU 0                            I2C adapter
i2c-1   i2c             AMDGPU SMU 1                            I2C adapter
i2c-2   i2c             AMDGPU DM i2c hw bus 0                  I2C adapter
i2c-3   i2c             AMDGPU DM i2c hw bus 1                  I2C adapter
i2c-4   i2c             AMDGPU DM i2c hw bus 2                  I2C adapter
i2c-5   i2c             AMDGPU DM i2c hw bus 3                  I2C adapter
i2c-6   i2c             AMDGPU DM aux hw bus 0                  I2C adapter
i2c-7   i2c             AMDGPU DM aux hw bus 1                  I2C adapter
i2c-8   i2c             AMDGPU DM aux hw bus 2                  I2C adapter
i2c-9   smbus           SMBus PIIX4 adapter port 0 at 0b00      SMBus adapter
i2c-10  smbus           SMBus PIIX4 adapter port 2 at 0b00      SMBus adapter
i2c-11  smbus           SMBus PIIX4 adapter port 1 at 0b20      SMBus adapter
i2c-12  i2c             AMD-GPU-I2C hidden bus 0                I2C adapter

```
Here we can see this module "AMD-GPU-I2C hidden bus" is detected as device number 12. We can now scan that device with:
```
sudo i2cdetect -y 12
```
It should output all clients available on that bus:
```
     0  1  2  3  4  5  6  7  8  9  a  b  c  d  e  f
00:                         08 -- -- -- -- -- -- -- 
10: -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- 
20: -- -- -- -- -- -- -- -- -- -- 2a -- -- -- -- -- 
30: -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- 
40: -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- 
50: -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- 
60: -- -- -- -- -- -- -- -- 68 -- -- -- -- -- -- -- 
70: -- -- -- -- -- -- -- --      
```
