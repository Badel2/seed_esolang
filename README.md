# seed_esolang
A tool for writing (or finding) seed programs

This tool allows to easyly write programs for the seed esolang (https://esolangs.org/wiki/Seed).
Currently, the programs can be up to 112 characters (without bruteforce, usually 116 if you have time to wait).

Usage:
First, download and compile, you will need gcc and python installed on your computer.

    git clone https://github.com/Badel2/seed_esolang
    cd seed_esolang
    make

And done. To use it, write your target befunge program in a file, for example program.txt, and then run

    ./seed_esolang -i program.txt -o program_seed.txt

Your desired seed program is saved as program_seed.txt. You can test it with the included seed interpreter:
    
    python3 seed_interpreter.py program_seed.txt

That's all.

This program may or may not be improved somewhere in the far future.
