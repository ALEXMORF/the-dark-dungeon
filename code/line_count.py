import os
    
line_count = 0
file_list = os.listdir()

for filename in file_list:
    if filename == 'stb_image.h':
        continue
    
    file_handle = open(filename)

    for line in file_handle:
        line_count += 1
    
    file_handle.close()

print('Directory Line Count: ', line_count)
