import os

file_list = os.listdir()

for file_name in file_list:

    if file_name.endswith("py") or file_name.startswith("stb"):
        continue
    
    file_handle = open(file_name)

    for line_num, line in enumerate(file_handle):
        if "TODO" in line:
            print("%s:%d " % (file_name,line_num+1), line)

    file_handle.close()
    
