def page_replace(size, pages):

    SIZE = size
    memory = []
    faults = 0

    for page in pages:

        inMemory = False
        for p, r, m in memory:
            if p == page:
                inMemory = True
                break

        if not inMemory and len(memory) < SIZE:

            memory.append([page, 1, 0])
            faults += 1

        elif not inMemory and len(memory) == SIZE:

            temp = []

            for i in range(len(memory)):

                elem = [memory[i] + [i]]
                temp += elem
            temp = sorted(temp, key=lambda x: (x[1], x[2], x[3]))

            targetElem = [temp[0][0], temp[0][1], temp[0][2]]

            idx = memory.index(targetElem)

            if targetElem[1] == 1:

                for i in range(len(memory)):

                    memory[i][1] = 0

            for _ in range(idx):

                tempElem = [memory[0][0], 0, memory[0][2]]
                memory.pop(0)
                memory.append(tempElem)

            memory.pop(0)
            tempElem = [page, 1, 0]
            memory.append(tempElem)

            faults += 1


        else:

            for i in range(len(memory)):

                if memory[i][0] == page:

                    memory[i][1] = 1

    return faults



size = 3
pages = [7, 0, 1, 2, 0, 3, 0, 4, 2, 3, 0, 3, 2, 1, 2, 0, 1, 7, 0, 1]

print(page_replace(size, pages))