import sys
import re
import operator

args = sys.argv

# Bilal Dinc
# This Script analyzes g++ warnings from 'make' outputs
# Give output as a first argument.


def main():
    f = open(args[1], 'r')
    lines = f.readlines()
    f.close()

    outputs = []
    warnings = {}
    k = 0;

    for i in lines:
        # check existence and get warning type
        warningType = re.search(r'\[-.*\]', i, re.M | re.I)
        if warningType:
            # if same line shows up more than ones, save only once
            if i not in outputs:
                outputs.append(i)

                if warningType.group() in warnings:
                    # increment occurrence
                    warnings[warningType.group()][0] = warnings[warningType.group()][0] + 1;
                else:
                    warnings[warningType.group()] = [1,[]]

                # search for exact place of warning and add to the list
                warningFile = re.search(r'.*?: ', i, re.M | re.I)
                if warningFile:
                    warnings[warningType.group()][1].append(warningFile.group())
                else:
                    print('something unexpected occured')

            else:
                k = k + 1

    # write all lines contains warning
    f = open("all_lines_with_warnings.txt", 'w')
    for i in outputs:
        f.write(i)
    f.close()

    # write some statistics
    sorted_x = sorted(warnings.items(), key = operator.itemgetter(1), reverse = True) #i don't know how it works
    f = open("warnings_types_occurences.txt", 'w')
    for i in sorted_x:
        f.write('occurrence : ' + str(i[1][0]) + '    type : ' + i[0] + '\n')
        print('occurrence : ' + str(i[1][0]) + '    type : ' + i[0])
    f.close()

    # write every indivudial warning
    for key,value in warnings.items():
        f = open(str(key) + '.txt', 'w')
        for i in value[1]:
            f.write(i + '\n')
        f.close()

    # some warnings coused by same place in code occur more than once
    print('number of same lines : ' + str(k))
    print("DONE!")



main()
