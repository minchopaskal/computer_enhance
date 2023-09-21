import sys

special = []
instrs = []
filename = sys.argv[1]
mode = 0
with open(filename, 'r') as f:
    lines = f.readlines()
    for line in lines:
        line = line.strip()
        if line == "":
            continue
        if line == "special":
            mode = 1
            continue
        if line == "normal":
            mode = 0
            continue
        if mode == 0:
            instrs.append(line)
        else:
            special.append(line)

instr_table = [None] * 256
for line in instrs:
    tokens = line.split(' ')
    name = tokens[0]
    pattern = tokens[1]
    instr_type = "SingleByte"
    special_idx = "-1"
    if len(tokens) > 2:
        instr_type = tokens[2]
    if len(tokens) > 3:
        special_idx = tokens[3]
    
    shift = 8 - len(pattern)
    values = []
    try:
        value = int(pattern, 2) << shift
        values.append(value)
    except ValueError:
        sr_codes = ["00", "10", "11", "01"]
        if pattern == "000--111":
            sr_codes.pop()
        for substr in sr_codes:
            values.append(int(pattern.replace("--", substr), 2))
        
    for value in values:
        num_combinations = 1<<shift
        for i in range(0, num_combinations):
            instr_table[value + i] = [name, instr_type, special_idx]

with open("instr_table.inl", 'w') as f:
    f.write('Instruction special_instructions[7][8] = {\n')
    f.write('   {\n')

    idx = 0
    cnt = 0
    for line in special:
        tokens = line.split(' ')
        f.write('       {{ \"{}\", InstructionType::{} }},\n'.format(tokens[0], tokens[1]))
        idx = idx + 1
        if idx == 8:
            idx = 0
            cnt = cnt + 1
            f.write("   },\n")
            if cnt <= 6:
                f.write('   {\n')
    f.write('};\n\n')

    f.write('Instruction instructions[256] = {\n')
    for i, instr in enumerate(instr_table):
        if instr == None:
            #print("Unknown instruction: {:02X}".format(i))
            instr = ["Unknown", "Unknown", "-1"]
        else:
            pass
            #print("[{:2X}] {}".format(i, instr[0]))
        opcode = instr[0]
        if instr[0] == "-":
            opcode = "Unknown"
        if opcode == "and" or opcode == "or" or opcode == "xor" or opcode == "int":
            opcode = opcode + "_"
        f.write('   {{ \"{}\", InstructionType::{}, InstructionOpcode::{}, {} }},\n'.format(instr[0], instr[1], opcode, instr[2]))
    f.write('};\n')


    




