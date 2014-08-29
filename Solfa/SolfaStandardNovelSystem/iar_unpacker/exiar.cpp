// exiar.cpp, v1.1 2012/10/27
// coded by asmodean

// contact: 
//   web:   http://asmodean.reverse.net
//   email: asmodean [at] hush.com
//   irc:   asmodean on efnet (irc.efnet.net)

// This tool extracts graphics from iar (*.iar) archives.  Image fragments
// are merged with the base.

// Thanks to puu for the decompression code.  I started implementing it myself
// but my code wasn't looking any prettier, so I stopped :).

#include "as-util.h"
#include "decode_lz.h"
#include "pngfile.h"

typedef unsigned char uchar;
typedef unsigned short ushort;
typedef unsigned long ulong;

// Define this to support V3 of IAR achives.  The TOC entries have a slightly
// different structure.
#define IAR_VERSION 4

struct IARHDR {
  uchar signature[4]; // "iar "
  ulong version;
  ulong unknown1;
  ulong unknown2;
  ulong unknown3;
  ulong unknown4;
  ulong entry_count;
  ulong entry_count2; // why?
};

struct IARENTRY {
  ulong offset;
#if IAR_VERSION >= 3
  ulong unknown;
#endif
};

struct IARDATAHDR {
  ulong flags;
  ulong unknown1;
  ulong original_length;
  ulong unknown2;
  ulong length;
  ulong pal_length;
  int  offset_x;
  int  offset_y;
  ulong width;
  ulong height;
  ulong stride;
#if IAR_VERSION >= 4
  uchar unknown5[28];
#else
  uchar unknown5[20];
#endif
};

static const ulong FLAGS_8BIT   = 0x00000002;
static const ulong FLAGS_24BIT  = 0x0000001c;
static const ulong FLAGS_ALPHA  = 0x00000020;
static const ulong FLAGS_FRAG   = 0x00000800;
static const ulong FLAGS_LAYERS = 0x00001000;

struct IARFRAGHDR {
  ulong base_index;
  ulong start_line;
  ulong line_count;
};

struct IARFRAGLINE {
  ushort seq_count;
};

struct IARFRAGSEQ {
  ushort skip_length;
  ushort copy_length;
};

void read_entry(ulong    fd,
                IARENTRY*   entries, 
                ulong    index, 
                IARDATAHDR& datahdr, 
                uchar*&   out_buff, 
                ulong&   out_len,
                ushort&   out_depth)
{
	lseek(fd, entries[index].offset, SEEK_SET);
	read(fd, &datahdr, sizeof(datahdr));

	ulong len = datahdr.length;
	auto     buff = new uchar[len];
	read(fd, buff, len);

	out_len = datahdr.original_length;

	if (datahdr.length != datahdr.original_length)
	{
		out_buff = new uchar[out_len];
		decode_lz(buff, len, out_buff, out_len);

		delete[] buff;
	}
	else
	{
		out_buff = buff;
	}

	if (datahdr.flags & FLAGS_8BIT)
	{
		out_depth = 8;
	}
	else
	{
		out_depth = 24;
	}

	// Assume we never have 8-bit data with alpha
	if (datahdr.flags & FLAGS_ALPHA)
	{
		out_depth = 32;
	}
}


//祝樱的ui iar文件没用到这个函数，暂且不分析
void merge_fragment(int           fd,
                    IARENTRY*     entries,
                    const string& prefix, 
                    ulong      index,
                    ulong&     width,
                    ulong&     height,
                    ulong&     stride,
                    uchar*      buff,
                    uchar*&     out_buff,
                    ulong&     out_len) 
{
  auto hdr = (IARFRAGHDR*) buff;
  buff += sizeof(*hdr);
 
  ushort depth = 0;

  {
    IARDATAHDR datahdr;
    ulong   temp_len  = 0;
    uchar*   temp_buff = NULL;
    read_entry(fd, entries, hdr->base_index, datahdr, temp_buff, temp_len, depth);

	  ulong y_offset = 0;
    ulong x_offset = 0;

    ulong out_y_offset = height - datahdr.height;

	  if (datahdr.height > height) {
      y_offset      = datahdr.height - height;
      out_y_offset  = 0;
	  }

    if (datahdr.width > width) {
      x_offset      = datahdr.stride - stride;
    }

    out_len  = height * stride;
    out_buff = new uchar[out_len];
    memset(out_buff, 0, out_len);

    for (ulong y = y_offset; y < datahdr.height; y++) {
      memcpy(out_buff + (y - y_offset + out_y_offset) * stride,
             temp_buff + y * datahdr.stride,
             datahdr.stride - x_offset);
    }

    delete [] temp_buff;
  }

  depth /= 8;

  ulong current_line = hdr->start_line;

  for (ulong i = 0; i < hdr->line_count; i++) {
    auto line_hdr = (IARFRAGLINE*) buff;
    buff += sizeof(*line_hdr);

    auto out_line = out_buff + current_line * stride;
    current_line++;

    ulong x = 0;

    for (int j = 0; j < line_hdr->seq_count; j++) {
      auto line_seq = (IARFRAGSEQ*) buff;
      buff += sizeof(*line_seq);

      ulong skip_bytes = line_seq->skip_length * depth;
      ulong copy_bytes = line_seq->copy_length * depth;

      x += skip_bytes;

      memcpy(out_line + x, buff, copy_bytes);
      buff += copy_bytes;
      x    += copy_bytes;
    }
  }
}


