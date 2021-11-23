import subprocess


bram_max_width = [2, 8, 16, 16, 32, 64, 64, 128]
bram_ratio = [1, 5, 5, 5, 15, 25, 32, 64]
second_bram_ratio = 1
LUTRAM_ratio = 1
mem_size = 2**10

best_area = -1
best_lutram_ratio = -1
best_bram1_size = -1
best_bram2_size = -1
in1 = -1
in2 = -1
best_area_sci_notation = -1
best_second_ratio = -1

while LUTRAM_ratio <= 5:
    i = 0
    while i < len(bram_max_width):
        j = 0
        while j < len(bram_max_width):
            if(i == j):
                j += 1
                continue
            while second_bram_ratio <= 300:
                mapper_result = subprocess.run(["./mapper_general", f"{mem_size*(2**(i))}", f"{bram_max_width[i]}", f"{bram_ratio[i]}", f"{mem_size*(2**(j))}", f"{bram_max_width[j]}", f"{second_bram_ratio}", f"{LUTRAM_ratio}"], stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.PIPE) 
                checker_res = subprocess.run(["./checker",f"-l", f"{LUTRAM_ratio}", "1" , f"-b", f"{mem_size*(2**(i))}", f"{bram_max_width[i]}", f"{bram_ratio[i]}", "1", f"-b", f"{mem_size*(2**(j))}", f"{bram_max_width[j]}", f"{second_bram_ratio}", "1", "logical_rams.txt", "logic_block_count.txt", "checker_input.txt"], stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.PIPE)         
                result_check = (checker_res.stdout[:-1]).decode("utf-8")
                result_check = result_check.split()
                area = float(result_check[-1])
                if(best_area == -1 or best_area > area):
                    best_area = area
                    best_area_sci_notation = result_check[-1]
                    best_lutram_ratio = LUTRAM_ratio
                    best_bram1_size = mem_size*(2**(i))
                    best_bram2_size = mem_size*(2**(j))
                    in1 = i
                    in2 = j
                    best_second_ratio = second_bram_ratio
                if(second_bram_ratio == 1):
                    second_bram_ratio += 4
                else:
                    second_bram_ratio += 5
            second_bram_ratio = 1
            j += 1
            print("************************************************************")
            print(f" FIRST BRAM: mem size = {best_bram1_size} - Max Width = {bram_max_width[in1]}, - Ratio: {bram_ratio[in1]}")
            print(f" SECOND BRAM: mem size = {best_bram2_size} - Max Width = {bram_max_width[in2]}, - Ratio: {best_second_ratio}")
            print(f"BEST LUTRAM RATIO: {best_lutram_ratio}")
            print(f"best area = {best_area_sci_notation}")
        i += 1
    LUTRAM_ratio += 1 

print("******************************BEST******************************")
print(f" FIRST BRAM: mem size = {best_bram1_size} - Max Width = {bram_max_width[in1]}, - Ratio: {bram_ratio[in1]}")
print(f" SECOND BRAM: mem size = {best_bram2_size} - Max Width = {bram_max_width[in2]}, - Ratio: {best_second_ratio}")
print(f"BEST LUTRAM RATIO: {best_lutram_ratio}")
print(f"best area = {best_area_sci_notation}")
