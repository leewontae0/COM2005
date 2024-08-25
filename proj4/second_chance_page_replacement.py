def page_replace(size, pages):

    SIZE = size
    memory = []
    faults = 0

    for page in pages:

        inMemory = False
        for p, i in memory:
            if p == page:
                inMemory = True
                break

        if not inMemory and len(memory) < SIZE:

            memory.append([page, 1])
            faults += 1

        elif not inMemory and len(memory) == SIZE:

            while (1):

                p, i = memory[0]
                if i:
                    memory[0][1] = 0
                    temp = memory.pop(0)
                    memory.append(temp)
                else:
                    memory.pop(0)
                    memory.append([page, 1])
                    break

            faults += 1


        else:

            for i in range(len(memory)):

                if memory[i][0] == page:

                    memory[i][1] = 1

    return faults



size = 3
pages = [7, 0, 1, 2, 0, 3, 0, 4, 2, 3, 0, 3, 2, 1, 2, 0, 1, 7, 0, 1]

print(page_replace(size, pages))