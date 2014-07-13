using System;
using System.IO;

namespace SysgrpConverter
{
    internal class Program
    {
        private static void Main(string[] args)
        {
            if (args.Length != 1)
            {
                Console.Write("Usage: SysgrpConverter.exe <sysgrp filename>|<bmp filename>");
                Console.ReadKey();
                return;
            }

            var strFilename = args[0];

            if (Path.GetExtension(strFilename) == "")
                BeginBuildBMP(strFilename);
            else if (Path.GetExtension(strFilename).ToLower() == ".bmp")
                BeginBuildResource(strFilename);
            else
                Console.WriteLine("ERROR:" + strFilename);
        }

        private static void BeginBuildResource(string strBmpFile)
        {
            int width;
            int height;
            int depth;
            byte[] bits;

            using (var br = new BinaryReader(new FileStream(strBmpFile, FileMode.Open)))
            {
                br.BaseStream.Position = 0x12;
                width = br.ReadInt32();
                height = br.ReadInt32();
                br.BaseStream.Position = 0x1C;
                depth = br.ReadInt16();

                br.BaseStream.Position = 0x36;
                //bits = br.ReadBytes((int)br.BaseStream.Length - 0x36);
                bits = br.ReadBytes(width * Math.Abs(height) * (depth / 8));
                br.Close();
            }

            using (var bw = new BinaryWriter(new FileStream(strBmpFile + ".out", FileMode.Create)))
            {
                bw.Write((Int16) width);
                bw.Write((Int16) Math.Abs(height));
                bw.Write((Int16) depth);
                bw.Write((Int16) 0);
                bw.Write((Int32) 0);
                bw.Write((Int32) 0);
                bw.Write(bits);

                bw.Close();
            }
        }

        private static void BeginBuildBMP(string strFilename)
        {
            int width;
            int height;
            int depth;
            byte[] bits;

            using (var br = new BinaryReader(new FileStream(strFilename, FileMode.Open)))
            {
                width = br.ReadInt16();
                height = br.ReadInt16();
                depth = br.ReadInt16();

                br.BaseStream.Position = 0x10;
                bits = br.ReadBytes((int) br.BaseStream.Length - 0x10);
                br.Close();
            }

            using (var bw = new BinaryWriter(new FileStream(strFilename + ".bmp", FileMode.Create)))
            {
                bw.Write(BuildBMP(bits, width, -height, depth));
                bw.Close();
            }
        }

        // For details, read http://en.wikipedia.org/wiki/BMP_file_format#File_structure.
        public static byte[] BuildBMP(byte[] bits, int width, int height, int depth)
        {
            var ms = new MemoryStream();

            //magic
            ms.WriteByte(0x42);
            ms.WriteByte(0x4D);
            //filesz
            ms.Write(BitConverter.GetBytes(bits.Length + 0x36), 0, 4);
            //creator1 + creator2
            ms.Write(BitConverter.GetBytes(0), 0, 4);
            //bmp_offset
            ms.Write(BitConverter.GetBytes(0x36), 0, 4);

            //header_sz
            ms.Write(BitConverter.GetBytes(0x28), 0, 4);
            //width
            ms.Write(BitConverter.GetBytes(width), 0, 4);
            //height
            ms.Write(BitConverter.GetBytes(height), 0, 4);
            //nplanes
            ms.Write(BitConverter.GetBytes((Int16) 1), 0, 2);
            //bitspp
            ms.Write(BitConverter.GetBytes((Int16) depth), 0, 2);
            //set other info to 0
            ms.Write(new byte[]
                {
                    0, 0, 0, 0, 0, 0,
                    0, 0, 0, 0, 0, 0,
                    0, 0, 0, 0, 0, 0,
                    0, 0, 0, 0, 0, 0,
                }, 0, 24);
            //write bits
            ms.Write(bits, 0, bits.Length);

            return ms.ToArray();
        }
    }
}