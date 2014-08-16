using System;
using System.Collections.Generic;
using System.Linq;
using System.IO;
using System.Text;

using Amemiya.Extensions;

namespace ScriptEncoder
{
    internal class Program
    {
        private static void Main(string[] args)
        {
            if (args.Length == 0)
            {
                Console.WriteLine("Usage: ScriptEncoder.exe <input_file.txt>");
                Console.ReadKey();
                return;
            }

            string fileInput = args[0];

            // Load script file.
            var br = new BinaryReader(new FileStream(fileInput.Replace(".txt", ""), FileMode.Open));

            var scriptBuffer = br.ReadBytes((int) br.BaseStream.Length);
            

            // Load translation file.
            var sr = new StreamReader(fileInput, Encoding.UTF8, true);
            var lines = sr.ReadToEnd().Replace("\r\n", "\n").Split('\n');
            sr.Close();
            if (lines.Length == 0)
                return;

            // headerLength includes MAGIC and @[0x1C].
            int headerLength = 0;

            // Check whether the file is in new format.
            // The most significant thing is the new format have the magic "BurikoCompiledScriptVer1.00\x00".
            // The difference between old and new is that, the old one DOES NOT have the HEADER which has
            // the length discribed at [0x1C] as a DWORD.
            if (
                scriptBuffer.Slice(0, 0x1C)
                            .EqualWith(new byte[]
                                {
                                    0x42, 0x75, 0x72, 0x69, 0x6B,
                                    0x6F, 0x43, 0x6F, 0x6D, 0x70,
                                    0x69, 0x6C, 0x65, 0x64, 0x53,
                                    0x63, 0x72, 0x69, 0x70, 0x74,
                                    0x56, 0x65, 0x72, 0x31, 0x2E,
                                    0x30, 0x30, 0x00
                                }))
            {
                headerLength = 0x1C + BitConverter.ToInt32(scriptBuffer, 0x1C);
            }
            // else headerLength = 0;


            // Get control bytes from original buffer.
            //var controlStream = new MemoryStream(scriptBuffer.Slice(headerLength, GetSmallestOffset(lines) + headerLength));
            var controlStream = new MemoryStream(scriptBuffer.Slice(headerLength, scriptBuffer.Length));

            // Let's begin.
            var textStream = new MemoryStream();
            foreach (var line in lines)
            {
                if (String.IsNullOrEmpty(line)) continue;

                var info = GetLineInfo(line);
                controlStream.WriteInt32(info[0], (int) (controlStream.Length + textStream.Length));

                string curline = GetText(line);
                if (curline.IndexOf("_") == -1)
                {
                    textStream.WriteBytes(Encoding.GetEncoding(936).GetBytes(curline));
                }
                else
                {
                    textStream.WriteBytes(Encoding.GetEncoding(932).GetBytes(curline));
                }
                

                textStream.WriteByte(0x00);
            }

            // Build new script file.
            var bw = new BinaryWriter(new FileStream(fileInput + ".new", FileMode.Create));
            // Write HEADER.
            if (headerLength != 0) bw.Write(scriptBuffer.Slice(0, headerLength));
            // Control bytes.
            bw.Write(controlStream.ToArray());
            // Text bytes.
            bw.Write(textStream.ToArray());
            bw.Close();

            br.Close();
        }

        private static int GetSmallestOffset(IEnumerable<string> lines)
        {
            return lines.Min(line => String.IsNullOrEmpty(line) ? Int32.MaxValue : GetLineInfo(line)[1]);
        }

        private static int[] GetLineInfo(string line)
        {
            string[] s =
                line.Substring(line.IndexOf('<') + 1, line.IndexOf('>') - line.IndexOf('<') - 1).Split(',');

            var i = new int[3];
            i[0] = Int32.Parse(s[0]);
            i[1] = Int32.Parse(s[1]);
            i[2] = Int32.Parse(s[2]);

            return i;
        }

        private static string GetText(string line)
        {
            return line.Substring(line.IndexOf('>') + 1).Replace(@"\n", "\x0A");
        }
    }
}
