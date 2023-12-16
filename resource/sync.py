import sqlite3

def connect_sql():
    return sqlite3.connect("./PhigrosInfo.db")

def float_equal(x, y, tolerance=1e-2):
    return abs(x - y) < tolerance

if __name__ == "__main__":
    levels = {}

    # 打开文件
    with open('difficulty.csv', 'r', encoding='utf-8') as difficulty:
        # 逐行读取文件内容
        for single in difficulty:
            level = single.strip().split(',')
            level[0] = f"{level[0]}.0"
            for index in range(1,len(level)):
                level[index] = float(level[index])
            if len(level) < 5:
                level.append(None)
            levels[level[0]] = level[1:]
    
    for key, value in levels.items():
        info = connect_sql().execute(f'SELECT * FROM phigros WHERE sid = "{key}"').fetchall()
        info = info[0]
        compare_level = (info[5], value[0]),(info[6], value[1]),(info[7], value[2]),(info[8], value[3])

        for index in range(len(compare_level)):
            val = compare_level[index]
            if None is val[0]:
                continue
            if not float_equal(val[0], val[1]):
                print(f"{key} : {val[0]}, {val[1]}")