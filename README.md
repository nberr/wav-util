# wav-util
Simple command line utility program to view and edit wav file header data. 
In it's current state (30 October 2024) it can only be used to view and edit 
the data in the `RIFF` and `fmt` chunks. 

## Background
This program was originally a school assignment. I haven't found a simple wav header editor so I decided to upload this
in case it's useful to anyone else.

## .wav file structure
![](img/wav-info.png)
* reference: http://soundfile.sapp.org/doc/WaveFormat/