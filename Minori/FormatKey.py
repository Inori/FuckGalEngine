


def main():
    with open('key.txt') as src, open('out.txt', 'w') as dst:

        array = ''
        for line in src.readlines():
            line = line.rstrip('\n')
            byte = line[-2:]
            hex = '0x{}, '.format(byte)
            array += hex

        dst.write(array)



if __name__ == '__main__':
    main()