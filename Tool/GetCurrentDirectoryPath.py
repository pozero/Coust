import os

def convert_path_to_cstring(path):
    directories = []
    start_i = 0
    for i in range(len(path)):
        if path[i] == '\\':
            directories.append(path[start_i:i])
            start_i = i + 1
    directories.append(path[start_i:len(path)])
    return "\\\\".join(directories)

cwd = os.getcwd()
print(convert_path_to_cstring(cwd))