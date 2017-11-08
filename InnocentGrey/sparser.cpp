#include <iostream>
#include <vector>
#include <sstream>
#include <fstream>
#include <cstdlib>
#include <string>
#include <cstring>
#include <io.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <windows.h>

using namespace std;

typedef char s8;
typedef short s16;
typedef int s32;
typedef long long s64;

typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;
typedef unsigned long long u64;

u32 get_file_size(string file_name)
{
    struct _stat buf;
    _stat(file_name.c_str(), &buf);
    return buf.st_size;
}
string get_string(const vector<u8> &in_script, u32 string_start_pos, u32 string_len)
{
    stringstream ss;
    for (u32 index = string_start_pos; index < string_start_pos + string_len; ++index)
    {
        if (in_script[index] == 0x00) break;
        ss << in_script[index];
    }

    string out_string = ss.str();
    return out_string;
}
inline u8 read_u8_le(const vector<u8> &in_script, u32 pos)
{
    u8 result = in_script[pos];
    return result;
}
inline u16 read_u16_le(const vector<u8> &in_script, u32 pos)
{
    u16 result = (in_script[pos + 1] << 8) + in_script[pos];
    return result;
}
inline u32 read_u32_le(const vector<u8> &in_script, u32 pos)
{
    u32 result =
        (in_script[pos + 3] << 0x18) + (in_script[pos + 2] << 0x10) + (in_script[pos + 1] << 0x08) + in_script[pos];
    return result;
}
void parse_script(const vector<u8> &in_script, u32 in_script_size, ofstream &out_txt)
{
    u32 index = 0;
    u8 opcode = 0;
    u8 opcode_len = 0;
    string text;
    vector<string> t_suo_png;
    while (index < in_script_size)
    {
        opcode = in_script[index];
        // printf("opcode %02X pos %02X\n", opcode, index);
        opcode_len = in_script[index + 1];
        switch (opcode)
        {
            // plain text
            case 0x00:
            {
                u32 text_start_pos = index + opcode_len;
                u8 text_len = in_script[index + 3];
                text = get_string(in_script, text_start_pos, text_len);
                out_txt << text << endl;
                index += text_len;
                break;
            }
            // bubble text(will not displayed)
            case 0x3F:
            {
                u32 text_start_pos = index + opcode_len;
                u8 text_len = in_script[index + 3];
                text = get_string(in_script, text_start_pos, text_len);
                if (!t_suo_png.empty())
                {
                    for (u32 i = 0; i < t_suo_png.size(); ++i) out_txt << "/" << t_suo_png[i] << "@";
                    t_suo_png.clear();
                }
                out_txt << text << endl;
                index += text_len;
                break;
            }
            // option and jump
            case 0x1D:
            {
                u32 text_start_pos = index + opcode_len;
                u16 text_len = read_u16_le(in_script, index + 2);
                u32 jump_offset = read_u32_le(in_script, index + 4);
                text = get_string(in_script, text_start_pos, text_len);
                out_txt << text << endl;
                index += text_len;
                break;
            }
            // jump
            case 0x0D:
            {
                break;
            }
            // second round jump
            case 0x3B:
            {
                u32 jump_offset = read_u32_le(in_script, index + 4);
                u16 content = read_u16_le(in_script, index + 2);
                break;
            }
            // goto script file
            case 0x02:
            {
                u32 script_filename_start_pos = index + opcode_len;
                u8 script_filename_len = in_script[index + 3];
                string script_filename = get_string(in_script, script_filename_start_pos, script_filename_len);
                index += script_filename_len;
                break;
            }
            // script end
            case 0x01:
            {
                break;
            }
            // bmp
            case 0x0F:
            case 0x10:
            {
                u32 bmp_filename_start_pos = index + opcode_len;
                u8 bmp_filename_len = in_script[index + 3];
                string bmp_filename = get_string(in_script, bmp_filename_start_pos, bmp_filename_len);
                index += bmp_filename_len;
                break;
            }
            // png
            case 0x12:
            case 0xB4:
            // bubble picture
            case 0x9C:
            {
                u32 png_filename_start_pos = index + opcode_len;
                u8 png_filename_len = in_script[index + 3];
                string png_filename = get_string(in_script, png_filename_start_pos, png_filename_len);
                if (png_filename[0] == 't' && png_filename[1] == '_') t_suo_png.push_back(png_filename);
                index += png_filename_len;
                break;
            }
            // ogg1
            case 0x22:
            case 0x27:
            case 0x28:
            {
                u32 ogg_filename_start_pos = index + opcode_len;
                u8 ogg_filename_len = in_script[index + 7];
                string ogg_filename = get_string(in_script, ogg_filename_start_pos, ogg_filename_len);
                index += ogg_filename_len;
                break;
            }
            // ogg2
            case 0x25:
            case 0x2D:
            {
                u32 ogg_filename_start_pos = index + opcode_len;
                u8 ogg_filename_len = in_script[index + 8];
                string ogg_filename = get_string(in_script, ogg_filename_start_pos, ogg_filename_len);
                index += ogg_filename_len;
                break;
            }
            // unknown resource
            case 0xAE:
            {
                u32 resource_filename_start_pos = index + opcode_len;
                u8 resource_filename_len = in_script[index + 3];
                string resource_filename = get_string(in_script, resource_filename_start_pos, resource_filename_len);
                index += resource_filename_len;
                break;
            }
            // unknown jump
            case 0x5F:
            case 0x64:
            {
                u32 jump_offset = read_u32_le(in_script, index + 4);
                break;
            }
            // unknown jump2
            case 0x06:
            case 0x07:
            case 0x08:
            case 0x09:
            case 0x0A:
            case 0x0B:
            {
                u32 jump_offset = read_u32_le(in_script, index + 0xC);
                break;
            }
            // opcode 0x60
            case 0x60:
            {
                break;
            }
            // endless loop
            case 0x2E:
            case 0x2F:
            case 0x34:
            case 0x4A:
            case 0x4B:
            case 0x76:
            case 0x77:
            case 0xA8:
            case 0xA9:
            case 0xAA:
            case 0xAC:
            {
                printf("Alert! endless opcode found!\n");
                exit(-1);
            }
            case 0x0C:
            case 0x11:
            {
                break;
            }
            case 0x21:
            {
				u16 content = read_u16_le(in_script, index + 2);
                break;
            }
            //好感度操作
            case 0x04:
            case 0x05:
            {
				u16 event = read_u16_le(in_script, index + 2);
                break;
            }
            //选择肢相关
            case 0x1C:
            {
                break;
            }
            case 0x1B:
            {
                u16 option_number = read_u16_le(in_script, index + 2);
                break;
            }
            // normal opcode
            case 0x33:
            case 0x13:
            case 0xB5:
            case 0x40:
            case 0x14:
            case 0xEu:
            case 0x54:
            case 0x5A:
            case 0x5B:
            case 0x16:
            case 0xB3:
            case 0x50:
            case 0x51:
            case 0x83:
            case 0x29:
            case 0x2C:
            case 0x84:
            case 0x23:
            case 0x24:
            case 0x72:
            case 0x73:
            case 0x74:
            case 0x75:
            case 0x48:
            case 0x35:
            case 0x36:
            case 0xB8:
            case 0x81:
            case 0xAF:
            case 0xB1:
            case 0xB0:
            case 0xB2:
            case 0x57:
            case 0x1E:
            case 0xBA:
            case 0x4C:
            case 0xB6:
            case 0x4D:
            case 0x66:
            case 0x67:
            case 0x68:
            case 0x69:
            case 0x6A:
            case 0x6B:
            case 0x6E:
            case 0x7B:
            case 0x7C:
            case 0x7D:
            case 0xB9:
            case 0x78:
            case 0x79:
            case 0x5D:
            case 0x5E:
            case 0x61:
            case 0x8B:
            case 0x62:
            case 0x63:
            case 0xB7:
            case 0x3A:
            case 0x59:
            case 0x2A:
            case 0xBF:
            case 0xC0:
            case 0xC1:
            case 0xBB:
            case 0xBC:
            case 0xBD:
            case 0xBE:
            {
                break;
            }
            default:
            {
                printf("Error! Unknown opcode: %x\n", opcode);
                printf("File index: %x\n", index);
                exit(-1);
            }
        }
        index += opcode_len;
    }
}

int main(int argc, char *argv[])
{
    if (argc != 3 && argc != 4)
    {
        printf("Usage: %s input.s output.txt [-x]\n", argv[0]);
        return -1;
    }
    bool xor_flag = false;
    if (argc == 4 && argv[4][1] == 'x') xor_flag = true;
    ifstream in_s(argv[1], ios::binary);
    u32 in_script_size = get_file_size(argv[1]);
    u8 *in_script_buf = new u8[in_script_size];
    in_s.read((char *)in_script_buf, in_script_size);
    vector<u8> in_script(in_script_buf, in_script_buf + in_script_size);
    delete[] in_script_buf;
    if (xor_flag)
    {
        for (u32 i = 0; i < in_script_size; ++i)
        {
            in_script[i] ^= 0xFF;
        }
    }
    ofstream out_txt(argv[2], ios::binary);
    parse_script(in_script, in_script_size, out_txt);
    out_txt.close();
    return 0;
}
