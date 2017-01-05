#!/usr/bin/env python3
from sys import argv
import random
#program="4 80814037" ##Place program here
with open(argv[1],'r') as f:
    program = f.read()
program=program.split()
length=int(program[0])

random.seed(int(program[1]))
chars="".join(map(chr,range(32,127)))+'\n'
prog="".join([chars[int(random.random()*96)] for i in range(length)])

print("Your program:\n----------------------------------------\n")
print(prog, "\n----------------------------------------\n")
