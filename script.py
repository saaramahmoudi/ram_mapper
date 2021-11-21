import subprocess

mem_size = 1024
args = 0

while mem_size <= 2**17:
    best_max_width = -1
    best_ratio = -1
    best_area = -1
    max_width = 2
    best_area_sci_notation = ""
    while max_width <= mem_size/256:
        ratio = 1
        while ratio <= 300:
            mapper_result = subprocess.run(["./a.out", f"{mem_size}", f"{max_width}", f"{ratio}"], stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
            result = (mapper_result.stdout[:-1]).decode("utf-8")
            result = result.split()
            # print("=================================")
            # print(f"mem size = {mem_size} - Max Width = {result[0]}, - Ratio: {result[1]}")
            checker_res = subprocess.run(["./checker",f"-l", "1", "1" , f"-b", f"{mem_size}", f"{result[0]}", f"{result[1]}", "1", "logical_rams.txt", "logic_block_count.txt", "checker_input.txt"], stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
            result_check = (checker_res.stdout[:-1]).decode("utf-8")
            result_check = result_check.split()
            # print(result_check[-1])
            area = float(result_check[len(result_check)-1])
            if(best_area == -1 or best_area > area):
                best_area = area
                best_max_width = max_width
                best_ratio = ratio
                best_area_sci_notation = result_check[-1]
            if ratio == 1:
                ratio += 4
            else:
                ratio += 5
        max_width *= 2
    print("******************************BEST******************************")
    print(f"mem size = {mem_size} - Max Width = {best_max_width}, - Ratio: {best_ratio}")
    print(f"best area = {best_area_sci_notation}")
    mem_size *= 2
    # break
