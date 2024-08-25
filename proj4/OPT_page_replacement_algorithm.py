def findLastAppearance(memory, pages, time):

    first, second, third = memory[0], memory[1], memory[2]
    basic = len(pages) + 1
    idx = [basic] * 3

    cnt = 0
    
    for page in pages[time:]:
        cnt += 1
        if page in (first, second, third):

            if page == first:

                idx[0] = min(cnt, idx[0])

            elif page == second:

                idx[1] = min(cnt, idx[1])

            else:

                idx[2] = min(cnt, idx[2])

            if idx[0] != basic and idx[1] != basic and idx[2] != basic:
                break

    return idx.index(max(idx))


def OPT(size, pages):

    SIZE = size
    memory = []
    faults = 0
    time = 0

    for page in pages:

        time += 1
        
        if memory.count(page) == 0 and len(memory) < SIZE:

            memory.append(page)
            faults += 1

        elif memory.count(page) == 0 and len(memory) == SIZE:

            idx = findLastAppearance(memory, pages, time)
            memory.pop(idx)
            memory.append(page)
            faults += 1

        else:

            pass

    return faults

size = 3
pages = [7, 0, 1, 2, 0, 3, 0, 4, 2, 3, 0, 3, 2, 1, 2, 0, 1, 7, 0, 1]

print(OPT(size, pages))