//祝樱的ui iar文件没用到这个函数，暂且不分析
void process_layers(int           fd,
                    IARENTRY*     entries,
                    const string& prefix,
                    uchar*      buff, 
                    ulong      len,
                    ulong      width,
                    ulong      height)
{

/*
  static const uchar LAYER_TYPE_IMAGE  = 0x00;
  static const uchar LAYER_TYPE_MASK   = 0x20;
  static const uchar LAYER_TYPE_OFFSET = 0x21;

  as::write_file(prefix + ".layers", buff, len);

  string      filename = prefix + "=";
  short     offset_x = 0;
  short     offset_y = 0;
  as::image_t image;

  auto end = buff + len;

  while (buff < end) {
    uchar c = *buff++;

    switch (c) {
    case LAYER_TYPE_OFFSET: 
      offset_x += *(short*)buff;
      buff += 2;
      offset_y += *(short*)buff;
      buff += 2;
      break;

    case LAYER_TYPE_MASK:
    case LAYER_TYPE_IMAGE:
      {
        ulong index = *(ulong*)buff;
        buff += 4;

        IARDATAHDR datahdr;
        ulong   temp_len  = 0;
        uchar*   temp_buff = NULL;
        ushort   depth     = 0;
        read_entry(fd, entries, index, datahdr, temp_buff, temp_len, depth);

        datahdr.offset_x -= offset_x;
        datahdr.offset_y -= offset_y;

        as::image_t layer(datahdr.width,
                          datahdr.height,
                          depth / 8,
                          temp_buff,
                          -datahdr.offset_x,
                          -datahdr.offset_y);

        delete [] temp_buff;

        layer.flip();

        if (image.valid()) {
          filename += as::stringf("+%05d", index);

          if (c == LAYER_TYPE_MASK) {
            layer.make_32bit();

            as::image_t mask = image;
            mask.clear();
            mask.blend(layer);

            layer.clear();
            image.blend(layer);
            
            for (ulong y = 0; y < image.height(); y++) {
              auto src = mask.row(y);
              auto dst = image.row(y);

              for (ulong x = 0; x < image.width(); x++) {
                if (!src.r()) {
                  dst.a() = 0;
                }

                ++src;
                ++dst;
              }
            }
          } else {
            image.blend(layer);
          }
        } else {
          filename += as::stringf("%05d", index);
          image = layer;
        }
      }
      break;

    default:
      printf("%s: unknown layer type %c\n", prefix.c_str(), c);
      break;
    }
  }

  image.trim();

  image.write(filename + ".bmp");
  */
}



int main(int argc, char** argv) {
  if (argc != 2) {
    fprintf(stderr, "exiar v1.1 by asmodean\n\n");
    fprintf(stderr, "usage: %s <input.iar>\n", argv[0]);
    return -1;
  }

  string in_filename(argv[1]);
  string prefix(as::get_file_prefix(in_filename, true));

  int fd = as::open_or_die(in_filename, O_RDONLY | O_BINARY);

  FILE* fh = as::open_or_die_file(prefix + "+info.txt", "w");
  fprintf(fh, "# width\theight\toffset_x\toffset_y\tname\n");

  IARHDR hdr;
  read(fd, &hdr, sizeof(hdr));

  auto entries = new IARENTRY[hdr.entry_count];
  read(fd, entries, sizeof(IARENTRY) * hdr.entry_count);

  for (ulong i = 0; i < hdr.entry_count; i++) 
  {  
    IARDATAHDR datahdr;
    ulong   out_len  = 0;
    uchar*   out_buff = NULL;
    ushort   depth    = 0;

    read_entry(fd, entries, i, datahdr, out_buff, out_len, depth);

    string name = prefix + as::stringf("+%05d", i);

    fprintf(fh, 
            "%d\t%d\t%d\t%d\t%s\n", 
            datahdr.width, 
            datahdr.height, 
            datahdr.offset_x, 
            datahdr.offset_y,
            name.c_str());

    if (datahdr.flags & FLAGS_LAYERS) 
    {
	
      process_layers(fd, 
                     entries, 
                     name, 
                     out_buff, 
                     out_len, 
                     datahdr.width, 
                     datahdr.height);
					 
    } 
    else 
    {
      if (datahdr.flags & FLAGS_FRAG) 
      {
        ulong temp_len  = 0;
        uchar* temp_buff = NULL;

        merge_fragment(fd,
                       entries,
                       prefix,
                       i, 
                       datahdr.width,
                       datahdr.height,
                       datahdr.stride,
                       out_buff,
                       temp_buff,
                       temp_len);

        delete [] out_buff;

        out_len  = temp_len;
        out_buff = temp_buff;
      }


	  pic_data png;
	  png.bit_depth = 8;
	  png.width = datahdr.width;
	  png.height = datahdr.height;
	  png.rgba = out_buff;
	  if (depth == 32)
	  {
		  png.flag = HAVE_ALPHA;
		  
	  }
	  else if (depth == 24)
	  {
		  png.flag = NO_ALPHA;
	  }
	  else
	  {
		  printf("don't support for 8 bit image!\n");
		  continue;
	  }

	  write_png_file(name + ".png", &png);
		  
	  /*
      as::write_bmp_ex(name + ".bmp",
                       out_buff,
                       out_len,
                       datahdr.width,
                       datahdr.height,
                       depth / 8,
                       256,
                       NULL,
                       as::WRITE_BMP_FLIP);
					   */
    }

    delete [] out_buff;
  }

  delete [] entries;

  fclose(fh);
  close(fd);

  return 0;
}
