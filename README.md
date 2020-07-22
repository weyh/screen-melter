# screen-melter

The code (as many similar), based off of [Napalm's original](http://www.rohitab.com/discuss/topic/23191-screen-melter/?p=190669).
However, many (if any) do not implement **multi-monitor support** and **dpi awareness**, which ruin the effect. This version does. Plus, I added some **args** too.

![melt.png](Imgs/melt.png)

### Args:

 - Sleep time before visual effect (ms): `-t`, `--time` Example: `-t 2000`, `--time=2000`
 - The program closes automatically after this amount of time (ms): `-e`, `--exit_time` Example: `-e 10000`, `--exit_time=10000`
 - Disable user input: `-I,--disable_input` (ctrl+alt+del still works)
 - Disable keyboard: `-K,--disable_keyboard` (ctrl+alt+del still works)
 - Disable mouse: `-M,--disable_mouse`
 
 E.g: `screen-melter_x86.exe -M --disable_keyboard -t 20 --exit_time=10000`
 
### License

This project is licensed under the MIT License - see the [MIT License](LICENSE) file for details.