// n64romdumpconverter.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>
#include <vector>
#include <fstream>
#include <stdint.h>

using namespace std;

const unsigned int z64 = 0x80371240;
const unsigned int n64 = 0x37804012;
const unsigned int v64 = 0x40123780;

const int z64_n64 = 1;
const int z64_v64 = 2;
const int n64_v64 = 3;


int main(int argc, char* argv[])
{
    // arguments
    if (argc != 3) {
        printf("Wrong number of arguments: %i\n", argc - 1);
        printf("Usage:\n\tn64romdumpconverter.exe <path/to/rom_dump> <format_to_convert_to>");
        printf("\n\tz: z64\n\tn: n64\n\tv: v64\n");
        return 0x01;
    }

    if (FILE* file = fopen(argv[1], "r")) {
        fclose(file);
        printf("File to convert: %s\n", argv[1]);
    }
    else {
        printf("\nFile doesn't exist/can't be opened\n");
        return 0x02;
    }

    int target_format;
    if (strlen(argv[2]) != 1 || 
        (*argv[2] != 0x7a &&
        *argv[2] != 0x6e && 
        *argv[2] != 0x76)) {
        printf("\nWrong target format\n");
        return 0x03;
    }
    else {
        target_format = *argv[2];

        string format = "";
        switch (target_format) {
        case 0x7a:
            format = "z64";
            target_format = 0;
            break;
        case 0x6e:
            format = "n64";
            target_format = 1;
            break;
        case 0x76:
            format = "v64";
            target_format = 2;
            break;
        }
        printf("Target format: %s\n", format.c_str());
    }

    // read file to buffer
    vector<char> read_buf;

    ifstream infile(argv[1], ios::binary | ios::ate);
    if (!infile) {
        printf("\nFailed to open file\n");
        return 0x04;
    }

    streamsize size = infile.tellg();
    infile.seekg(0, std::ios::beg);

    read_buf = std::vector<char>(size);

    if (!infile.read(read_buf.data(), size)) {
        printf("\nFailed to read file\n");
        return 0x05;
    }

    infile.close();

    // copy buffer
    unsigned int buf_size = read_buf.size();
    uint8_t* proc_buf = new uint8_t[buf_size];
    for (int i = 0; i < buf_size; i++) {
        *(proc_buf + i) = (uint8_t)(read_buf[i] & 0xff);
    }

    // determine rom dump format (input)
    unsigned int rom_id = 0x00;
    for (int i = 0; i < 4; i++) {
        rom_id |= *(proc_buf + i) << ((3 - i) * 8);
    }

    int source_format;
    string format = "";
    switch (rom_id) {
    case z64:
        source_format = 0;
        format = "z64";
        break;
    case n64:
        source_format = 1;
        format = "n64";
        break;
    case v64:
        source_format = 2;
        format = "v64";
        break;
    default:
        printf("\nROM dump format not recognized\n");
        return 0x06;
    }
    printf("Source format: %s\n", format.c_str());

    if (source_format == target_format) {
        printf("\nROM dump format already corresponds to the target format !\n");
        return 0x00;
    }

    // change byte order
    int op_type = target_format + source_format;
    uint16_t tmp;
    switch (op_type) {
    case z64_n64:
        printf("Converting z64<->n64 ...\n");
        for (int i = 0; i < buf_size; i += 2) {
            tmp = *(uint16_t*)(proc_buf + i);
            *(proc_buf + i) = (uint8_t)((tmp & 0xff00) >> 8);
            *(proc_buf + i + 1) = (uint8_t)(tmp & 0xff);
        }
        break;
    case z64_v64:
        printf("Converting z64<->v64 ...\n");
        for (int i = 0; i < buf_size; i += 4) {
            for (int j = 0; j < 2; j++) {
                tmp = *(proc_buf + i + j);
                *(proc_buf + i + j) = *(proc_buf + i + (3 - j));
                *(proc_buf + i + (3 - j)) = tmp;
            }
        }
        break;
    case n64_v64:
        printf("Converting n64<->v64 ...\n");
        for (int i = 0; i < buf_size; i += 4) {
            tmp = *(uint16_t*)(proc_buf + i);
            *(uint16_t*)(proc_buf + i) = *(uint16_t*)(proc_buf + i + 2);
            *(uint16_t*)(proc_buf + i + 2) = tmp;
        }
        break;
    }
    printf("DONE !\n");

    // write buffer to file
    char* target_file = argv[1];
    for (int i = strlen(target_file); i > 0; --i) {
        if (*(target_file + i) == 0x2e) {
            switch (target_format) {
            case 0:
                *(target_file + i + 1) = 0x7a;
                break;
            case 1:
                *(target_file + i + 1) = 0x6e;
                break;
            case 2:
                *(target_file + i + 1) = 0x76;
                break;
            }
        }
    }

    printf("Creating and writing target file: %s ...\n", target_file);

    ofstream outfile(target_file, ios::binary | ios::ate);
    if (!outfile) {
        printf("Can't create/open target file\n");
        return 0x07;
    }

    for (int i = 0; i < buf_size; i++) {
        outfile.write((char*)(proc_buf + i), sizeof(uint8_t));
    }
    outfile.close();
    printf("DONE !");

    return 0x00;
}