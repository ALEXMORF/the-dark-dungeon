import os

file_list = os.listdir()

def remove_leading_space(line):
    c_index = 0
    while line[c_index] == ' ' or line[c_index] == '\t':
        c_index += 1
    return line[c_index:]

for file_name in file_list:

    if file_name.endswith("py") or file_name.startswith("stb"):
        continue
    
    file_handle = open(file_name)

    for line_num, line in enumerate(file_handle):
        if "TODO" in line:
            print("%s:%d " % (file_name,line_num+1), remove_leading_space(line))

    file_handle.close()
    
